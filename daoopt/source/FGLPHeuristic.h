#ifndef FGLPHEURISTIC_H_
#define FGLPHEURISTIC_H_

#include "Heuristic.h"
#include "Function.h"
#include "Problem.h"
#include "ProgramOptions.h"
#include "Pseudotree.h"
#include "utils.h"
#include "FGLP.h"
#include "ResidualFGLP.h"
#include "PriorityFGLP.h"
#include "ExtraNodeInfo.h"

namespace daoopt {

// Utility class to contain all the FGLP processed problems
// corresponding to each variable assignment
// Also stores the cost to the node based on the original functions
class FGLPNodeInfo : public ExtraNodeInfo {
    unique_ptr<FGLP> m_fglpStore;
    double m_origCostToNode;
    public:
    const unique_ptr<FGLP> &getFGLPStore() const { return m_fglpStore; }
    void setFGLPStore(FGLP *fglp) { m_fglpStore.reset(fglp); }

    double getOrigCostToNode() const { return m_origCostToNode; }
    void setOrigCostToNode(double v) { m_origCostToNode = v; }
    ~FGLPNodeInfo() {
    }
};

class FGLPHeuristic : public Heuristic {
protected:
    double m_globalUB;  
    FGLP *rootFGLP;
    vector<vector<int>> m_ordering;

    vector<vector<int>> m_updateOrdering;

    map<int,val_t> m_tempAssn;
    vector<double> m_tempLabelsFGLP;
    vector<double> m_tempLabels;

    vector<set<int>> m_subproblemFunIds;
    vector<set<int>> m_subproblemVars;

    // Cannot do this with AO search
    // Whenever getHeur is called, this is popped until the top of the stack
    // is the parent of the current search node or it is empty
//    std::stack<pair<SearchNode*,vector<FGLP*>>> fglpStore;
//
    void computeSubproblemFunIds();
    void computeSubproblemVars();

    unsigned long long totalIterationsRun;
    unsigned long long totalInitiated;

    vector<unsigned long> m_countVars;
    vector<unsigned long> m_varsUpdated;

    
public:
    FGLPHeuristic(Problem *p, Pseudotree *pt, ProgramOptions *po);

    // No memory adjustments for FGLP
    size_t limitSize(size_t limit, const std::vector<val_t> *assignment) { 
        return 0;
    }

    // No memory size for FGLP
    size_t getSize() const { return 0; }

    // Get the initial bound for the problem via FGLP
    size_t build(const std::vector<val_t> *assignment = NULL, bool computeTables = true);

    bool readFromFile(std::string filename) { return false; } 
    bool writeToFile(std::string filename) const { return false; } 

    double getGlobalUB() const { return m_globalUB; }

    virtual double getHeur(int var, std::vector<val_t> &assignment, SearchNode *node);
	virtual double getHeurPerIndSubproblem(int var, std::vector<val_t> & assignment, SearchNode* node, double label, std::vector<double> & subprobH);
    virtual void getHeurAll(int var, std::vector<val_t> &assignment, SearchNode *node, std::vector<double> &out);

    // Readjusts the heuristic value so it is consistent with the original 
    // functions that are already assigned.
    // This differs in that we pass in the out vector that has already been 
    // computed by getHeurAll.
    void AdjustHeurAll(int var, const std::vector<val_t>& assignment, SearchNode* node, std::vector<double>& out);


    double getLabel(int var, const std::vector<val_t> &assignment, SearchNode *node);
    void getLabelAll(int var, const std::vector<val_t> &assignment, SearchNode *node, std::vector<double> &out);

    void printExtraStats() const {}

    unsigned long long getTotalIterationsRun() { return totalIterationsRun; }
    unsigned long long getTotalInitiated() { return totalInitiated; }

    const vector<unsigned long> &getCountVars() { return m_countVars; }
    const vector<unsigned long> &getVarsUpdated() { return m_varsUpdated; }

    FGLP* getRootFGLP() const { return rootFGLP; }

    virtual ~FGLPHeuristic() { 
        if (rootFGLP)
            delete rootFGLP;
    }

protected:
    void findDfsOrder(vector<int>& ordering, int var) const;
    void findBfsOrder(vector<int>& ordering, int var) const;


};

}  // namespace daoopt

#endif
