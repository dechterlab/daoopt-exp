/*
 * Main.h
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
 *  Created on: Oct 18, 2011
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#ifndef MAIN_H_
#define MAIN_H_

#include "_base.h"

#include "FGLPHeuristic.h"
#include "FGLPMBEHybrid.h"
#include "Function.h"
#include "Graph.h"
#include "MiniBucketElim.h"
#include "MiniBucketElimLH.h"
#include "Problem.h"
#include "ProgramOptions.h"
#include "Pseudotree.h"
#ifdef ENABLE_SLS
#include "SLSWrapper.h"
#endif

#ifdef PARALLEL_DYNAMIC
#include "BranchAndBoundMaster.h"
#include "BoundPropagatorMaster.h"
#include "SubproblemHandler.h"
#include "SigHandler.h"
#else
#ifdef PARALLEL_STATIC
#include "ParallelManager.h"
#include "BranchAndBoundSampler.h"
#endif
#include "BranchAndBound.h"
#include "BranchAndBoundRotate.h"
#include "BoundPropagator.h"
#endif

//#include "BestFirst.h"
#include "LimitedDiscrepancy.h"

namespace daoopt {

class Main {
 protected:
  bool m_solved;
  bool m_started;
  scoped_ptr<ProgramOptions> m_options;
  scoped_ptr<Problem> m_problem;
  scoped_ptr<Pseudotree> m_pseudotree;
  std::unique_ptr<Heuristic> m_heuristic;
#ifdef ENABLE_SLS
  scoped_ptr<SLSWrapper> m_slsWrapper;
#endif

#if defined PARALLEL_DYNAMIC
  scoped_ptr<BranchAndBoundMaster> m_search;
  scoped_ptr<SearchSpaceMaster> m_space;
#else
#ifdef PARALLEL_STATIC
  scoped_ptr<ParallelManager> m_search;
#else
  scoped_ptr<Search> m_search;
#endif
  scoped_ptr<SearchSpace> m_space;
#endif
  scoped_ptr<BoundPropagator> m_prop;

 protected:
  bool runSearchDynamic();
  bool runSearchStatic();
  bool runSearchWorker(size_t nodeLimit = 0);

  static Heuristic* newHeuristic(Problem* p, Pseudotree* pt,
      ProgramOptions* po);

  double evaluate(SearchNode* node) const;

 public:
  bool start() const;
  bool parseOptions(int argc, char** argv);
  bool setOptions(const ProgramOptions& options);
  bool setSLSOptions(int slsIter, int slsTimePerIter);
  bool outputInfo() const;
  bool loadProblem();
  bool findOrLoadOrdering();
  bool runSLS();
  bool stopSLS();
  bool initDataStructs();
  bool preprocessHeuristic();
  bool compileHeuristic();
  bool runLDS();
  bool finishPreproc();
  bool runSearch(size_t nodeLimit = 0);
  bool outputStats() const;
  int outputStatsToFile() const;

  bool isSolved() const { return m_solved; }

  double getSolution() const;
  const vector<val_t>& getSolutionAssg() const;
  void getSolutionAssgOrg(vector<val_t> &) const; // get solution assignment in terms of original problem specification, with evid added back in.

  double runEstimation(size_t nodeLimit = 0);

  inline Heuristic* getHeuristic() { return m_heuristic.get(); }

  Main();
};

/* Inline implementations */

inline Main::Main() : m_solved(false), m_started(false) { /* nothing here */ }

inline bool Main::runSearch(size_t nodeLimit) {
  if (m_options->nosearch) {
    return true;
  }
  if (m_solved) {
    return true;
  }
  if (!m_started) {
    cout << "--- Starting search ---" << endl;
    m_started = true;
  }
#if defined PARALLEL_DYNAMIC
  return runSearchDynamic();
#elif defined PARALLEL_STATIC
  return runSearchStatic();
#else
  return runSearchWorker(nodeLimit);
#endif
}

}  // namespace daoopt

#endif /* MAIN_H_ */
