/*
 * Pseudotree.cpp
 *
 *  Copyright (C) 2008-2012 Lars Otten
 *  This file is part of DAOOPT.
 *
 *  DAOOPT is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  DAOOPT is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with DAOOPT.  If not, see <http://www.gnu.org/licenses/>.
 *  
 *  Created on: Oct 24, 2008
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#include "Pseudotree.h"

#undef DEBUG

namespace daoopt {

int Pseudotree::restrictSubproblem(int i) {

  assert(m_root && i<(int)m_nodes.size() && m_nodes[i]);

  if (m_root->getVar()==i) // root remains unchanged
    return m_root->getDepth();
  m_widthConditioned = NONE;

  int rootOldDepth = m_nodes[i]->getDepth();

  m_root->setChild(m_nodes[i]);
  m_nodes[i]->setParent(m_root);

  // recompute subproblem variables (recursive)
  m_root->updateSubprobVars(m_nodes.size());  // already includes dummy
  m_sizeConditioned = m_root->getSubprobSize()-1;  // -1 for dummy

  // update subproblem height, subtract -1 for bogus node
  m_heightConditioned = m_root->updateDepthHeight(-1) - 1;

  // compute subproblem width (when conditioning on subproblem context)
  const vector<int>& condset = m_nodes[i]->m_contextV;
  stack<PseudotreeNode*> stck;
  stck.push(m_nodes[i]);
  while (stck.size()) {
    PseudotreeNode* n = stck.top();
    stck.pop();
    int x = setminusSize(n->m_contextV, condset);
    m_widthConditioned = max(m_widthConditioned,x);
    for (vector<PseudotreeNode*>:: const_iterator it=n->getChildren().begin(); it!=n->getChildren().end(); ++it) {
      stck.push(*it);
    }
  }

  return rootOldDepth;
}

int Pseudotree::computeSubproblemWidth(int i) {
  int width = NONE;
  // compute subproblem width (when conditioning on subproblem context)
  const vector<int>& condset = m_nodes[i]->m_contextV;
  stack<PseudotreeNode*> stck;
  stck.push(m_nodes[i]);
  while (stck.size()) {
    PseudotreeNode* n = stck.top();
    stck.pop();
    int x = setminusSize(n->m_contextV, condset);
    width = max(width,x);
    for (vector<PseudotreeNode*>:: const_iterator it=n->getChildren().begin(); it!=n->getChildren().end(); ++it) {
      stck.push(*it);
    }
  }
  return width;
}

void Pseudotree::resetFunctionInfo(const vector<Function*>& fns) {
  // reset existing function mapping (if any)
  for (vector<PseudotreeNode*>::iterator it=m_nodes.begin(); it!=m_nodes.end(); ++it)
    (*it)->resetFunctions();
  // generate new function mapping
  for (vector<Function*>::const_iterator itF=fns.begin(); itF!=fns.end(); ++itF) {
    const set<int>& scope = (*itF)->getScopeSet();
    /*
    cout << "Function: " << **itF << endl;
    cout << "Scope: " << scope << endl;
    */
    if (scope.size() == 0) {
      m_nodes[m_elimOrder.back()]->addFunction(*itF);  // dummy variable
      continue;
    }
    vector<int>::const_iterator it = m_elimOrder.begin();
    for (;;++it) {
      if (scope.find(*it)!=scope.end()) {
        m_nodes[*it]->addFunction(*itF);
        break;
      }
    }
  }
}

void Pseudotree::addDomainInfo(const vector<val_t>& domains) {
  assert(domains.size() == m_nodes.size());
  for(size_t i = 0; i < domains.size(); ++i)
    m_nodes.at(i)->setDomain(domains.at(i));
}

