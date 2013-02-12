/*
 * Pseudotree.h
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

#ifndef PSEUDOTREE_H_
#define PSEUDOTREE_H_

#include <float.h>

#include "_base.h"
#include "Graph.h"
#include "Function.h"
#include "Problem.h"

#ifdef PARALLEL_STATIC
#include "SubprobStats.h"
#endif

/* forward declaration */
class PseudotreeNode;


class Pseudotree {

  friend class PseudotreeNode;

protected:

  int m_subOrder;

  int m_height;
  int m_heightConditioned;
  int m_width;
  int m_widthConditioned;
  int m_components;
  int m_size;
  int m_sizeConditioned;

  Problem* m_problem;
  PseudotreeNode* m_root;

  // for minfill
  vector<nCost> m_initialScores;

  vector<PseudotreeNode*> m_nodes;
  vector<int> m_elimOrder;
#if defined PARALLEL_DYNAMIC || defined PARALLEL_STATIC
  vector<list<PseudotreeNode*> > m_levels;
#endif

public:
  /* computes an elimination order into 'elim' and returns its induced width
   * if 'limit' is given, will terminate early if new order is worse than limit
   * and return INT_MAX */
  int eliminate(Graph G, vector<int>& elim, int limit=INT_MAX, int tolerance=0);

  /* builds the pseudo tree according to order 'elim' */
  void build(Graph G, const vector<int>& elim, const int cachelimit = NONE);
  /* builds the pseudo tree as a *chain* according to order 'elim' */
  void buildChain(Graph G, const vector<int>& elim, const int cachelimit = NONE);

  /* returns the width of the (sub) pseudo tree */
  int getWidth() const { return m_width; }
  int getWidthCond() const { if (m_widthConditioned == NONE) return m_width; else return m_widthConditioned; };

  /* returns the height of the (sub) pseudo tree */
  int getHeight() const { return m_height; }
  int getHeightCond() const { if (m_heightConditioned == NONE ) return m_height; else return m_heightConditioned; }

  /* returns the number of nodes in the (sub) pseudo tree */
  int getSize() const { return m_size; }
  int getSizeCond() const { if (m_sizeConditioned == NONE) return m_size; else return m_sizeConditioned;}

  /* returns the number of nodes in the *full* pseudo tree */
  size_t getN() const { return m_nodes.size(); }

  /* returns the number of components */
  int getComponents() const { return m_components; }
  int getComponentsCond() const;

  /* returns the root node */
  PseudotreeNode* getRoot() const { return m_root; }

  /* returns the pseudotree node for variable i */
  PseudotreeNode* getNode (int i) const;

  const vector<int>& getElimOrder() const { return m_elimOrder; }

  /* augments the pseudo tree with function information */
  void addFunctionInfo(const vector<Function*>& fns);

  /* adds domain size info to pseudo tree nodes */
  void addDomainInfo(const vector<val_t>& domains);

#if defined PARALLEL_DYNAMIC || defined PARALLEL_STATIC
  /* computes the subproblem complexity parameters for all subtrees */
  int computeComplexities(int workers);
#endif

#ifdef PARALLEL_STATIC
  void computeSubprobStats();
  double getStateSpaceCond() const;
#endif

  const vector<Function*>& getFunctions(int i) const;

  /* restricts to a subproblem rooted at variable i, returns the depth
   * of var i in the overall pseudo tree */
  int restrictSubproblem(int i);

private:
  /* recursive helper function for outputToFile(...) below */
  void outputToFileNode(const PseudotreeNode*, ostringstream&) const;
public:
  /* outputs the pseudo tree structure to a file in ASCII format, to
   * be used as input to a plotting script, for instance */
  void outputToFile(string basename) const;

protected:
  /* creates a new node in the PT for variable i, with context N. Also makes sure
   * existing roots are checked and connected appropriately. */
  void insertNewNode(const int i, const set<int>& N, list<PseudotreeNode*>& roots);

  // "clears out" the pseudo tree by resetting parameters and removing the node structure
  void reset();

public:
  Pseudotree(Problem* p, int subOrder);
  Pseudotree(const Pseudotree& pt); // copy constructor
  ~Pseudotree();
};


/* represents a single problem variable in the pseudotree */
class PseudotreeNode {

