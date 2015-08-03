/*
 * BoundPropagatorMaster.cpp
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
 *  Created on: Feb 14, 2010
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#undef DEBUG

#include "BoundPropagatorMaster.h"


#ifdef PARALLEL_DYNAMIC

namespace daoopt {

void BoundPropagatorMaster::operator() () {

  bool allDone = false;
  size_t noPropagated = 0;

  try { while (!allDone) {

    SearchNode* n = NULL;
    Subproblem* sp = NULL;
    bool hasMore = false;
    noPropagated = 0;
    boost::thread* tp = NULL;

    do {
      {
        GETLOCK(m_spaceMaster->mtx_solved, lk);

        while( m_spaceMaster->solved.empty() && m_spaceMaster->leaves.empty() ) {
          CONDWAIT(m_spaceMaster->cond_solved, lk);
        }

        if (m_spaceMaster->leaves.size()) {
          // new leaf node to propagate
          n = m_spaceMaster->leaves.front();
          m_spaceMaster->leaves.pop();
        } else if (m_spaceMaster->solved.size()) {
          // new externally solved subproblem
          sp = m_spaceMaster->solved.front();
          n = sp->root; // root node of subproblem

          // collect subproblem statistics
          {
//            myprint("Adding subproblem to statistics\n");
            GETLOCK(m_spaceMaster->mtx_stats, lk2);
//            myprint("Acquired mutex\n");
            m_spaceMaster->avgStats->addSubprob(sp);
          }

          { // clean up processing thread
            GETLOCK(m_spaceMaster->mtx_activeThreads, lk2);
            map< Subproblem*, boost::thread* >::iterator it = m_spaceMaster->activeThreads.find(sp);
            if (it!=m_spaceMaster->activeThreads.end()) {
              // tp = m_space->activeThreads.find(p)->second;
              tp = it->second;
              m_spaceMaster->activeThreads.erase(sp);
              tp->join();
              delete tp;
            }
          }

          delete sp;
          m_spaceMaster->solved.pop();
        }

      } // mtx_solved released

      { // actual propagation
        GETLOCK(m_spaceMaster->mtx_space, lk);
        propagate(n, true);
        ++noPropagated;
      }

      { //check if there's more to propagate
        GETLOCK(m_spaceMaster->mtx_solved, lk);
        hasMore = !m_spaceMaster->solved.empty() || !m_spaceMaster->leaves.empty();
      }

    } while (hasMore);



    { // TODO: Make sure race condition can't happen
      GETLOCK(m_spaceMaster->mtx_searchDone,lk);
      if (m_spaceMaster->searchDone) { // search process done?
        GETLOCK(m_spaceMaster->mtx_activeThreads,lk2);
        if (m_spaceMaster->activeThreads.empty()) { // no more processing threads
          allDone = true;
        }
      } else { // search not done, notify
        { // notify the search that the buffer is open again
          GETLOCK(m_spaceMaster->mtx_allowedThreads,lk);
          m_spaceMaster->allowedThreads += noPropagated;
          m_spaceMaster->cond_allowedThreads.notify_one();
        }
      }
    }

  } // overall while(!allDone) loop

  myprint("\t!!! PROP done !!!\n");

  } catch (boost::thread_interrupted i) {
    myprint("\t!!! PROP aborted !!!\n");
  }

}

}  // namespace daoopt

#endif /* PARALLEL_DYNAMIC */

