/*
 * utils.h
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

#ifndef UTILS_H_
#define UTILS_H_

#include "_base.h"
#include "gzstream.h"

namespace daoopt {
int memoryusage();
}  // namespace daoopt

#if defined WINDOWS || defined __APPLE__
namespace daoopt {
inline int memoryusage() {
  return -1;
}
}  // namespace daoopt
#else
#include <malloc.h>
namespace daoopt {
inline int memoryusage() {
  struct mallinfo info;
  info = mallinfo();
  cout
    << info.arena << '\t'
    << info.uordblks << '\t'
    << info.fordblks << '\t'
    << info.hblkhd << endl;
  return info.arena;
}
}  // namespace daoopt
#endif

namespace daoopt {

void myprint(std::string s);
void myerror(std::string s);
void err_txt(std::string s);

ostream& operator <<(ostream& os, const vector<int>& s);
ostream& operator <<(ostream& os, const vector<unsigned int>& s);

ostream& operator <<(ostream& os, const vector<signed short>& s);
ostream& operator <<(ostream& os, const vector<signed char>& s);

ostream& operator <<(ostream& os, const set<int>& s);
ostream& operator <<(ostream& os, const set<unsigned int>& s);

ostream& operator <<(ostream& os, const vector<double>& s);
ostream& operator <<(ostream& os, const vector<bool>& s);

ostream& operator <<(ostream& os, const map<int,val_t>& s);

string str_replace(string& s, const string& x, const string& y);

/* trim string from start (found on stackoverflow.com) */
static inline string& ltrim(string& s) {
  s.erase(s.begin(), find_if(s.begin(), s.end(), not1(ptr_fun<int, int>(isspace))));
  return s;
}
/* trim string from end (found on stackoverflow.com) */
static inline string& rtrim(string& s) {
  s.erase(find_if(s.rbegin(), s.rend(), not1(ptr_fun<int, int>(isspace))).base(), s.end());
  return s;
}
/* trim string from both ends (found on stackoverflow.com) */
static inline string& trim(string& s) {
  return ltrim(rtrim(s));
}


/*
 * increments the tuple value, up to each entry's limit. Returns false
 * iff no more tuples can be generated
 */
inline bool increaseTuple(size_t& idx, vector<val_t>& tuple, const vector<val_t>& limit) {
  assert(tuple.size()==limit.size());

  size_t i=tuple.size();
  while (i) {
    ++tuple[i-1];
    if (tuple[i-1] == limit[i-1]) { tuple[i-1] = 0; --i; }
    else break;
  }
  ++idx;
  if (i) return true;
  else return false;
}

/* same as above, but with int* as the tuple (used by mini bucket elimination) */
inline bool increaseTuple(size_t& idx, val_t* tuple, const vector<val_t>& limit) {

  size_t i=limit.size();
  while (i) {
    ++( tuple[i-1]);
    if ( tuple[i-1] == limit[i-1]) { tuple[i-1] = 0; --i; }
    else break;
  }
  ++idx;
  if (i) return true;
  else return false;
}

inline bool idx_map_increment(vector<val_t*> idx_map,
    const vector<int>& domains) {
  for (int k = idx_map.size() - 1; k >= 0; --k) {
    if (++(*idx_map[k]) < domains[k]) {
      return true;
    }
    *idx_map[k] = 0;
  }
  return false;
}

#if FALSE
/* Convert an int/size_t into a std::string */
inline char* myitoa(size_t x) {
  size_t z = x;
  size_t size = 2;
  while (z/=10) ++size;
  char* s = new char[size];
  snprintf(s,size,"%d",x);
  return s;
//  return std::string(s);
}
inline char* myitoa(int x) {
  int z = x;
  size_t size = (x<0)?3:2;
  while (z/=10) ++size;
  char* s = new char[size];
  snprintf(s,size,"%d",x);
  return s;
//  return std::string(s);
}
#endif