  friend class Pseudotree;

#if defined PARALLEL_DYNAMIC || defined PARALLEL_STATIC
protected:
  /* internal container class for subproblem complexity information */
  class Complexity {
    /*
     * If X is the current variable, then:
     * subwidth - is the induced width of the subproblem below, when conditioned
     *     on context(X).
     * subsize - is an upper bound on the size of the subproblem (in number of
     *     AND nodes) below *one* OR node for variable X in the search space.
     * ownsize - is the full cluster size of X, i.e. the product of its domain
     *     size with the domain sizes of a variables in context(X).
     * numContexts - is the number of max. possible context instantiations for X,
     *     i.e. the product of the domain sizes of the variables in context(X)
     */
  public:
    int subCondWidth; // Induced width of subproblem if conditioned on this variable's context
    bigint subCondBound; // Upper bound on subproblem size below one OR node if conditioned on context
    bigint ownsize; // Own cluster size, i.e. product of cluster variable domain sizes
    bigint numContexts; // Number of possible context instantiations
  public:
    Complexity() : subCondWidth(UNKNOWN), subCondBound(UNKNOWN), ownsize(UNKNOWN), numContexts(UNKNOWN) {}
    Complexity(int i, bigint b, bigint o, bigint n) : subCondWidth(i), subCondBound(b), ownsize(o), numContexts(n) {}
    Complexity(const Complexity& c) : subCondWidth(c.subCondWidth), subCondBound(c.subCondBound), ownsize(c.ownsize), numContexts(c.numContexts) {}
  };
#endif

protected:
  val_t m_domain; // The node's variable's domain size
  int m_var; // The node variable
  int m_depth; // The node's depth in the tree
  int m_subHeight; // The node's height in the tree (distance from farthest child)
  int m_subWidth; // max. width in subproblem
  PseudotreeNode* m_parent; // The parent node
  Pseudotree* m_tree;
#if defined PARALLEL_DYNAMIC || defined PARALLEL_STATIC
  Complexity* m_complexity; // Contains information about subproblem complexity
#endif
#ifdef PARALLEL_STATIC
  SubprobStats* m_subprobStats;
#endif
  vector<int> m_subproblemVars; // The variables in the subproblem (including self)
  vector<int> m_subproblemVarMap; // Maps variables to their index in subprob assignment
  // Maintain contexts as both set and vector -- speedup is worth space overhead.
  set<int> m_contextS; // The node's full OR context (!doesn't include own variable!)
  vector<int> m_contextV; // OR context as vector
  set<int> m_cacheContextS; // The (possibly smaller) context for (adaptive) caching
  vector<int> m_cacheContextV; // caching context as vector
  list<int> m_cacheResetList; // List of var's whose cache tables need to be reset when this
                              // var's search node is expanded (for adaptive caching)
  vector<Function*> m_functions; // The functions that will be fully instantiated at this point
  vector<PseudotreeNode*> m_children; // The node's children

public:

  void setParent(PseudotreeNode* p) { m_parent = p; }
  PseudotreeNode* getParent() const { return m_parent; }

  void addChild(PseudotreeNode* p) { m_children.push_back(p); }
  void setChild(PseudotreeNode* p) { m_children.clear(); m_children.push_back(p); }
  const vector<PseudotreeNode*>& getChildren() const { return m_children; }
  void orderChildren(int subOrder);

  static bool compGreater(PseudotreeNode* a, PseudotreeNode* b);
  static bool compLess(PseudotreeNode* a, PseudotreeNode* b);

  void setFullContext(const set<int>& c);
  const set<int>& getFullContext() const { return m_contextS; }
  const vector<int>& getFullContextVec() const { return m_contextV; }

  void setCacheContext(const set<int>& c);
  const vector<int>& getCacheContextVec() const { return m_cacheContextV; }

  void setCacheReset(const list<int>& l) { m_cacheResetList = l; }
  void addCacheReset(int i) { m_cacheResetList.push_back(i); }
  const list<int>& getCacheReset() const { return m_cacheResetList; }

  void addFunction(Function* f) { m_functions.push_back(f); }
  void setFunctions(const vector<Function*>& l) { m_functions = l; }
  void resetFunctions() { m_functions.clear(); }
  const vector<Function*>& getFunctions() const { return m_functions; }

  void setDomain(val_t d) { m_domain = d; }
  val_t getDomain() const { return m_domain; }

  int getVar() const { return m_var; }
  int getDepth() const { return m_depth; }
  int getSubHeight() const { return m_subHeight; }
  int getSubWidth() const { return m_subWidth; }

  size_t getSubprobSize() const { return m_subproblemVars.size(); }
  const vector<int>& getSubprobVars() const { return m_subproblemVars; }
  const vector<int>& getSubprobVarMap() const { return m_subproblemVarMap; }
  void setSubprobVarMap(const vector<int>& map) { m_subproblemVarMap = map; }

public:
#if defined PARALLEL_DYNAMIC || defined PARALLEL_STATIC
  int getSubCondWidth() const { assert(m_complexity); return m_complexity->subCondWidth; }
  bigint getSubCondBound() const { assert(m_complexity); return m_complexity->subCondBound; }
  bigint getOwnsize() const { assert(m_complexity); return m_complexity->ownsize; }
  bigint getNumContexts() const { assert(m_complexity); return m_complexity->numContexts; }
#endif

#if defined PARALLEL_DYNAMIC || defined PARALLEL_STATIC
  void initSubproblemComplexity();
#endif
  const vector<int>& updateSubprobVars(int numVars);
  int updateDepthHeight(int d);
  int updateSubWidth();

#ifdef PARALLEL_STATIC
public:
  const SubprobStats* getSubprobStats() const { return m_subprobStats; }
  void computeStatsCluster(vector<int>&);
  void computeStatsLeafDepth(vector<int>&);
  void computeStatsDomain(vector<int>&);
  void computeStatsClusterCond();
protected:
  /* appends cluster sizes of all descendants, conditioned on set.
   * Also computes and returns conditioned subproblem state space. */
  double computeStatsClusterCondSub(const set<int>&, vector<int>&) const;
#endif

#if defined PARALLEL_DYNAMIC || defined PARALLEL_STATIC
public: // TODO protected:
  /* computes an estimate for the subproblem under this node, assuming the context
   * is instantiated as given in assig. Exploits function determinism.
   * (essentially just a wrapper around computeSubCompDet(...) ) */
  bigint computeHWB(const vector<val_t>* assig);

protected:
  /* compute bounds on subproblem complexity, assuming conditioning on C and assignment
   * to the vars in 'cond' as given in assig */
  bigint computeSubCompDet(const set<int>& cond, const vector<val_t>* assig = NULL ) const;
#endif

public:
  /* creates new pseudo tree node for variable v with context s */
  PseudotreeNode(Pseudotree* t, int v, const set<int>& s);
  ~PseudotreeNode();
};


