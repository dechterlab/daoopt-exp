/*
 * Statistics.cpp
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
 *  Created on: Mar 2, 2010
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#include "Statistics.h"

#include "utils.h"

#ifdef PARALLEL_STATIC

namespace daoopt {

ostream& operator << (ostream& os, const SubproblemStats& s) {
  os << NONE << '\t';  // subprob id
  os << s.rootVar << '\t';
  os << s.depth << '\t';
  os << s.numVars << '\t';
  os << s.lowerBound << '\t';
  os << s.upperBound << '\t';
  os << s.height << '\t';
  os << s.width << '\t';
  os << UNKNOWN << '\t';  // # OR nodes
  os << s.subNodeCount << '\t';
  os << UNKNOWN << '\t';  // time
  os << UNKNOWN << '\t';  // avgD
  os << UNKNOWN << '\t';  // metric

  return os;
}

}  // namespace daoopt

#endif

#ifdef PARALLEL_DYNAMIC

#define FALLOFF 0.9

namespace daoopt {

void AvgStatistics::init(int depth, int height, count_t N, count_t L, count_t D, double lower, double upper) {

  cout << "Statistics::init(" << depth <<','<< N <<','<< L <<','<< D <<','<< lower <<','<< upper <<")"<< endl;

  ostringstream ss;
  ss << "Statistics initialized using " << N << " nodes,";

  /*
  // avg. leaf depth first (using full node profile!)
  count_t leafs = 0;
  for (size_t i=0 ; i<leafProf.size(); ++i) {
    leafs += leafProf.at(i);
  }

  double avgD = 0.0; size_t h=0;
  for (size_t i=0; i<leafProf.size(); ++i, ++h) {
    avgD += (leafProf.at(i))*h*1.0/leafs;
  }
  avgD -= depth;
  */

  defHei = height - 1 ; // to artificially reduce first set of subproblems  TODO?
  ss << " h:" << defHei;

  defDep = D/(double)L;
  ss << " avgD:" << defDep;

  // compute avg. increment
  double inc = pow(upper - lower, _alpha);
  inc *= pow(height, _beta);
  inc /= defDep;
  defInc = inc * .9; // .9 to be cautious  TODO?
  ss << " inc:" << defInc;

  // branching factor
  double br = pow(N, 1.0/ defDep);
  defBra = br;
  ss << " br:" << br;

  ss << endl;
  myprint(ss.str());

}


/* !! ATTENTION: caller should own mutex on mtx_space !! */
void AvgStatistics::addSubprob(Subproblem* subp) {

  assert(subp && subp->isSolved());

  ostringstream ss;
  ss << "Recorded subproblem " << subp->threadId << ": ";

  // record actual number of nodes
  nodesAND.push_back(subp->nodesAND);
  ss << subp->nodesAND << " / ";

  // record precomputed estimate
  estimate.push_back(subp->estimate);
  ss << subp->estimate;

  // record sub pseudotree height
  height.push_back(subp->ptHeight);
  ss << " h:" << subp->ptHeight;

  // compute average leaf depth;
  count_t leafs = 0;
  vector<count_t>::const_iterator it;
  for (it=subp->leafP.begin(); it!=subp->leafP.end(); ++it)
    leafs += *it;

  double avgD = 0.0; size_t h=0;
  for (it=subp->leafP.begin(); it!=subp->leafP.end(); ++it, ++h)
    avgD += (*it)*h*1.0 /leafs;

  avgLeafDepth.push_back(avgD);
  ss << " avgD:" << avgD;

  // compute the avg. increment
  if (avgD != 0.0 && subp->lowerBound != ELEM_ZERO) {
    double inc = pow(subp->upperBound - subp->lowerBound, _alpha);
    inc *= pow(subp->ptHeight, _beta);
    inc /= avgD;
    increment.push_back(inc);
    ss << " inc:" << inc;
  }

  // compute the branching factor
  if (avgD != 0.0) {
    double br = pow(subp->nodesAND, 1.0/ avgD);
    branching.push_back(br);
    ss << " br:" << br;
  }

  _minN = min((count_t)(_minN/FALLOFF),subp->nodesAND)-1;
  _maxN = max((count_t)(_maxN*FALLOFF),subp->nodesAND)+1;

  _minE = min((count_t)(_minE/FALLOFF),subp->estimate)-1;
  _maxE = max((count_t)(_maxE*FALLOFF),subp->estimate)+1;

  ss << endl;
  myprint(ss.str());

}

double AvgStatistics::getAvgInc() {
  if (increment.empty()) {
    if (defInc==NONE)
      assert(false);
    else
      return defInc;
  }

  double all = ELEM_ONE;
  for (vector<double>::const_iterator it=increment.begin(); it!=increment.end(); ++it)
    all OP_TIMESEQ *it ;

#ifdef USE_LOG
  return all / increment.size();
#else
  return pow(all, 1.0/increment.size());
#endif

}


double AvgStatistics::getAvgBra() {
  if (branching.empty()) {
    if (defBra==NONE)
      assert(false);
    else
      return defBra;
  }

  double all = ELEM_ONE;
  for (vector<double>::const_iterator it=branching.begin(); it!=branching.end(); ++it)
    all OP_TIMESEQ *it ;

#ifdef USE_LOG
  return all / branching.size();
#else
  return pow(all, 1.0/branching.size());
#endif

}


double AvgStatistics::getAvgHei() {
  if (height.empty()) {
    if (defHei==NONE)
      assert(false);
    else
      return defHei;
  }

  double all = 0.0;
  for (vector<int>::const_iterator it=height.begin(); it!=height.end(); ++it)
    all += *it;
  return all/height.size();
}


double AvgStatistics::normalize(double d) const {

  if (branching.size() < 2 || increment.size() < 2)
    return d;

  double norm = d - _minE;
  norm /= (_maxE - _minE);
  norm *= (_maxN - _minN);
  norm += _minN ;

  return max(0.0,norm);

}

}  // namespace daoopt

#endif /* PARALLEL_DYNAMIC */

