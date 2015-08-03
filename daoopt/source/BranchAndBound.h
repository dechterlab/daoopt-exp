/*
 * BranchAndBound.h
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
 *  Created on: Nov 3, 2008
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#ifndef BRANCHANDBOUND_H_
#define BRANCHANDBOUND_H_

#include "Search.h"

namespace daoopt {

/* Branch and Bound search, implements pure virtual functions
 * from Search.h */
class BranchAndBound : virtual public Search {

protected:
#ifdef ANYTIME_DEPTH
  stack<SearchNode*> m_stackDive; // first stack for initial dives
#endif
  stack<SearchNode*> m_stack; // The DFS stack of nodes

protected:
  bool isDone() const;
  bool doCompleteProcessing(SearchNode* n);
  bool doExpand(SearchNode* n);
  SearchNode* nextNode();
  bool isMaster() const { return false; }

public:
  void reset(SearchNode* p = NULL);
  bool solve(size_t nodeLimit);
  BranchAndBound(Problem* prob, Pseudotree* pt, SearchSpace* space,
      Heuristic* heur, BoundPropagator* prop, ProgramOptions*po) ;
  virtual ~BranchAndBound() {}
};


/* Inline definitions */


inline bool BranchAndBound::isDone() const {
#if defined ANYTIME_DEPTH
  return (m_stack.empty() && m_stackDive.empty());
#else
  return m_stack.empty();
#endif
}

}  // namespace daoopt

#endif /* BRANCHANDBOUND_H_ */
