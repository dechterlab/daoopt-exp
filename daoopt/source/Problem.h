/*
 * Problem.h
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
 *  Created on: Oct 10, 2008
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#ifndef PROBLEM_H_
#define PROBLEM_H_

#include "Function.h"
#include "Graph.h"
#include "MiniBucket.h"
#include "SearchSpace.h"
#include "_base.h"
#include "gzstream.h"

namespace daoopt {

class SearchStats;

/* holds a problem instance with variable domains and function tables */
class Problem {

protected:

  bool m_is_copy;        // true iff object instance is copy

  bool m_subprobOnly;    // Solving only a conditioned subproblem
  bool m_hasDummy;       // is last variable a dummy variable?

  int m_prob;            // Problem class (multiplication or summation of costs)
  int m_task;            // Type of problem (Minim. or maxim. task)


  int m_n;               // No. of variables
  int m_nOrg;            // No. of variables (before evidence was removed)
  val_t m_k;             // Max. domain size
  int m_e;               // No. of evidence

  int m_c;               // No. of functions
  int m_r;               // Max. function arity

  double m_globalConstant;        // Global constant modifier for objective function

  double m_curCost;               // Cost of current solution
  double m_curUpperBound;            // Current upper bound

  string m_name;                  // Problem name

  vector<val_t> m_domains;        // Domain sizes of variables

  vector<Function*> m_functions;  // List of functions

  map<int,val_t> m_evidence;      // List of evidence as <index,value>

  map<int,int> m_old2new;         // Translation of variable names after removing evidence

  vector<val_t> m_curSolution;       // Current best solution

  unsigned int num_zero_tuples_;  // Number of zero tuples
  unsigned int num_tuples_;  // Number of tuples
  double determinism_ratio_; // ratio of above two numbers

public:
  void setCopy(bool isCopy = true) { m_is_copy = isCopy; }

  val_t getDomainSize(int i) const;
  double globalConstInfo() const;

  int getN() const { return m_n; }
  int getNOrg() const { return m_nOrg; }
  val_t getK() const { return m_k; }
  int getE() const { return m_e; }
  int getC() const { return m_c; }
  int getR() const { return m_r; }

  void setSubprobOnly() { m_subprobOnly = true; }
  const string& getName() const { return m_name; }

  const vector<Function*>& getFunctions() const { return m_functions; }
  const vector<val_t>& getDomains() const { return m_domains; }

  // replaces the current set of functions with an equivalent one
  // (pseudo tree compatibility is implicitly assumed)
  void replaceFunctions(const vector<Function*>& newFunctions, bool asCopy=false);

  bool hasDummy() const { return m_hasDummy; }

  size_t getSize() const;

public:

  /* parses a UAI format input file */
  bool parseUAI16(char* prob, size_t probN, char* evid, size_t evidN,
                bool collapse=false);
  bool parseUAI(char* prob, size_t probN, char* evid, size_t evidN,
                bool collapse=false);
//  bool parseUAI(const string& prob, const string& evid, bool collapse=false);

  /* writes the current problem to a UAI file */
  void writeUAI(const string& prob) const;

  /* parses an ordering from file 'file' and stores it in 'elim' */
  bool parseOrdering(const string& file, vector<int>& elim) const;
  bool parseOrdering(const vector<int>& input, vector<int>& elim) const;
  /* stores ordering from 'elim' in file 'file' */
  void saveOrdering(const string& file, const vector<int>& elim) const;

  /* removes evidence and unary-domain variables */
  void removeEvidence(bool clearEvid=false);

  /* combines factors with the same scope */
  void collapseFunctions();

  void perturbDeterminism(double epsilon);

  /* condition problem */
  /* note that this does not re-index the variables */
  void condition(const map<int,val_t> &cond);



  /* retrieve the current optimal solution */
  double getSolutionCost() const { return m_curCost; }