/* Inline definitions */

inline PseudotreeNode* Pseudotree::getNode(int i) const {
  assert (i<(int)m_nodes.size());
  return m_nodes[i];
}

inline const vector<Function*>& Pseudotree::getFunctions(int i) const {
  assert (i < (int) m_nodes.size());
  return m_nodes[i]->getFunctions();
}

inline int Pseudotree::getComponentsCond() const  {
  return (int) m_root->getChildren().size();
}

inline void Pseudotree::reset() {
  assert(m_problem);
  m_height = UNKNOWN; m_heightConditioned = UNKNOWN;
  m_width = UNKNOWN; m_widthConditioned = UNKNOWN;
  m_components = 0;
  m_size = UNKNOWN; m_sizeConditioned = UNKNOWN;
  m_root = NULL;
  for (vector<PseudotreeNode*>::iterator it=m_nodes.begin(); it!=m_nodes.end(); ++it) {
    delete *it;
    *it = NULL;
  }
  m_nodes.clear();
  m_nodes.reserve(m_problem->getN()+1); // +1 for dummy variable
  m_nodes.resize(m_problem->getN(), NULL);
  m_size = m_problem->getN();
}

inline Pseudotree::Pseudotree(Problem* p, int subOrder) :
    m_subOrder(subOrder),
    m_height(UNKNOWN), m_heightConditioned(UNKNOWN),
    m_width(UNKNOWN), m_widthConditioned(UNKNOWN),
    m_components(0),
    m_size(UNKNOWN), m_sizeConditioned(UNKNOWN),
    m_problem(p), m_root(NULL)
{
  assert(p);
  m_nodes.reserve(p->getN()+1); // +1 for bogus node
  m_nodes.resize(p->getN(), NULL);
  m_size = p->getN();
}

inline Pseudotree::~Pseudotree() {
  for (vector<PseudotreeNode*>::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it) {
    if (*it) delete *it;
  }
}

/* PseudotreeNode inlines */


inline void PseudotreeNode::setFullContext(const set<int>& c) {
  m_contextS = c;
  vector<int> newV(c.begin(), c.end());
  m_contextV.swap(newV);
}

inline void PseudotreeNode::setCacheContext(const set<int>& c) {
  m_cacheContextS = c;
  vector<int> newV(c.begin(), c.end());
  m_cacheContextV.swap(newV);
}

#if defined PARALLEL_DYNAMIC || defined PARALLEL_STATIC
inline bigint PseudotreeNode::computeHWB(const vector<val_t>* assig) {
  return computeSubCompDet(m_contextS, assig);
}
#endif /* PARALLEL MODE */


/* Constructor */
inline PseudotreeNode::PseudotreeNode(Pseudotree* t, int v, const set<int>& s) :
#if defined PARALLEL_DYNAMIC
  m_domain(UNKNOWN), m_var(v), m_depth(UNKNOWN), m_subHeight(UNKNOWN), m_parent(NULL), m_tree(t), m_complexity(NULL),
  m_contextS(s), m_contextV(s.begin(), s.end()) {}
#elif defined PARALLEL_STATIC
  m_domain(UNKNOWN), m_var(v), m_depth(UNKNOWN), m_subHeight(UNKNOWN), m_parent(NULL), m_tree(t), m_complexity(NULL), m_subprobStats(NULL),
  m_contextS(s), m_contextV(s.begin(), s.end())
{
  m_subprobStats = new SubprobStats();
}
#else
  m_domain(UNKNOWN), m_var(v), m_depth(UNKNOWN), m_subHeight(UNKNOWN), m_parent(NULL), m_tree(t),
  m_contextS(s), m_contextV(s.begin(), s.end()) {}
#endif

inline PseudotreeNode::~PseudotreeNode()  {
#if defined PARALLEL_DYNAMIC || defined PARALLEL_STATIC
    if (m_complexity) delete m_complexity;
#endif
#ifdef PARALLEL_STATIC
    if (m_subprobStats) delete m_subprobStats;
#endif
}

#endif /* PSEUDOTREE_H_ */