/* computes an elimination order into 'elim' and returns its tree width */
int Pseudotree::eliminate(Graph G, vector<int>& elim, int limit, int tolerance) {

  int width = UNKNOWN;
  int n = G.getStatNodes();

  if (elim.size())
    elim.clear();
  elim.reserve(n);

  const set<int>& nodes = G.getNodes();

  // keeps track of node scores
  vector<nCost> scores;

  // have initial scores already been computed? then reuse!
  if (m_initialScores.size()) {
    scores = m_initialScores;
  } else {
    scores.resize(n);
    for (set<int>::const_iterator it = nodes.begin(); it!=nodes.end(); ++it)
      scores[*it] = G.scoreMinfill(*it);
    m_initialScores = scores;
  }

  int nextNode = NONE;

  // keeps track of minimal score nodes
  vector<vector<int>* > candidates(tolerance+1, NULL);
  for (int i=0; i<=tolerance; ++i)
    candidates[i] = new vector<int>;
  vector<nCost> candScore(tolerance+1, numeric_limits<nCost>::max());
  vector<int> simplicial; // simplicial nodes (score 0)

  // eliminate nodes until all gone
  while (G.getStatNodes() != 0) {

    for (int i=0; i<=tolerance; ++i) {
      candidates[i]->clear();
      candScore[i] = numeric_limits<nCost>::max();
    }
    simplicial.clear();

    // find node to eliminate
    for (int i=0; i<n; ++i) {
      if (scores[i] == 0) { // score 0
        simplicial.push_back(i);
      } else {
        if (tolerance == 0) {
          if (scores[i] == candScore[0])
            candidates[0]->push_back(i);
          else if (scores[i] < candScore[0]) {
            candidates[0]->clear();
            candidates[0]->push_back(i);
            candScore[0] = scores[i];
          }
        } else {
          for (int j=0; j<=tolerance; ++j) {
            if (scores[i] == candScore[j]) {
              candidates[j]->push_back(i);
              break;
            }
            else if (scores[i] < candScore[j]) {
              candidates[tolerance]->clear();
              vector<int>* temp = candidates[tolerance];
              for (int k=tolerance; k>j; --k) {  // move back candidate lists
                candidates[k] = candidates[k-1];
                candScore[k] = candScore[k-1];
              }
              candidates[j] = temp;
              candidates[j]->push_back(i);
              candScore[j] = scores[i];
              break;
            }
          }
        }
      }
    }

    // eliminate all nodes with score=0 -> no edges will have to be added
    for (vector<int>::iterator it=simplicial.begin(); it!=simplicial.end(); ++it) {
      elim.push_back(*it);
      width = max(width,(int)G.getNeighbors(*it).size());
      G.removeNode(*it);
      scores[*it] = numeric_limits<nCost>::max();
    }

    // anything left to eliminate? If not, we are done!
    if (candScore[0] == numeric_limits<nCost>::max()) {
      for (int i=0; i<=tolerance; ++i)
        delete candidates[i];
      return width;
    }

    // Pick one of the minimal score nodes (with score >= 1),
    // breaking ties randomly
    if (tolerance == 0) {
      nextNode = candidates[0]->at(rand::next(candidates[0]->size()));
    } else {
      size_t candTotal = 0;
      for (int i=0; i<=tolerance; ++i) {
        if (candScore[i] == numeric_limits<nCost>::max())
          break;
        candTotal += candidates[i]->size();
      }
      size_t choice = rand::next(candTotal);
      for (int i=0; i<=tolerance; ++i) {
        if (choice < candidates[i]->size()) {
          nextNode = candidates[i]->at(choice);
          break;  // for loop
        } else
          choice -= candidates[i]->size();
      }
    }
    elim.push_back(nextNode);

    // remember it's neighbors, to be used later
    const set<int>& neighbors = G.getNeighbors(nextNode);

    // update width of implied tree decomposition
    width = max(width, (int) neighbors.size());

    // early termination condition: width above given limit
    if (width > limit) {
      for (int i=0; i<=tolerance; ++i)
        delete candidates[i];
      return INT_MAX;
    }

    // connect neighbors in primal graph
    G.addClique(neighbors);

    // compute candidates for score update (node's neighbors and their neighbors)
    set<int> updateCand(neighbors);
    for (set<int>::const_iterator it = neighbors.begin(); it != neighbors.end(); ++it) {
      const set<int>& X = G.getNeighbors(*it);
      updateCand.insert(X.begin(),X.end());
    }
    updateCand.erase(nextNode);

    // remove node from primal graph
    G.removeNode(nextNode);
    scores[nextNode] = numeric_limits<nCost>::max(); // tag score

    // update scores in primal graph (candidate nodes computed earlier)
    for (set<int>::const_iterator it = updateCand.begin(); it != updateCand.end(); ++it) {
      scores[*it] = G.scoreMinfill(*it);
    }
  }

  for (int i=0; i<=tolerance; ++i)
    delete candidates[i];
  return width;
}


