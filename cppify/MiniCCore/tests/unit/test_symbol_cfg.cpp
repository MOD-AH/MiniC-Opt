// Unit tests for the scoped symbol table and for CFG construction —
// the leader algorithm, edge wiring, dominators, back edges, natural
// loops, and reachability.
#include "test_helpers.h"
#include "symbol.h"

using namespace minic;

namespace {

Symbol var(const std::string& name) {
    Symbol s;
    s.kind = SymbolKind::VARIABLE;
    s.name = name;
    s.type = TypeSpec{};
    return s;
}

CFG cfgOf(const std::string& src, const std::string& fn = "main") {
    return CFG(th::funcOf(src, fn));
}

}  // namespace

// ---- symbol table: scoping -------------------------------------------

TEST(symtab_starts_at_global_level_zero) {
    SymbolTable st;
    CHECK_EQ(st.currentLevel(), 0);
}

TEST(symtab_enter_scope_raises_level) {
    SymbolTable st;
    st.enterScope();
    CHECK_EQ(st.currentLevel(), 1);
}

TEST(symtab_exit_scope_lowers_level) {
    SymbolTable st;
    st.enterScope();
    st.exitScope();
    CHECK_EQ(st.currentLevel(), 0);
}

TEST(symtab_declare_then_lookup_finds_it) {
    SymbolTable st;
    CHECK(st.declare(var("x")));
    CHECK(st.lookup("x") != nullptr);
}

TEST(symtab_lookup_of_undeclared_is_null) {
    SymbolTable st;
    CHECK(st.lookup("nope") == nullptr);
}

TEST(symtab_redeclaration_in_same_scope_is_rejected) {
    SymbolTable st;
    CHECK(st.declare(var("x")));
    CHECK(!st.declare(var("x")));
}

TEST(symtab_same_name_in_inner_scope_is_allowed) {
    SymbolTable st;
    CHECK(st.declare(var("x")));
    st.enterScope();
    CHECK(st.declare(var("x")));
}

TEST(symtab_inner_declaration_shadows_outer) {
    SymbolTable st;
    Symbol outer = var("x");
    outer.offset = 1;
    st.declare(outer);
    st.enterScope();
    Symbol inner = var("x");
    inner.offset = 2;
    st.declare(inner);
    REQUIRE(st.lookup("x") != nullptr);
    CHECK_EQ(st.lookup("x")->offset, 2);
}

TEST(symtab_shadow_disappears_when_scope_exits) {
    SymbolTable st;
    Symbol outer = var("x");
    outer.offset = 1;
    st.declare(outer);
    st.enterScope();
    Symbol inner = var("x");
    inner.offset = 2;
    st.declare(inner);
    st.exitScope();
    REQUIRE(st.lookup("x") != nullptr);
    CHECK_EQ(st.lookup("x")->offset, 1);
}

TEST(symtab_inner_scope_sees_outer_names) {
    SymbolTable st;
    st.declare(var("outer"));
    st.enterScope();
    CHECK(st.lookup("outer") != nullptr);
}

TEST(symtab_outer_scope_cannot_see_inner_names) {
    SymbolTable st;
    st.enterScope();
    st.declare(var("inner"));
    st.exitScope();
    CHECK(st.lookup("inner") == nullptr);
}

TEST(symtab_records_scope_level_on_declare) {
    SymbolTable st;
    st.enterScope();
    st.enterScope();
    st.declare(var("deep"));
    REQUIRE(st.lookup("deep") != nullptr);
    CHECK_EQ(st.lookup("deep")->scopeLevel, 2);
}

TEST(symtab_offsets_are_distinct_and_increasing) {
    SymbolTable st;
    int a = st.nextOffset();
    int b = st.nextOffset();
    CHECK(b > a);
}

TEST(symtab_resetOffsets_restarts_numbering) {
    SymbolTable st;
    int first = st.nextOffset();
    st.nextOffset();
    st.resetOffsets();
    CHECK_EQ(st.nextOffset(), first);
}

TEST(symtab_sibling_scopes_are_independent) {
    SymbolTable st;
    st.enterScope();
    st.declare(var("a"));
    st.exitScope();
    st.enterScope();
    CHECK(st.lookup("a") == nullptr);
    CHECK(st.declare(var("a")));
}

TEST(symtab_kind_is_preserved) {
    SymbolTable st;
    Symbol f;
    f.kind = SymbolKind::FUNCTION;
    f.name = "g";
    st.declare(f);
    REQUIRE(st.lookup("g") != nullptr);
    CHECK(st.lookup("g")->kind == SymbolKind::FUNCTION);
}

// ---- CFG: leader algorithm and edges ----------------------------------

TEST(cfg_straight_line_code_is_reachable_throughout) {
    CFG c = cfgOf("int main() { int a; a = 1; a = a + 1; return a; }");
    CHECK_EQ(c.reachableFromEntry().size(), c.blocks().size());
}

TEST(cfg_has_at_least_one_block) {
    CFG c = cfgOf("int main() { return 0; }");
    CHECK(c.blocks().size() >= size_t(1));
}

TEST(cfg_entry_is_a_valid_block_id) {
    CFG c = cfgOf("int main() { return 0; }");
    CHECK(c.entry() >= 0);
    CHECK(c.entry() < static_cast<int>(c.blocks().size()));
}

TEST(cfg_block_ids_match_their_index) {
    CFG c = cfgOf("int main() { int i; i = 0; while (i < 3) { i = i + 1; } return i; }");
    for (size_t i = 0; i < c.blocks().size(); ++i)
        CHECK_EQ(c.block(static_cast<int>(i)).id, static_cast<int>(i));
}