  /* retreive the current global upper bound */
  double getUpperBound() const { return m_curUpperBound; }

#ifndef NO_ASSIGNMENT
  /* retrieve the current optimal assignment */
  const vector<val_t>& getSolutionAssg() const { return m_curSolution; }
  /* compute the assignment for output (might add evidence back in) */
  void assignmentForOutput(vector<val_t>&) const;
  void assignmentForOutput(const vector<val_t>& in, vector<val_t>& out) const;
#endif

  /* report an updated solution */
  void updateSolution(double cost,
#ifndef NO_ASSIGNMENT
      const vector<val_t>& sol,
#endif
      const SearchStats* nodestats = NULL,
      bool output = true);

  /* report an updated upper bound */
  void updateUpperBound(double bound, const SearchStats* nodestats = NULL,
      bool output = true);
  
  /* report both lower-upper bound */
  void updateLowerUpperBound(double cost, double bound,
                             const SearchStats* nodestats = NULL,
                             bool output = true);


  /* resets current optimal solution cost and assignment */
  void resetSolution();

  /* outputs the solution to the screen and, if file!="", writes it to file
   * (for subproblem solving, only relevant variables will be output)
   *  - cost is the MPE tuple value
   *  - sol is the optimal solution tuple
   *  - noNodes is the number of OR/AND nodes
   *  - nodeProf and leafProf are the full and leaf node profiles
   *  - if toScreen==true, will skip the console output (file only)
   */
  void outputAndSaveSolution(const string& file, const SearchStats* nodestats,
                             const vector<count_t>& nodeProf, const vector<count_t>& leafProf,
                             bool toScreen = true) const;

#ifndef NO_ASSIGNMENT
  /* returns true iff the index variable from the full set has been eliminated
   * as evidence or unary */
  bool isEliminated(int i) const;
#endif

  /* adds the dummy variable to connect disconnected pseudo tree components */
  void addDummy();

public:
  Problem();
  Problem(Problem *p);
  virtual ~Problem();
};


/* Inline definitions */

inline val_t Problem::getDomainSize(int i) const {
  assert (i<m_n);
  return m_domains[i];
}

inline double Problem::globalConstInfo() const {
  return m_globalConstant;
}

inline Problem::Problem() :
    m_is_copy(false),
    m_subprobOnly(false),
    m_hasDummy(false),
    m_prob(UNKNOWN),
    m_task(UNKNOWN),
    m_n(UNKNOWN),
    m_nOrg(UNKNOWN),
    m_k(UNKNOWN),
    m_e(UNKNOWN),
    m_c(UNKNOWN),
    m_r(UNKNOWN),
    m_globalConstant(ELEM_NAN),
    m_curCost(ELEM_NAN),
    m_curUpperBound(ELEM_NAN)
{ /* empty*/ }

// Functions are owned by the copy of the problem
inline Problem::Problem(Problem *p) :
    m_is_copy(false),
    m_subprobOnly(p->m_subprobOnly),
    m_hasDummy(p->m_hasDummy),
    m_prob(p->m_prob),
    m_task(p->m_task),
    m_n(p->m_n),
    m_nOrg(p->m_nOrg),
    m_k(p->m_k),
    m_e(p->m_e),
    m_c(p->m_c),
    m_r(p->m_r),
    m_globalConstant(p->m_globalConstant),
    m_curCost(p->m_curCost),
    m_name(p->m_name),
    m_domains(p->m_domains),
    m_evidence(p->m_evidence),
    m_old2new(p->m_old2new),
    m_curSolution(p->m_curSolution),
    m_curUpperBound(p->m_curUpperBound)
{
    for (size_t i = 0; i < p->m_functions.size(); ++i) {
        m_functions.push_back(p->m_functions[i]->clone());
    }
}

inline Problem::~Problem() {
  // delete functions
  if (!m_is_copy)
    for (vector<Function*>::iterator it = m_functions.begin(); it!= m_functions.end(); ++it)
      if (*it) delete (*it);
}

}  // namespace daoopt

#endif /* PROBLEM_H_ */