void Pseudotree::buildChain(Graph G, const vector<int>& elim, const int cachelimit) {


  int oldWidth = m_width;
  if (m_height != UNKNOWN)
    this->reset();
  m_width = oldWidth;

  const int n = G.getStatNodes();
  assert(n == (int) m_nodes.size());
  assert(n == (int) elim.size());

  set<int> context; // maintain the running context
  PseudotreeNode* prev = NULL;

  // build actual pseudo tree
  for (vector<int>::const_iterator it=elim.begin(); it!=elim.end(); ++it) {
    const set<int>& N = G.getNeighbors(*it);

    // update running context
    context.erase(*it);
    context.insert(N.begin(),N.end());

    // can't compute width correctly using an OR chain, but this gives the pathwidth
    m_pathwidth = max(m_pathwidth,(int)context.size());
    
//    m_width = max(m_width,(int)context.size());

    // create pseudo tree node
    m_nodes[*it] = new PseudotreeNode(this,*it,context);
    if (prev) {
      m_nodes[*it]->setChild(prev);
      prev->setParent(m_nodes[*it]);
    }
    prev = m_nodes[*it];

    G.addClique(N);
    G.removeNode(*it);

  }

  // compute contexts for adaptive caching
  for (vector<PseudotreeNode*>::iterator it = m_nodes.begin(); it!=m_nodes.end(); ++it) {
    const set<int>& ctxt = (*it)->m_contextS;
    if (cachelimit == NONE || cachelimit >= (int) ctxt.size()) {
      (*it)->setCacheContext(ctxt);
      continue; // proceed to next node
    }
    int j = cachelimit;
    PseudotreeNode* p = (*it)->getParent();
    set<int> newCacheCtxt;
    while (j--) { // note: cachelimit < ctxt.size()
      while (ctxt.find(p->getVar()) == ctxt.end() )
        p = p->getParent();
      newCacheCtxt.insert(p->getVar());
      p = p->getParent();
    }
    (*it)->setCacheContext(newCacheCtxt);
    // find reset variable for current node's cache table
    while (ctxt.find(p->getVar()) == ctxt.end() )
      p = p->getParent();
    p->addCacheReset((*it)->getVar());
//    cout << "AC for var. " << (*it)->getVar() << " context " << (*it)->getCacheContext()
//         << " out of " << (*it)->getFullContext() << ", reset at " << p->getVar() << endl;
  }

  // add artificial root node to connect disconnected components
  int bogusIdx = elim.size();
  PseudotreeNode* p = new PseudotreeNode(this,bogusIdx,set<int>());
  //p->addToContext(bogusIdx);

  p->addChild(prev);
  prev->setParent(p);
  m_components = 1;

  m_nodes.push_back(p);
  m_root = p;

  // remember the elim. order
  m_elimOrder = elim;
  m_elimOrder.push_back(bogusIdx); // add dummy variable as first node in ordering

  // initiate depth/height computation for tree and its nodes (bogus variable
  // gets depth -1), then need to subtract 1 from height for bogus as well
  m_height = m_root->updateDepthHeight(-1) - 1;

  // initiate subproblem width computation (recursive)
  m_root->updateSubWidth();

  // initiate subproblem variables computation (recursive)
  m_root->updateSubprobVars(m_nodes.size());
  m_size = m_root->getSubprobSize() -1; // -1 for bogus

#if defined PARALLEL_DYNAMIC || defined PARALLEL_STATIC
  // compute depth->list<nodes> mapping
  m_levels.clear();
  m_levels.resize(m_height+2); // +2 bco. bogus node
  for (vector<PseudotreeNode*>::iterator it = m_nodes.begin(); it!= m_nodes.end(); ++it) {
    m_levels.at((*it)->getDepth()+1).push_back(*it);
  }
#endif

  return;


}


