// MiniC-Opt — basic blocks, control-flow graph, data-flow framework
// Owner: Member 3 (System Designer / Core Algorithms)
//
// STATUS: DRAFT — circulated for team review ahead of the Week-4 freeze.
//
// The data-flow solver is written ONCE, parameterised by direction, meet
// operator, boundary condition and transfer function, and instantiated four
// ways: reaching definitions, available expressions, live variables and
// dominators. That single generic solver is a direct demonstration of the
// monotone data-flow framework.
#pragma once
#include "ir.h"
#include <vector>
#include <set>
#include <map>

namespace minic {

// A maximal straight-line instruction sequence: one entry, one exit.
// Leaders (Aho et al. 8.4): the first instruction; any jump target;
// any instruction immediately following a jump.
struct BasicBlock {
    int               id = -1;
    std::vector<Quad> code;
    std::vector<int>  preds;
    std::vector<int>  succs;

    // Data-flow sets, filled in by the solver.
    std::set<int> gen, kill, in, out;
};

class CFG {
public:
    explicit CFG(const IRFunction& fn);          // runs the leader algorithm

    const std::vector<BasicBlock>& blocks() const { return blocks_; }
    BasicBlock&       block(int id)       { return blocks_[id]; }
    const BasicBlock& block(int id) const { return blocks_[id]; }
    int entry() const { return entry_; }

    // Structural analyses
    std::vector<std::set<int>> computeDominators() const;
    std::vector<std::pair<int,int>> findBackEdges() const;   // (tail, head)
    std::set<int> naturalLoop(int tail, int head) const;
    std::set<int> reachableFromEntry() const;

    // Graphviz export — minic --dump-cfg
    std::string toDot() const;

private:
    std::vector<BasicBlock> blocks_;
    int entry_ = 0;
    std::string fnName_;
};

// ---- Generic iterative worklist solver -----------------------------------
enum class Direction { FORWARD, BACKWARD };
enum class Meet      { UNION, INTERSECTION };

struct DataFlowSpec {
    Direction direction;
    Meet      meet;
    std::set<int> boundary;    // IN[entry] for forward, OUT[exit] for backward
    // transfer: OUT = gen ∪ (IN − kill)   (supplied per-instantiation)
};

// Terminates because the transfer functions are monotone over a finite
// lattice. An iteration cap is included as a defensive diagnostic only.
void solve(CFG& cfg, const DataFlowSpec& spec, int iterationCap = 10000);

} // namespace minic
