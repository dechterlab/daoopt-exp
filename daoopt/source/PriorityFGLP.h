#ifndef PRIORITYFGLP_H_
#define PRIORITYFGLP_H_

#include "FGLP.h"
#include "mex/include/indexedHeap.h"

namespace daoopt {

class PriorityFGLP : public FGLP {
 public:
  PriorityFGLP(Problem *p, bool use_nullary_shift = false);
  PriorityFGLP(PriorityFGLP *parent_fglp, const map<int, val_t> &assignment,
               const set<int>& sub_vars, int condition_var);

  virtual void Run(int max_updates, double max_time,
                   double tolerance);

  inline const mex::indexedHeap &var_priority() const { return var_priority_; }
  inline void set_var_priority(const mex::indexedHeap &vp) {
    var_priority_ = vp;
  }
  inline ~PriorityFGLP();

 protected:
  virtual void Condition(const vector<Function *> &fns,
                         const map<int, val_t> &assignment,
                         const set<int>& sub_vars, int conditioned_var);

  double MessageDist(double *m1, double *m2, double ns, int var);

  mex::indexedHeap var_priority_;
};

inline PriorityFGLP::~PriorityFGLP() {}

}  // namespace daoopt

#endif