/* builds the pseudo tree according to order 'elim' */
void Pseudotree::build(Graph G, const vector<int>& elim, const int cachelimit) {

  if (m_height != UNKNOWN)
    this->reset();

  const int n = G.getStatNodes();
  assert(n == (int) m_nodes.size());
  assert(n == (int) elim.size());

  list<PseudotreeNode*> roots;

  // build actual pseudo tree(s)
  for (vector<int>::const_iterator it = elim.begin(); it!=elim.end(); ++it) {
    const set<int>& N = G.getNeighbors(*it);
    m_width = max(m_width,(int)N.size());
    insertNewNode(*it,N,roots);
    G.addClique(N);
    G.removeNode(*it);
  }

  // compute contexts for adaptive caching
  for (vector<PseudotreeNode*>::iterator it = m_nodes.begin(); it!=m_nodes.end(); ++it) {
    const set<int>& ctxt = (*it)->m_contextS;
    if (cachelimit == NONE || cachelimit >= (int) ctxt.size()) {
      (*it)->setCacheContext(ctxt);
      continue; // proceed to next node
    }
    int j = cachelimit;
    PseudotreeNode* p = (*it)->getParent();
    set<int> newCacheCtxt;
    while (j--) { // note: cachelimit < ctxt.size()
      while (ctxt.find(p->getVar()) == ctxt.end() )
        p = p->getParent();
      newCacheCtxt.insert(p->getVar());
      p = p->getParent();
    }
    (*it)->setCacheContext(newCacheCtxt);
    // find reset variable for current node's cache table
    while (ctxt.find(p->getVar()) == ctxt.end() )
      p = p->getParent();
    p->addCacheReset((*it)->getVar());
//    cout << "AC for var. " << (*it)->getVar() << " context " << (*it)->getCacheContext()
//         << " out of " << (*it)->getFullContext() << ", reset at " << p->getVar() << endl;
  }

  // add artificial root node to connect disconnected components
  int bogusIdx = elim.size();
  PseudotreeNode* p = new PseudotreeNode(this,bogusIdx,set<int>());
  //p->addToContext(bogusIdx);
  for (list<PseudotreeNode*>::iterator it=roots.begin(); it!=roots.end(); ++it) {
    p->addChild(*it);
    (*it)->setParent(p);
    ++m_components; // increase component count
  }
  m_nodes.push_back(p);
  m_root = p;

  // remember the elim. order
  m_elimOrder = elim;
  m_elimOrder.push_back(bogusIdx); // add dummy variable as first node in ordering

  // initiate depth/height computation for tree and its nodes (bogus variable
  // gets depth -1), then need to subtract 1 from height for bogus as well
  m_height = m_root->updateDepthHeight(-1) - 1;

  // initiate subproblem width computation (recursive)
  m_root->updateSubWidth();

//  const vector<Pseudotree*> rootC = m_root->getChildren();
//  for (vector<Pseudotree*>::iterator it = rootC.begin(); it != rootC.end(); ++it) {
//    cout << (*it)->getSubWidth();
//  }

  // reorder each list of child nodes by complexity (width/height)
  for (vector<PseudotreeNode*>::iterator it = m_nodes.begin(); it!=m_nodes.end(); ++it)
    (*it)->orderChildren(m_subOrder);

  // initiate subproblem variables computation (recursive)
  m_root->updateSubprobVars(m_nodes.size());  // includes dummy
  m_size = m_root->getSubprobSize() - 1;  // -1 for dummy

#if defined PARALLEL_DYNAMIC || defined PARALLEL_STATIC
  // compute depth->list<nodes> mapping
  m_levels.clear();
  m_levels.resize(m_height+2); // +2 bco. bogus node
  for (vector<PseudotreeNode*>::iterator it = m_nodes.begin(); it!= m_nodes.end(); ++it) {
    m_levels.at((*it)->getDepth()+1).push_back(*it);
  }
#endif

  return;
}


Pseudotree::Pseudotree(const Pseudotree& pt) {

  m_problem = pt.m_problem;
  m_size = pt.m_size;
  m_subOrder = pt.m_subOrder;

  m_nodes.reserve(m_size+1);
  m_nodes.resize(m_size+1, NULL);

  m_elimOrder = pt.m_elimOrder;

  // clone PseudotreeNode structure
  stack<PseudotreeNode*> stack;

  PseudotreeNode *ptnNew = NULL, *ptnPar = NULL;
  // start with bogus variable, always has highest index
  m_nodes.at(m_size) = new PseudotreeNode(this,m_size,set<int>());
  m_root = m_nodes.at(m_size);
  stack.push(m_root);

  while (!stack.empty()) {
    ptnPar = stack.top();
    stack.pop();
    int var = ptnPar->getVar();
    const vector<PseudotreeNode*>& childOrg = pt.m_nodes.at(var)->getChildren();
    for (vector<PseudotreeNode*>::const_iterator it=childOrg.begin(); it!=childOrg.end(); ++it) {
       ptnNew = new PseudotreeNode(this, (*it)->getVar(), (*it)->m_contextS);
       m_nodes.at((*it)->getVar()) = ptnNew;
       ptnPar->addChild(ptnNew);
       ptnNew->setParent(ptnPar);
       ptnNew->setCacheContext( (*it)->m_cacheContextS );
       ptnNew->setCacheReset( (*it)->getCacheReset() );
       ptnNew->setFunctions( (*it)->getFunctions() );

       stack.push(ptnNew);
    }
  }

  m_height = m_root->updateDepthHeight(-1) - 1;
  m_root->updateSubWidth();
  m_root->updateSubprobVars(m_nodes.size());
  m_size = m_root->getSubprobSize() -1; // -1 for bogus

  m_heightConditioned = pt.m_heightConditioned;
  m_width = pt.m_width;
  m_widthConditioned = pt.m_widthConditioned;
  m_components = pt.m_components;
  m_sizeConditioned = pt.m_sizeConditioned;

  // reorder each list of child nodes by complexity (width/height)
  for (vector<PseudotreeNode*>::iterator it = m_nodes.begin(); it!=m_nodes.end(); ++it) {
    (*it)->orderChildren(m_subOrder);
  }

#if defined PARALLEL_DYNAMIC || defined PARALLEL_STATIC
  // compute depth->list<nodes> mapping
  m_levels.clear();
  m_levels.resize(m_height+2); // +2 bco. bogus node
  for (vector<PseudotreeNode*>::iterator it = m_nodes.begin(); it!= m_nodes.end(); ++it) {
    m_levels.at((*it)->getDepth()+1).push_back(*it); // dummy root has depth -1
  }
#endif

}


