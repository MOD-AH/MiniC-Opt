// Cppify — IR verifier (see include/verify.h for the rules)
#include "verify.h"
#include "ir_utils.h"
#include <algorithm>
#include <map>
#include <set>
#include <unordered_map>

namespace minic {
namespace {

bool isTerminator(Op op) { return op == Op::GOTO || op == Op::IF_FALSE || op == Op::RET; }

// A block an earlier pass emptied and disconnected. flatten() emits nothing
// for it, so it is transparent to fallthrough.
bool isCleared(const CFG& cfg, int id) {
    const BasicBlock& b = cfg.block(id);
    return id != cfg.entry() && b.code.empty() && b.phis.empty() && b.preds.empty() && b.succs.empty();
}

std::string where(int block, const Quad* q) {
    std::string s = "B" + std::to_string(block);
    if (q) s += " `" + q->toString() + "`";
    return s;
}

void checkShape(const Quad& q, int bi, std::vector<std::string>& errs) {
    auto need = [&](bool ok, const char* what) {
        if (!ok) errs.push_back(where(bi, &q) + ": " + what);
    };
    bool resName = isNameOperand(q.result);
    switch (q.op) {
        case Op::LABEL: case Op::GOTO:
            need(q.result.kind == OperandKind::LABEL_REF, "needs a label operand"); break;
        case Op::IF_FALSE:
            need(q.result.kind == OperandKind::LABEL_REF, "needs a label operand");
            need(!q.arg1.isNone(), "branch has no condition"); break;
        case Op::COPY: case Op::NEG: case Op::NOT: case Op::BITNOT: case Op::TO_INT: case Op::TO_FLOAT:
            need(!q.arg1.isNone(), "missing operand"); need(resName, "result is not a name"); break;
        case Op::LOAD_INDEX:
            need(isNameOperand(q.arg1), "base is not a name"); need(!q.arg2.isNone(), "missing index");
            need(resName, "result is not a name"); break;
        case Op::STORE_INDEX:
            need(resName, "base is not a name");
            need(!q.arg1.isNone() && !q.arg2.isNone(), "missing index or value"); break;
        case Op::COPY_BLOCK:
            need(resName && isNameOperand(q.arg1), "copy needs two pointers");
            need(q.arg2.kind == OperandKind::INT_CONST, "copy size is not a constant"); break;
        case Op::ALLOC_ARRAY:
            need(q.arg1.kind == OperandKind::INT_CONST, "size is not a constant");
            need(resName, "result is not a name"); break;
        case Op::CALL:
            need(q.arg2.kind == OperandKind::INT_CONST, "argument count is not a constant"); break;
        case Op::PARAM:
            need(!q.arg1.isNone(), "missing argument"); break;
        case Op::RET: case Op::NOP:
            break;
        default:
            need(!q.arg1.isNone() && !q.arg2.isNone(), "binary operation is missing an operand");
            need(resName, "result is not a name");
    }
}

}  // namespace

std::vector<std::string> verifyCFG(const CFG& cfg) {
    std::vector<std::string> errs;
    const int n = static_cast<int>(cfg.blocks().size());

    // ---- block shape and labels -------------------------------------------
    std::map<std::string, int> labelBlock;
    for (int bi = 0; bi < n; ++bi) {
        const BasicBlock& b = cfg.block(bi);
        if (b.id != bi) errs.push_back("block at index " + std::to_string(bi) + " has id " + std::to_string(b.id));
        for (size_t qi = 0; qi < b.code.size(); ++qi) {
            const Quad& q = b.code[qi];
            checkShape(q, bi, errs);
            if (q.op == Op::LABEL) {
                if (qi != 0) errs.push_back(where(bi, &q) + ": label is not the first instruction of its block");
                else if (!labelBlock.emplace(q.result.name, bi).second)
                    errs.push_back(where(bi, &q) + ": label is defined twice");
            }
            if (isTerminator(q.op) && qi + 1 != b.code.size())
                errs.push_back(where(bi, &q) + ": control transfer is not the last instruction of its block");
        }
    }

    // ---- succs match the code; preds are their inverse --------------------
    for (int bi = 0; bi < n; ++bi) {
        if (isCleared(cfg, bi)) continue;
        const BasicBlock& b = cfg.block(bi);
        int ft = -1;
        for (int j = bi + 1; j < n; ++j) if (!isCleared(cfg, j)) { ft = j; break; }
        std::multiset<int> expect;
        const Quad* last = b.code.empty() ? nullptr : &b.code.back();
        auto target = [&](const Quad& q) {
            auto it = labelBlock.find(q.result.name);
            if (it == labelBlock.end()) {
                errs.push_back(where(bi, &q) + ": branch target '" + q.result.name + "' does not exist");
                return;
            }
            expect.insert(it->second);
        };
        if (last && last->op == Op::GOTO) target(*last);
        else if (last && last->op == Op::IF_FALSE) { target(*last); if (ft >= 0) expect.insert(ft); }
        else if (last && last->op == Op::RET) {}
        else if (ft >= 0) expect.insert(ft);
        std::set<int> have(b.succs.begin(), b.succs.end()), want(expect.begin(), expect.end());
        if (have != want) {
            std::string s = where(bi, last) + ": succs {";
            for (int x : have) s += " B" + std::to_string(x);
            s += " } but the code transfers control to {";
            for (int x : want) s += " B" + std::to_string(x);
            errs.push_back(s + " }");
        }
    }
    for (int bi = 0; bi < n; ++bi) {
        for (int s : cfg.block(bi).succs) {
            if (s < 0 || s >= n) { errs.push_back(where(bi, nullptr) + ": successor out of range"); continue; }
            const auto& sp = cfg.block(s).preds;
            const auto& bs = cfg.block(bi).succs;
            if (std::count(sp.begin(), sp.end(), bi) != std::count(bs.begin(), bs.end(), s))
                errs.push_back("edge B" + std::to_string(bi) + " -> B" + std::to_string(s) +
                               " is not mirrored exactly in the successor's preds");
        }
        for (int p : cfg.block(bi).preds) {
            if (p < 0 || p >= n) { errs.push_back(where(bi, nullptr) + ": predecessor out of range"); continue; }
            const auto& ps = cfg.block(p).succs;
            if (std::find(ps.begin(), ps.end(), bi) == ps.end())
                errs.push_back("B" + std::to_string(bi) + " lists B" + std::to_string(p) +
                               " as a predecessor, but B" + std::to_string(p) + " has no edge to it");
        }
        const BasicBlock& b = cfg.block(bi);
        for (const Phi& phi : b.phis)
            if (phi.args.size() != b.preds.size())
                errs.push_back(where(bi, nullptr) + ": phi for " + phi.result.name + " has " +
                               std::to_string(phi.args.size()) + " arguments for " +
                               std::to_string(b.preds.size()) + " predecessors");
    }
    if (!errs.empty()) return errs;   // the checks below assume a consistent graph

    // ---- temporaries are written before they are read, on every path ------
    std::set<int> reach = cfg.reachableFromEntry();
    std::set<std::string> universe;
    for (const auto& b : cfg.blocks()) {
        for (const Phi& phi : b.phis) if (phi.result.kind == OperandKind::TEMPORARY) universe.insert(phi.result.name);
        for (const Quad& q : b.code) {
            Operand w;
            if (writesOf(q, w) && w.kind == OperandKind::TEMPORARY) universe.insert(w.name);
        }
    }
    std::vector<std::set<std::string>> out(static_cast<size_t>(n), universe), in(static_cast<size_t>(n));
    auto defsInto = [](const BasicBlock& b, std::set<std::string>& s) {
        for (const Phi& phi : b.phis) if (phi.result.kind == OperandKind::TEMPORARY) s.insert(phi.result.name);
        for (const Quad& q : b.code) {
            Operand w;
            if (writesOf(q, w) && w.kind == OperandKind::TEMPORARY) s.insert(w.name);
        }
    };
    for (bool changed = true; changed; ) {
        changed = false;
        for (int bi = 0; bi < n; ++bi) {
            if (!reach.count(bi)) continue;
            std::set<std::string> cur;
            bool first = true;
            for (int p : cfg.block(bi).preds) {
                if (!reach.count(p)) continue;
                if (first) { cur = out[static_cast<size_t>(p)]; first = false; continue; }
                std::set<std::string> tmp;
                std::set_intersection(cur.begin(), cur.end(), out[static_cast<size_t>(p)].begin(),
                                      out[static_cast<size_t>(p)].end(), std::inserter(tmp, tmp.begin()));
                cur = std::move(tmp);
            }
            in[static_cast<size_t>(bi)] = cur;
            defsInto(cfg.block(bi), cur);
            if (cur != out[static_cast<size_t>(bi)]) { out[static_cast<size_t>(bi)] = std::move(cur); changed = true; }
        }
    }
    for (int bi : reach) {
        const BasicBlock& b = cfg.block(bi);
        for (const Phi& phi : b.phis)
            for (size_t i = 0; i < phi.args.size(); ++i) {
                const Operand& a = phi.args[i];
                int p = b.preds[i];
                if (a.kind == OperandKind::TEMPORARY && reach.count(p) && !out[static_cast<size_t>(p)].count(a.name))
                    errs.push_back(where(bi, nullptr) + ": phi argument " + a.name + " from B" +
                                   std::to_string(p) + " may be unset on that edge");
            }
        std::set<std::string> cur = in[static_cast<size_t>(bi)];
        for (const Phi& phi : b.phis) if (phi.result.kind == OperandKind::TEMPORARY) cur.insert(phi.result.name);
        for (const Quad& q : b.code) {
            for (const Operand& r : readsOf(q))
                if (r.kind == OperandKind::TEMPORARY && !cur.count(r.name))
                    errs.push_back(where(bi, &q) + ": temporary " + r.name + " may be read before it is written");
            Operand w;
            if (writesOf(q, w) && w.kind == OperandKind::TEMPORARY) cur.insert(w.name);
        }
    }

    // ---- SSA ---------------------------------------------------------------
    if (!cfg.inSSA) return errs;
    std::set<std::string> originals;
    for (const auto& kv : cfg.ssaOrigin) originals.insert(kv.second);
    struct Site { int block; int index; };   // index -1: a phi
    std::unordered_map<std::string, Site> def;
    auto noteDef = [&](const std::string& name, int bi, int qi) {
        if (originals.count(name))
            errs.push_back(where(bi, nullptr) + ": original name " + name + " is assigned in SSA form");
        else if (cfg.ssaOrigin.count(name) && !def.emplace(name, Site{bi, qi}).second)
            errs.push_back(where(bi, nullptr) + ": SSA name " + name + " is defined more than once");
    };
    for (int bi = 0; bi < n; ++bi) {
        const BasicBlock& b = cfg.block(bi);
        for (const Phi& phi : b.phis) noteDef(phi.result.name, bi, -1);
        for (size_t qi = 0; qi < b.code.size(); ++qi) {
            Operand w;
            if (writesOf(b.code[qi], w)) noteDef(w.name, bi, static_cast<int>(qi));
        }
    }
    std::vector<std::set<int>> dom = cfg.computeDominators();
    auto dominatesUse = [&](const std::string& name, int bi, int qi, const char* how) {
        if (!cfg.ssaOrigin.count(name)) return;
        auto it = def.find(name);
        if (it == def.end()) { errs.push_back(where(bi, nullptr) + ": " + name + " is used but never defined"); return; }
        const Site& d = it->second;
        bool ok = d.block == bi ? d.index < qi : dom[static_cast<size_t>(bi)].count(d.block) > 0;
        if (!ok) errs.push_back(where(bi, nullptr) + ": " + how + " of " + name + " is not dominated by its definition");
    };
    for (int bi : reach) {
        const BasicBlock& b = cfg.block(bi);
        for (const Phi& phi : b.phis)
            for (size_t i = 0; i < phi.args.size(); ++i)
                if (isNameOperand(phi.args[i]) && reach.count(b.preds[i]))
                    dominatesUse(phi.args[i].name, b.preds[i], 1 << 30, "phi argument");
        for (size_t qi = 0; qi < b.code.size(); ++qi)
            for (const Operand& r : readsOf(b.code[qi])) dominatesUse(r.name, bi, static_cast<int>(qi), "use");
    }
    return errs;
}

}  // namespace minic