#if FALSE
/* Convert an int/size_t into a std::string */
inline std::string myitoa(size_t x) {
  size_t z = x;
  size_t size = 2;
  while (z/=10) ++size;
  char s[size];
  snprintf(s,size,"%d",x);
  return std::string(s);
}
inline std::string myitoa(int x) {
  int z = x;
  size_t size = (x<0)?3:2;
  while (z/=10) ++size;
  char s[size];
  snprintf(s,size,"%d",x);
  return std::string(s);
}
#endif

/*
 * returns true iff the intersection of X and Y is empty
 */
inline bool intersectionEmpty(const set<int>& a, const set<int>& b) {
  set<int>::iterator ita = a.begin();
  set<int>::iterator itb = b.begin();
  while (ita != a.end() && itb != b.end()) {
    if (*ita == *itb) {
      return false;
      ++ita;
      ++itb;
    } else if (*ita < *itb) {
      ++ita;
    } else { // *ita > *itb
      ++itb;
    }
  }
  return true;
}

/*
 * computes the intersection of two set<int>
 */
inline set<int> intersection(const set<int>& a, const set<int>& b) {
  set<int> s;
  set<int>::iterator ita = a.begin();
  set<int>::iterator itb = b.begin();
  while (ita != a.end() && itb != b.end()) {
    if (*ita == *itb) {
      s.insert(*ita);
      ++ita;
      ++itb;
    } else if (*ita < *itb) {
      ++ita;
    } else { // *ita > *itb
      ++itb;
    }
  }
  return s;
}

/*
 * returns the the set A - B
 */
inline set<int> setminus(const set<int> &a, const set<int>& b) {
  set<int> s(a);
  for (set<int>::iterator itb = b.begin(); itb != b.end(); ++itb) {
      s.erase(*itb);
  }
  return s;
}

/*
 * returns the size of (set A without set B)
 */
inline int setminusSize(const set<int>& a, const set<int>& b) {
  set<int>::iterator ita = a.begin();
  set<int>::iterator itb = b.begin();
  int s = 0;
  while (ita != a.end() && itb != b.end()) {
    if (*ita == *itb) {
      ++ita; ++itb;
    } else if (*ita < *itb) {
      ++s; ++ita;
    } else { // *ita > *itb
      ++itb;
    }
  }
  while (ita != a.end()) {
    ++s; ++ita;
  }
  return s;
}

inline int setminusSize(const vector<int>& a, const vector<int>& b) {
  vector<int>::const_iterator ita = a.begin();
  vector<int>::const_iterator itb = b.begin();
  int s = 0;
  while (ita != a.end() && itb != b.end()) {
    if (*ita == *itb) {
      ++ita; ++itb;
    } else if (*ita < *itb) {
      ++s; ++ita;
    } else { // *ita > *itb
      ++itb;
    }
  }
  while (ita != a.end()) {
    ++s; ++ita;
  }
  return s;
}

// Check if a is a subset of b
inline bool isSubset(const set<int> &a, const set<int> &b) {
    for (auto e : a) {
        if (b.find(e) == b.end()) return false;
    }
    return true;
}


/* hex output of values (e.g., doubles) */
template<typename _T>
void print_hex(const _T* d) {
  const unsigned char* ar = (const unsigned char*) d;
  for (size_t i = 0; i < sizeof(_T); ++i)
    printf("%X", ar[i]);
}

// Hack to read a file into a string
static inline std::string getFileContents(const char* filename) {
	igzstream in(filename, std::ios::in | std::ios::binary);
	std::string contents((std::istreambuf_iterator<char>(in)),
      std::istreambuf_iterator<char>());
	in.close();
	return contents;
}

}  // namespace daoopt

template<typename Collection, typename Key>
bool ContainsKey(const Collection& container, const Key& key) {
  return container.find(key) != container.end();
}


#endif /* UTILS_H_ */