#if defined PARALLEL_DYNAMIC || defined PARALLEL_STATIC
/* a-priori computation of several complexity estimates, outputs various
 * results for increasing depth-based cutoff. Returns suggested optimal
 * cutoff depth (bad choice in practice) */
int Pseudotree::computeComplexities(int workers) {

  // compute subproblem complexity parameters of all nodes
  for (vector<PseudotreeNode*>::iterator it = m_nodes.begin(); it!= m_nodes.end(); ++it)
    (*it)->initSubproblemComplexity();

  bigint c = m_root->getSubCondBound();
  cout << "State space bound:\t" << mylog10(c) << " (" << c << ")" << endl;

  /*
   * iterate over levels, assuming cutoff, and compute upper bounds as follows:
   *   central - number of nodes explored by master down to this level
   *   tmax - max. subproblem size when conditioning at this level
   *   tsum - sum of all conditioned subproblems
   *   tnum - number of subproblems resulting from conditioning at this level
   *   bound - bounding expression reflecting parallelism over workers
   *   wmax - max. induced width of conditioned subproblems at this level
   */
  bigint central = 0, tmax=0, tsum=0, tnum=0, bound=0, size=0;
  int wmax = 0;
  vector<bigint> bounds; bounds.reserve(m_levels.size());
  for (vector<list<PseudotreeNode*> >::const_iterator it=m_levels.begin();
       it!=m_levels.end(); ++it) {
//    cout << "# " << central << '\t';
    tmax = 0; tsum=0; tnum=0; wmax=0;
    for (list<PseudotreeNode*>::const_iterator itL = it->begin(); itL!=it->end(); ++itL) {
      tmax = max(tmax, (*itL)->getSubCondBound() );
      tsum += (*itL)->getSubCondBound() * (*itL)->getNumContexts();
      tnum += (*itL)->getNumContexts();
      wmax = max(wmax, (*itL)->getSubCondWidth());
    }

    bigfloat ratio = 1;
    if (size != 0) {
      ratio = bigfloat(tsum+central) / size ;
    }
    size = tsum + central;

    bound = central + max( tmax, bigint(tsum / min(tnum,bigint(workers))) );
    bounds.push_back( bound );

//    cout << tnum << '\t' << (tmax) << '\t' << (tsum) << '\t' << (bound) << '\t' << ratio << '\t' << wmax << endl;
    for (list<PseudotreeNode*>::const_iterator itL = it->begin(); itL!=it->end(); ++itL) {
//      cout << " + " << (*itL)->getVar();
      central += (*itL)->getOwnsize();
    }
//    cout << endl;
  }
//  cout << "# " << central << "\t0\t0\t0\t" << central << "\t1\t0" << endl;

  int curDepth = -1, minDepth=UNKNOWN;
  bigint min = bounds.at(0)+1;
  for (vector<bigint>::iterator it = bounds.begin(); it != bounds.end(); ++it, ++curDepth) {
    if (*it < min) {
      min = *it;
      minDepth = curDepth;
    }
  }

  // TODO: Heuristically add 1 to minimizing level
  minDepth += 1;

  return minDepth;

}
#endif