TEST(cfg_branch_creates_more_than_one_block) {
    CFG c = cfgOf("int main() { int x; x = 1; if (x) { x = 2; } return x; }");
    CHECK(c.blocks().size() > size_t(1));
}

TEST(cfg_edges_are_symmetric) {
    // Every successor edge must appear as a predecessor edge on the far end.
    CFG c = cfgOf("int main() { int i; i = 0; while (i < 3) { i = i + 1; } return i; }");
    for (const BasicBlock& b : c.blocks()) {
        for (int s : b.succs) {
            const BasicBlock& t = c.block(s);
            bool found = false;
            for (int p : t.preds)
                if (p == b.id) found = true;
            CHECK(found);
        }
    }
}

TEST(cfg_entry_has_no_predecessors_in_a_simple_function) {
    CFG c = cfgOf("int main() { return 0; }");
    CHECK(c.block(c.entry()).preds.empty());
}

TEST(cfg_a_loop_produces_a_back_edge) {
    CFG c = cfgOf("int main() { int i; i = 0; while (i < 3) { i = i + 1; } return i; }");
    CHECK(c.findBackEdges().size() >= size_t(1));
}

TEST(cfg_straight_line_code_has_no_back_edge) {
    CFG c = cfgOf("int main() { int a; a = 1; return a; }");
    CHECK(c.findBackEdges().empty());
}

TEST(cfg_two_loops_produce_two_back_edges) {
    CFG c = cfgOf(
        "int main() { int i; int j; int s; s = 0;"
        " for (i = 0; i < 3; i = i + 1) { s = s + 1; }"
        " for (j = 0; j < 3; j = j + 1) { s = s + 1; }"
        " return s; }");
    CHECK_EQ(c.findBackEdges().size(), size_t(2));
}

TEST(cfg_nested_loops_produce_two_back_edges) {
    CFG c = cfgOf(
        "int main() { int i; int j; int s; s = 0;"
        " for (i = 0; i < 3; i = i + 1) {"
        "   for (j = 0; j < 3; j = j + 1) { s = s + 1; } }"
        " return s; }");
    CHECK_EQ(c.findBackEdges().size(), size_t(2));
}

// ---- CFG: dominators --------------------------------------------------

TEST(cfg_every_block_dominates_itself) {
    CFG c = cfgOf("int main() { int i; i = 0; while (i < 3) { i = i + 1; } return i; }");
    std::vector<std::set<int>> dom = c.computeDominators();
    for (size_t i = 0; i < dom.size(); ++i)
        CHECK(dom[i].count(static_cast<int>(i)) == 1);
}

TEST(cfg_entry_dominates_every_reachable_block) {
    CFG c = cfgOf("int main() { int x; x = 1; if (x) { x = 2; } else { x = 3; } return x; }");
    std::vector<std::set<int>> dom = c.computeDominators();
    std::set<int> reach = c.reachableFromEntry();
    for (int b : reach) CHECK(dom[b].count(c.entry()) == 1);
}

TEST(cfg_entry_is_dominated_only_by_itself) {
    CFG c = cfgOf("int main() { int x; x = 1; if (x) { x = 2; } return x; }");
    std::vector<std::set<int>> dom = c.computeDominators();
    CHECK_EQ(dom[c.entry()].size(), size_t(1));
}

TEST(cfg_dominator_sets_cover_every_block) {
    CFG c = cfgOf("int main() { int i; i = 0; while (i < 3) { i = i + 1; } return i; }");
    CHECK_EQ(c.computeDominators().size(), c.blocks().size());
}

// ---- CFG: natural loops and reachability ------------------------------

TEST(cfg_natural_loop_contains_its_header_and_tail) {
    CFG c = cfgOf("int main() { int i; i = 0; while (i < 3) { i = i + 1; } return i; }");
    std::vector<std::pair<int,int>> back = c.findBackEdges();
    REQUIRE(!back.empty());
    std::set<int> loop = c.naturalLoop(back[0].first, back[0].second);
    CHECK(loop.count(back[0].first) == 1);
    CHECK(loop.count(back[0].second) == 1);
}

TEST(cfg_natural_loop_is_not_the_whole_function) {
    // The return block sits outside the loop.
    CFG c = cfgOf("int main() { int i; i = 0; while (i < 3) { i = i + 1; } return i; }");
    std::vector<std::pair<int,int>> back = c.findBackEdges();
    REQUIRE(!back.empty());
    CHECK(c.naturalLoop(back[0].first, back[0].second).size() < c.blocks().size());
}

TEST(cfg_reachable_set_includes_entry) {
    CFG c = cfgOf("int main() { return 0; }");
    CHECK(c.reachableFromEntry().count(c.entry()) == 1);
}

TEST(cfg_reachable_never_exceeds_block_count) {
    CFG c = cfgOf("int main() { int x; x = 1; if (x) { x = 2; } else { x = 3; } return x; }");
    CHECK(c.reachableFromEntry().size() <= c.blocks().size());
}

// ---- CFG: rendering ---------------------------------------------------

TEST(cfg_toDot_emits_a_digraph) {
    CFG c = cfgOf("int main() { return 0; }");
    CHECK(c.toDot().find("digraph") != std::string::npos);
}

TEST(cfg_toDot_mentions_every_block) {
    CFG c = cfgOf("int main() { int i; i = 0; while (i < 3) { i = i + 1; } return i; }");
    std::string dot = c.toDot();
    for (size_t i = 0; i < c.blocks().size(); ++i)
        CHECK(dot.find("B" + std::to_string(i)) != std::string::npos);
}