/*
 * orders the sub pseudo trees of a node ("less" -> ascending by w*)
 */
void PseudotreeNode::orderChildren(int subOrder) {
  if (subOrder == SUBPROB_WIDTH_INC) {
    sort(m_children.begin(), m_children.end(), PseudotreeNode::compLess );
  } else if (subOrder == SUBPROB_WIDTH_DEC) {
    sort(m_children.begin(), m_children.end(), PseudotreeNode::compGreater );
  }
}


/* compares two pseudotree nodes, returns true if subtree below a is more complex
 * than below b (looks at width, then height) */
bool PseudotreeNode::compGreater(PseudotreeNode* a, PseudotreeNode* b) {
  assert (a && b);
  if ( a->getSubWidth() > b->getSubWidth() )
    return true;
  if ( a->getSubWidth() < b->getSubWidth() )
    return false;
  if ( a->getSubHeight() > b->getSubHeight() )
    return true;
  return false;
}

bool PseudotreeNode::compLess(PseudotreeNode* a, PseudotreeNode* b) {
  assert (a && b);
  if ( a->getSubWidth() < b->getSubWidth() )
    return true;
  if ( a->getSubWidth() > b->getSubWidth() )
    return false;
  if ( a->getSubHeight() < b->getSubHeight() )
    return true;
  return false;
}


/* updates a single node's depth and height, recursively updating the child nodes.
 * return value is height of node's subtree */
int PseudotreeNode::updateDepthHeight(int d) {

  m_depth = d;

  if (m_children.empty()) {
    m_subHeight = 0;
  }
  else {
    int m=0;
    for (vector<PseudotreeNode*>::iterator it=m_children.begin(); it!=m_children.end(); ++it)
      m = max(m, (*it)->updateDepthHeight(m_depth+1));
    m_subHeight = m + 1;
  }

  return m_subHeight;

}


/* recursively finds and returns the max. width in this node's subproblem */
int PseudotreeNode::updateSubWidth() {

  m_subWidth = m_contextV.size();

  for (vector<PseudotreeNode*>::iterator it=m_children.begin(); it!=m_children.end(); ++it)
    m_subWidth = max(m_subWidth, (*it)->updateSubWidth());

  return m_subWidth;

}


/* recursively updates the set of variables in the current subproblem */
const vector<int>& PseudotreeNode::updateSubprobVars(int numVars) {

  // clear current set
  m_subproblemVars.clear();
  // add self
  m_subproblemVars.push_back(m_var);

  // iterate over children and collect their subproblem variables
  for (vector<PseudotreeNode*>::iterator it = m_children.begin(); it!=m_children.end(); ++it) {
    const vector<int>& childVars = (*it)->updateSubprobVars(numVars);
    for (vector<int>::const_iterator itC = childVars.begin(); itC!=childVars.end(); ++itC)
      m_subproblemVars.push_back(*itC);
  }
  sort(m_subproblemVars.begin(), m_subproblemVars.end());

  m_subproblemVarMap.clear();
  m_subproblemVarMap.resize(numVars, NONE);
  size_t i = 0;
  for (vector<int>::const_iterator it = m_subproblemVars.begin();
       it != m_subproblemVars.end(); ++it, ++i) {
    m_subproblemVarMap[*it] = i;
  }

#ifdef DEBUG
//  cout << "Subproblem at var. " << m_var << ": " << m_subproblemVars << endl;
#endif

  // return a const reference
  return m_subproblemVars;
}

#if defined PARALLEL_STATIC || TRUE

void Pseudotree::computeSubprobStats() {
  vector<int> bogus;
  m_root->computeStatsCluster(bogus);
  m_root->computeStatsDomain(bogus);
  m_root->computeStatsLeafDepth(bogus);
  BOOST_FOREACH( PseudotreeNode* pt, m_nodes ) {
    pt->computeStatsClusterCond();
  }
//  cout << * m_root->getSubprobStats() << endl;
}

double Pseudotree::getStateSpaceCond() const {
  return m_root->getSubprobStats()->getStateSpaceCond();
}

void PseudotreeNode::computeStatsCluster(vector<int>& result) {
  result.clear();
  vector<int> chi;
  BOOST_FOREACH( PseudotreeNode* pt, m_children ) {
    pt->computeStatsCluster(chi);
    copy(chi.begin(), chi.end(), back_inserter(result));
  }
  result.push_back(m_contextV.size());
  m_subprobStats->setClusterStats(result);
}

void PseudotreeNode::computeStatsLeafDepth(vector<int>& result) {
  result.clear();
  if (m_children.empty()) {
    result.push_back(1);
  } else {
    vector<int> chi;
    BOOST_FOREACH( PseudotreeNode* pt, m_children ) {
      pt->computeStatsLeafDepth(chi);
      BOOST_FOREACH( int d, chi ) {
        result.push_back(d+1);
      }
    }
  }
  m_subprobStats->setDepthStats(result);  // also sets leaf count
}

void PseudotreeNode::computeStatsDomain(vector<int>& result) {
  result.clear();
  vector<int> chi;  // to collect child domain stats
  BOOST_FOREACH( PseudotreeNode* pt, m_children ) {
    pt->computeStatsDomain(chi);
    copy(chi.begin(), chi.end(), back_inserter(result));
  }
  result.push_back(m_domain);
  m_subprobStats->setDomainStats(result);  // also set var count
}

void PseudotreeNode::computeStatsClusterCond() {
  vector<int> result;
  double twbCond = this->computeStatsClusterCondSub(m_contextS, result);
  m_subprobStats->setClusterCondStats(result);
  m_subprobStats->setStateSpaceCond(twbCond);
}

double PseudotreeNode::computeStatsClusterCondSub(
    const set<int>& cond, vector<int>& result) const {
  set<int> context_cond;
  set_difference(m_contextS.begin(), m_contextS.end(), cond.begin(), cond.end(),
                 inserter(context_cond, context_cond.begin()));
  double twb = m_domain;
  BOOST_FOREACH(int v, context_cond) {
    twb *= m_tree->getNode(v)->getDomain();
  }
  result.push_back(context_cond.size());
  BOOST_FOREACH( PseudotreeNode* pt, m_children ) {
    twb += pt->computeStatsClusterCondSub(cond, result);
  }
  return twb;
}

#endif


#if defined PARALLEL_DYNAMIC || defined PARALLEL_STATIC
/* computes subproblem complexity parameters for a particular pseudo tree node */
void PseudotreeNode::initSubproblemComplexity() {

  const vector<val_t>& domains = m_tree->m_problem->getDomains();

  queue<PseudotreeNode*> Q;
  Q.push(this);

  int w = NONE;
  bigint s = 0;
  PseudotreeNode* node = NULL;

  const vector<int>& ctxt = this->getFullContextVec();

  while (!Q.empty()) {
    node = Q.front();
    Q.pop();

    for (vector<PseudotreeNode*>::const_iterator it=node->getChildren().begin();
         it!=node->getChildren().end(); ++it)
      Q.push(*it);

    // compute parameters for current node
    int w2 = 0;
    bigint s2 = domains.at(node->getVar()); // own var is NOT in context
    const vector<int>& fullCtxt = node->getFullContextVec();
    vector<int>::const_iterator itFC = fullCtxt.begin(), itC = ctxt.begin();
    while (itFC != fullCtxt.end() && itC != ctxt.end()) {
      if (*itFC == *itC) {
        ++itFC; ++itC;
      } else if (*itFC < *itC) {
        ++w2; s2 *= domains.at(*itFC); ++itFC;
      } else { // *ita > *itb
        ++itC;
      }
    }
    while (itFC != fullCtxt.end()) {
      ++w2; s2 *= domains.at(*itFC); ++itFC;
    }
    if (w2>w) w=w2; // update max. induced width
    s += s2; // add cluster size to subproblem size

  }

  // compute own cluster size only
  bigint ctxtsize = 1;
  for (vector<int>::const_iterator itb = ctxt.begin(); itb!=ctxt.end(); ++itb)
    ctxtsize *= domains.at(*itb); // context variable

  // multiply by own domain size for cluster size
  bigint own = ctxtsize*domains.at(this->getVar());

  m_complexity = new Complexity(w,s,own,ctxtsize);

#ifdef DEBUG
//  cout << "Subproblem at node " << m_var << ": w = " << w << ", twb = " << s << endl;
#endif

}
#endif /* PARALLEL */


#if defined PARALLEL_DYNAMIC || defined PARALLEL_STATIC
/* compute upper bound on subproblem size, assuming conditioning on 'cond' */
bigint PseudotreeNode::computeSubCompDet(const set<int>& cond, const vector<val_t>* assig) const {

  set<int> cluster(m_contextS); // get context
  cluster.insert(m_var); // add own variable to get cluster
  const vector<val_t>& doms = m_tree->m_problem->getDomains();

//  DIAG( cout << "Covering cluster " << cluster << " conditioned on " << cond << endl );

  // compute locally relevant set of variables, i.e. context minus instantiated vars in 'cond'
  set<int> vars;
  set_difference(cluster.begin(), cluster.end(), cond.begin(), cond.end(), inserter(vars,vars.begin()) );

  // collect all relevant functions by going up in the PTree
  vector<Function*> funcs( this->getFunctions() );
  for (PseudotreeNode* node = this->getParent(); node; node = node->getParent() )
    funcs.insert( funcs.end(), node->getFunctions().begin(), node->getFunctions().end() );

#ifdef DEBUG
  cout << " Functions : " ;
  for (list<Function*>::iterator itFu=funcs.begin(); itFu!= funcs.end(); ++itFu)
    cout << (*(*itFu)) << ", " ;
  cout << endl;
#endif

  // TODO filter functions by scope

  set<int> uncovered(vars); // everything uncovered initially
  list<Function*> cover;
  while (!uncovered.empty()) {
    bigfloat ratio = 1;//, ctemp = 1;
    Function* cand = NULL;
    vector<Function*>::iterator candIt = funcs.begin();

//    DIAG(cout << "  Uncovered: " << uncovered << endl);

    for (vector<Function*>::iterator itF = funcs.begin(); itF!= funcs.end(); ++itF) {
      bigfloat rtemp = (*itF)->gainRatio(uncovered, cluster, cond, assig);
//      DIAG( cout << "   Ratio for " << (*(*itF)) <<": " << rtemp << endl );
      if (rtemp != UNKNOWN && rtemp < ratio)
        ratio = rtemp, cand = *itF, candIt = itF;
    }

    if (!cand) break; // covering is not complete but can't be improved

    // add function to covering and remove its scope from uncovered set
    cover.push_back(cand);
    funcs.erase(candIt);
#ifdef DEBUG
    cout << "  Adding " << (*cand) << endl;
#endif
    const vector<int>& cscope = cand->getScopeVec();
    for (vector<int>::const_iterator itSc = cscope.begin(); itSc!=cscope.end(); ++itSc)
      uncovered.erase(*itSc);

  }

  // compute upper bound based on clustering
  bigint bound = 1;
  for (set<int>::iterator itU=uncovered.begin(); itU!=uncovered.end(); ++itU)
    bound *= doms.at(*itU); // domain sizes of uncovered variables
  for (list<Function*>::iterator itC=cover.begin(); itC!=cover.end(); ++itC)
    bound *= (*itC)->getTightness(cluster,cond,assig); // projected function tightness

#ifdef DEBUG
  cout << "@ " << bound << endl;
#endif

  // add bounds from child nodes
  for (vector<PseudotreeNode*>::const_iterator itCh=m_children.begin(); itCh!=m_children.end(); ++itCh)
    bound += (*itCh)->computeSubCompDet(cond, assig);

  return bound;

}
#endif /* PARALLEL */


void Pseudotree::insertNewNode(const int i, const set<int>& N, list<PseudotreeNode*>& roots) {
  // create new node in pseudo tree
  PseudotreeNode* p = new PseudotreeNode(this,i,N);
//  p->addContext(i);
  m_nodes[i] = p;

  // incorporate new pseudo tree node
  list<PseudotreeNode*>::iterator it = roots.begin();
  while (it != roots.end()) {
    const set<int>& context = (*it)->m_contextS;
    if (context.find(i) != context.end()) {
      p->addChild(*it); // add child to current node
      (*it)->setParent(p); // set parent of previous node
      it = roots.erase(it); // remove previous node from roots list
    } else {
      ++it;
    }
  }
  roots.push_back(p); // add current node to roots list

}


void Pseudotree::outputToFileNode(const PseudotreeNode* node, ostringstream& oss) const {
  oss << "(" << node->getVar();
  for (vector<PseudotreeNode*>::const_iterator it = node->getChildren().begin();
       it != node->getChildren().end(); ++it)
    outputToFileNode(*it, oss);
  oss << ")";
}


void Pseudotree::outputToFile(string of_name) const {
  assert(m_root);
  ostringstream oss;
  outputToFileNode(m_root, oss);  // recursive, call on root node

  ofstream of;
  of.open(of_name.c_str(), ios_base::out);
  of << oss.str() << endl;
  of.close();
}

}  // namespace daoopt
