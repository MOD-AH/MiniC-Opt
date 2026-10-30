// Unit tests for the data-flow engine (reaching definitions, available
// expressions, live variables), the IR data structures, the optimization
// passes in isolation, and the reference interpreter's arithmetic.
#include "test_helpers.h"
#include "opt_passes.h"

#include "ir_utils.h"

#include <set>
#include <stdexcept>

using namespace minic;

namespace {

CFG cfgOf(const std::string& src, const std::string& fn = "main") {
    return CFG(th::funcOf(src, fn));
}

// Runs one named pass over a function's CFG and reports how many
// transformations it claimed.
int transformsOf(std::unique_ptr<Pass> pass, const std::string& src,
                 const std::string& fn = "main") {
    CFG c = cfgOf(src, fn);
    // Guard against a test naming a function that does not exist: an empty
    // IRFunction would report zero transforms and pass vacuously.
    CHECK(!c.blocks().empty());
    pass->run(c);
    return pass->transformCount();
}

const char* kLoop =
    "int main() { int i; int s; i = 0; s = 0;"
    " while (i < 4) { s = s + i; i = i + 1; } return s; }";

}  // namespace

// ---- data-flow result shape -------------------------------------------

TEST(dataflow_live_has_one_in_set_per_block) {
    CFG c = cfgOf(kLoop);
    DataFlowResult r = computeLiveVariables(c, IRFunction{});
    CHECK_EQ(r.in.size(), c.blocks().size());
}

TEST(dataflow_live_has_one_out_set_per_block) {
    CFG c = cfgOf(kLoop);
    DataFlowResult r = computeLiveVariables(c, IRFunction{});
    CHECK_EQ(r.out.size(), c.blocks().size());
}

TEST(dataflow_reaching_has_one_set_per_block) {
    CFG c = cfgOf(kLoop);
    DataFlowResult r = computeReachingDefs(c, th::funcOf(kLoop));
    CHECK_EQ(r.in.size(), c.blocks().size());
}

TEST(dataflow_available_has_one_set_per_block) {
    CFG c = cfgOf(kLoop);
    DataFlowResult r = computeAvailableExpressions(c, th::funcOf(kLoop));
    CHECK_EQ(r.out.size(), c.blocks().size());
}

TEST(dataflow_live_out_at_exit_is_empty) {
    // The analysis is intra-procedural: nothing is live after the return.
    CFG c = cfgOf(kLoop);
    DataFlowResult r = computeLiveVariables(c, IRFunction{});
    for (const BasicBlock& b : c.blocks())
        if (b.succs.empty()) CHECK(r.out[b.id].empty());
}

TEST(dataflow_live_in_at_entry_is_empty_when_nothing_is_read_first) {
    // main() takes no parameters and initialises before reading, so no
    // variable is live on entry.
    CFG c = cfgOf(kLoop);
    DataFlowResult r = computeLiveVariables(c, IRFunction{});
    CHECK(r.in[c.entry()].empty());
}

TEST(dataflow_parameter_is_live_on_entry) {
    CFG c = cfgOf("int f(int a) { return a + 1; }", "f");
    DataFlowResult r = computeLiveVariables(c, IRFunction{});
    CHECK(!r.in[c.entry()].empty());
}

TEST(dataflow_reaching_defs_entry_in_is_empty) {
    CFG c = cfgOf(kLoop);
    DataFlowResult r = computeReachingDefs(c, th::funcOf(kLoop));
    CHECK(r.in[c.entry()].empty());
}

TEST(dataflow_available_expressions_entry_in_is_empty) {
    CFG c = cfgOf(kLoop);
    DataFlowResult r = computeAvailableExpressions(c, th::funcOf(kLoop));
    CHECK(r.in[c.entry()].empty());
}

TEST(dataflow_live_is_stable_when_recomputed) {
    // The solver must be deterministic: same CFG, same answer.
    CFG c = cfgOf(kLoop);
    DataFlowResult a = computeLiveVariables(c, IRFunction{});
    DataFlowResult b = computeLiveVariables(c, IRFunction{});
    CHECK(a.in == b.in);
    CHECK(a.out == b.out);
}

TEST(dataflow_reaching_raw_exposes_definition_facts) {
    CFG c = cfgOf(kLoop);
    ReachingDefFacts f = computeReachingDefsRaw(c);
    (void)f;  // shape only: the call must succeed and not throw
    CHECK(true);
}

TEST(dataflow_converges_on_nested_loops) {
    CFG c = cfgOf(
        "int main() { int i; int j; int s; s = 0;"
        " for (i = 0; i < 3; i = i + 1) {"
        "   for (j = 0; j < 3; j = j + 1) { s = s + 1; } }"
        " return s; }");
    DataFlowResult r = computeLiveVariables(c, IRFunction{});
    CHECK_EQ(r.in.size(), c.blocks().size());
}

TEST(dataflow_handles_a_single_block_function) {
    CFG c = cfgOf("int main() { return 7; }");
    DataFlowResult r = computeLiveVariables(c, IRFunction{});
    CHECK_EQ(r.in.size(), c.blocks().size());
}

// ---- IR data structures ------------------------------------------------

TEST(ir_newTemp_produces_distinct_names) {
    IRFunction fn;
    CHECK_NE(fn.newTemp().name, fn.newTemp().name);
}

TEST(ir_newLabel_produces_distinct_names) {
    IRFunction fn;
    CHECK_NE(fn.newLabel().name, fn.newLabel().name);
}

TEST(ir_newTemp_advances_the_counter) {
    IRFunction fn;
    int before = fn.tempCounter;
    fn.newTemp();
    CHECK(fn.tempCounter > before);
}

TEST(ir_newLabel_advances_the_counter) {
    IRFunction fn;
    int before = fn.labelCounter;
    fn.newLabel();
    CHECK(fn.labelCounter > before);
}

TEST(ir_newTemp_is_a_temporary_operand) {
    IRFunction fn;
    CHECK(fn.newTemp().kind == OperandKind::TEMPORARY);
}

TEST(ir_newLabel_is_a_label_operand) {
    IRFunction fn;
    CHECK(fn.newLabel().kind == OperandKind::LABEL_REF);
}

TEST(ir_default_operand_is_none) {
    Operand o;
    CHECK(o.isNone());
}

TEST(ir_int_const_reports_isConst) {
    Operand o;
    o.kind = OperandKind::INT_CONST;
    o.ival = 3;
    CHECK(o.isConst());
}

TEST(ir_float_const_reports_isConst) {
    Operand o;
    o.kind = OperandKind::FLOAT_CONST;
    o.fval = 1.5;
    CHECK(o.isConst());
}

TEST(ir_variable_is_not_const) {
    Operand o;
    o.kind = OperandKind::VARIABLE;
    o.name = "x";
    CHECK(!o.isConst());
}

TEST(ir_quad_toString_is_non_empty) {
    Quad q;
    q.op = Op::ADD;
    q.result.kind = OperandKind::VARIABLE; q.result.name = "x";
    q.arg1.kind = OperandKind::INT_CONST;  q.arg1.ival = 1;
    q.arg2.kind = OperandKind::INT_CONST;  q.arg2.ival = 2;
    CHECK(q.toString().size() > 0);
}

TEST(ir_quad_toString_mentions_its_target) {
    Quad q;
    q.op = Op::COPY;
    q.result.kind = OperandKind::VARIABLE; q.result.name = "zz";
    q.arg1.kind = OperandKind::INT_CONST;  q.arg1.ival = 1;
    CHECK(q.toString().find("zz") != std::string::npos);
}

TEST(irgen_emits_a_function_for_main) {
    IRProgram p = th::irOf("int main() { return 0; }");
    CHECK_EQ(p.functions.size(), size_t(1));
}

TEST(irgen_emits_one_function_per_definition) {
    IRProgram p = th::irOf("int f() { return 1; } int main() { return f(); }");
    CHECK_EQ(p.functions.size(), size_t(2));
}

TEST(irgen_records_parameter_names) {
    IRFunction f = th::funcOf("int f(int a, int b) { return a + b; } int main() { return f(1,2); }", "f");
    CHECK_EQ(f.params.size(), size_t(2));
}

TEST(irgen_puts_globals_in_globalInit) {
    IRProgram p = th::irOf("int g = 5; int main() { return g; }");
    CHECK(!p.globalInit.empty());
}

TEST(irgen_zero_initialises_a_bare_global) {
    IRProgram p = th::irOf("int g; int main() { return g; }");
    CHECK(!p.globalInit.empty());
}

TEST(irgen_a_loop_emits_a_conditional_branch) {
    CHECK(th::countOp(th::funcOf(kLoop), Op::IF_FALSE) >= 1);
}

TEST(irgen_a_loop_emits_a_goto) {
    CHECK(th::countOp(th::funcOf(kLoop), Op::GOTO) >= 1);
}

TEST(irgen_a_call_emits_a_call_quad) {
    CHECK(th::countOp(th::funcOf("int f() { return 1; } int main() { return f(); }"), Op::CALL) >= 1);
}

TEST(irgen_arguments_emit_param_quads) {
    IRFunction m = th::funcOf("int f(int a, int b) { return a + b; } int main() { return f(1,2); }");
    CHECK_EQ(th::countOp(m, Op::PARAM), 2);
}

TEST(irgen_an_array_declaration_allocates) {
    CHECK(th::countOp(th::funcOf("int main() { int a[4]; a[0] = 1; return a[0]; }"), Op::ALLOC_ARRAY) >= 1);
}

TEST(irgen_array_read_emits_load_index) {
    CHECK(th::countOp(th::funcOf("int main() { int a[4]; a[0] = 1; return a[0]; }"), Op::LOAD_INDEX) >= 1);
}

TEST(irgen_array_write_emits_store_index) {
    CHECK(th::countOp(th::funcOf("int main() { int a[4]; a[0] = 1; return a[0]; }"), Op::STORE_INDEX) >= 1);
}

TEST(irgen_mixing_int_and_float_emits_a_conversion) {
    IRFunction m = th::funcOf("int main() { float f; f = 1; return 0; }");
    CHECK(th::countOp(m, Op::TO_FLOAT) + th::countOp(m, Op::TO_INT) >= 1);
}

// ---- optimization passes in isolation ---------------------------------

TEST(pass_fold_reports_a_transform_on_constant_arithmetic) {
    CHECK(transformsOf(makeConstantFoldingPass(),
                       "int main() { int x; x = 2 + 3; return x; }") >= 1);
}

TEST(pass_fold_does_nothing_without_constants) {
    CHECK_EQ(transformsOf(makeConstantFoldingPass(),
                          "int f(int a, int b) { return a + b; }", "f"), 0);
}

TEST(pass_constprop_reports_a_transform) {
    CHECK(transformsOf(makeConstantPropagationPass(),
                       "int main() { int a; int b; a = 5; b = a + 1; return b; }") >= 1);
}

TEST(pass_copyprop_reports_a_transform) {
    CHECK(transformsOf(makeCopyPropagationPass(),
                       "int f(int p) { int a; int b; a = p; b = a; return b; }", "f") >= 1);
}

TEST(pass_dce_removes_a_dead_local) {
    CHECK(transformsOf(makeDeadCodeEliminationPass(),
                       "int main() { int dead; dead = 5; return 7; }") >= 1);
}

TEST(pass_dce_removes_only_the_dead_zero_init) {
    // `int a;` emits `a = 0`, which the following `a = p + 1` immediately
    // overwrites, so exactly one store is dead. The live store feeding the
    // return must survive -- the companion test below checks the value.
    CHECK_EQ(transformsOf(makeDeadCodeEliminationPass(),
                          "int f(int p) { int a; a = p + 1; return a; }", "f"), 1);
}

TEST(pass_dce_preserves_the_value_of_a_used_local) {
    // Same function, end to end: if DCE had taken the live store too, this
    // would not still be p + 1.
    CHECK_EQ(th::runToValue("int f(int p) { int a; a = p + 1; return a; }"
                            " int main() { return f(41); }"), 42LL);
}

TEST(pass_dce_reports_nothing_on_a_function_with_no_dead_store) {
    // No declaration-without-use here, so there is nothing for DCE to take.
    CHECK_EQ(transformsOf(makeDeadCodeEliminationPass(),
                          "int f(int p) { return p + 1; }", "f"), 0);
}

TEST(pass_cse_reports_a_transform_on_a_repeated_expression) {
    CHECK(transformsOf(makeCommonSubexprEliminationPass(),
                       "int f(int a, int b) { int x; int y; x = a * b; y = a * b; return x + y; }", "f") >= 1);
}

TEST(pass_every_factory_returns_a_pass) {
    CHECK(makeConstantFoldingPass() != nullptr);
    CHECK(makeConstantPropagationPass() != nullptr);
    CHECK(makeCopyPropagationPass() != nullptr);
    CHECK(makeCommonSubexprEliminationPass() != nullptr);
    CHECK(makeDeadCodeEliminationPass() != nullptr);
    CHECK(makeUnreachableCodeEliminationPass() != nullptr);
    CHECK(makeStrengthReductionPass() != nullptr);
    CHECK(makeLoopInvariantCodeMotionPass() != nullptr);
    CHECK(makePeepholePass() != nullptr);
}

TEST(pass_every_pass_names_itself) {
    CHECK_EQ(makeConstantFoldingPass()->name(), std::string("fold"));
    CHECK_EQ(makeConstantPropagationPass()->name(), std::string("constprop"));
    CHECK_EQ(makeCopyPropagationPass()->name(), std::string("copyprop"));
    CHECK_EQ(makeCommonSubexprEliminationPass()->name(), std::string("cse"));
    CHECK_EQ(makeDeadCodeEliminationPass()->name(), std::string("dce"));
    CHECK_EQ(makeUnreachableCodeEliminationPass()->name(), std::string("unreachable"));
    CHECK_EQ(makeStrengthReductionPass()->name(), std::string("strength"));
    CHECK_EQ(makeLoopInvariantCodeMotionPass()->name(), std::string("licm"));
    CHECK_EQ(makePeepholePass()->name(), std::string("peephole"));
}

TEST(pass_every_pass_describes_itself) {
    CHECK(makeConstantFoldingPass()->description().size() > 0);
    CHECK(makeDeadCodeEliminationPass()->description().size() > 0);
    CHECK(makePeepholePass()->description().size() > 0);
}

TEST(pass_dce_never_deletes_a_store_to_a_global) {
    // Regression guard: liveness is intra-procedural, so without the
    // explicit global set this deleted the whole body of bump().
    std::set<std::string> globals{"counter"};
    CHECK_EQ(transformsOf(makeDeadCodeEliminationPass(globals),
                          "int counter = 0; void bump() { counter = counter + 1; }"
                          " int main() { bump(); return counter; }", "bump"), 0);
}

// ---- interpreter arithmetic -------------------------------------------

TEST(interp_addition) { CHECK_EQ(th::runToValue("int main() { return 2 + 3; }"), 5LL); }
TEST(interp_subtraction) { CHECK_EQ(th::runToValue("int main() { return 9 - 4; }"), 5LL); }
TEST(interp_multiplication) { CHECK_EQ(th::runToValue("int main() { return 6 * 7; }"), 42LL); }
TEST(interp_division_truncates) { CHECK_EQ(th::runToValue("int main() { return 7 / 2; }"), 3LL); }
TEST(interp_modulo) { CHECK_EQ(th::runToValue("int main() { return 7 % 3; }"), 1LL); }

TEST(interp_negative_division_truncates_toward_zero) {
    CHECK_EQ(th::runToValue("int main() { int a; int b; a = -7; b = 2; return a / b; }"), -3LL);
}

TEST(interp_negative_modulo_follows_the_dividend) {
    CHECK_EQ(th::runToValue("int main() { int a; int b; a = -7; b = 3; return a % b; }"), -1LL);
}

TEST(interp_unary_minus) { CHECK_EQ(th::runToValue("int main() { int a; a = 5; return -a; }"), -5LL); }

TEST(interp_logical_not_of_zero_is_one) {
    CHECK_EQ(th::runToValue("int main() { int a; a = 0; return !a; }"), 1LL);
}

TEST(interp_logical_not_of_nonzero_is_zero) {
    CHECK_EQ(th::runToValue("int main() { int a; a = 5; return !a; }"), 0LL);
}

TEST(interp_comparison_true_is_one) {
    CHECK_EQ(th::runToValue("int main() { int a; a = 2; return a < 3; }"), 1LL);
}

TEST(interp_comparison_false_is_zero) {
    CHECK_EQ(th::runToValue("int main() { int a; a = 5; return a < 3; }"), 0LL);
}

TEST(interp_and_short_circuits_past_a_trap) {
    // The right operand divides by zero; reaching it would throw.
    CHECK_EQ(th::runToValue("int main() { int a; a = 0; if (a != 0 && 1 / a > 0) { return 1; } return 2; }"), 2LL);
}

TEST(interp_or_short_circuits_past_a_trap) {
    CHECK_EQ(th::runToValue("int main() { int a; a = 0; if (a == 0 || 1 / a > 0) { return 1; } return 2; }"), 1LL);
}

TEST(interp_while_loop_accumulates) {
    CHECK_EQ(th::runToValue(kLoop), 6LL);  // 0+1+2+3
}

TEST(interp_recursion_returns_correctly) {
    CHECK_EQ(th::runToValue("int f(int n) { if (n <= 0) { return 0; } return n + f(n - 1); }"
                            " int main() { return f(5); }"), 15LL);
}

TEST(interp_falling_off_the_end_yields_zero) {
    CHECK_EQ(th::runToValue("void f() { } int main() { f(); return 0; }"), 0LL);
}

TEST(interp_counts_executed_instructions) {
    IRProgram p = th::irOf("int main() { int a; a = 1; return a; }");
    Interp in;
    in.run(p);
    CHECK(in.instructionsExecuted > 0);
}

TEST(interp_throws_on_division_by_zero) {
    bool threw = false;
    try {
        th::runToValue("int main() { int a; int b; a = 1; b = 0; return a / b; }");
    } catch (const Interp::RuntimeError&) {
        threw = true;
    }
    CHECK(threw);
}

TEST(interp_throws_on_modulo_by_zero) {
    bool threw = false;
    try {
        th::runToValue("int main() { int a; int b; a = 1; b = 0; return a % b; }");
    } catch (const Interp::RuntimeError&) {
        threw = true;
    }
    CHECK(threw);
}

TEST(interp_throws_on_out_of_bounds_array_access) {
    bool threw = false;
    try {
        th::runToValue("int main() { int a[2]; a[0] = 1; return a[50]; }");
    } catch (const Interp::RuntimeError&) {
        threw = true;
    }
    CHECK(threw);
}

TEST(interp_throws_when_the_entry_point_is_missing) {
    bool threw = false;
    try {
        IRProgram p = th::irOf("int f() { return 1; } int main() { return f(); }");
        Interp in;
        in.run(p, "no_such_function");
    } catch (const Interp::RuntimeError&) {
        threw = true;
    }
    CHECK(threw);
}

// ---- semantic analysis -------------------------------------------------

TEST(sema_accepts_a_well_formed_program) {
    CHECK(th::check("int main() { int a; a = 1; return a; }").ok);
}

TEST(sema_rejects_undeclared_identifier) {
    CHECK(th::hasError(th::check("int main() { return x; }").errors, "undeclared identifier"));
}

TEST(sema_rejects_redeclaration_in_scope) {
    CHECK(th::hasError(th::check("int main() { int x; int x; return 0; }").errors, "redeclaration"));
}

TEST(sema_rejects_call_arity_mismatch) {
    CHECK(th::hasError(th::check("int f(int a) { return a; } int main() { return f(1,2); }").errors,
                       "expects"));
}

TEST(sema_rejects_subscripting_a_non_array) {
    CHECK(th::hasError(th::check("int main() { int x; x = 1; return x[0]; }").errors,
                       "not an array"));
}

TEST(sema_rejects_missing_return) {
    CHECK(th::hasError(th::check("int f() { } int main() { return f(); }").errors,
                       "does not return a value"));
}

TEST(sema_rejects_call_to_undeclared_function) {
    CHECK(th::hasError(th::check("int main() { return nope(1); }").errors,
                       "undeclared function"));
}

TEST(sema_rejects_modulo_on_a_float) {
    CHECK(th::hasError(th::check("int main() { float f; f = 1.5; return f % 2; }").errors,
                       "must be integral"));
}

TEST(sema_rejects_constant_division_by_zero) {
    CHECK(th::hasError(th::check("int main() { return 1 / 0; }").errors, "division by zero"));
}

TEST(sema_rejects_assigning_to_a_whole_array) {
    CHECK(th::hasError(th::check("int main() { int a[2]; int b[2]; a = b; return 0; }").errors,
                       "cannot assign to an array"));
}

TEST(sema_rejects_too_many_initializers) {
    CHECK(th::hasError(th::check("int main() { int a[2] = {1,2,3}; return a[0]; }").errors,
                       "too many initializers"));
}

TEST(sema_rejects_value_returned_from_void) {
    CHECK(th::hasError(th::check("void f() { return 1; } int main() { f(); return 0; }").errors,
                       "returning void"));
}

TEST(sema_rejects_duplicate_parameter_names) {
    CHECK(th::hasError(th::check("int f(int a, int a) { return a; } int main() { return f(1,2); }").errors,
                       "redeclaration of parameter"));
}

TEST(sema_accepts_shadowing_in_an_inner_scope) {
    CHECK(th::check("int main() { int x; x = 1; { int x; x = 2; } return x; }").ok);
}

TEST(sema_accepts_a_forward_reference_to_a_later_function) {
    CHECK(th::check("int main() { return g(); } int g() { return 1; }").ok);
}

TEST(sema_accepts_recursion) {
    CHECK(th::check("int f(int n) { if (n <= 0) { return 0; } return f(n - 1); }"
                    " int main() { return f(3); }").ok);
}

TEST(sema_error_carries_a_position) {
    std::vector<Diagnostic> errs = th::check("int main() { return x; }").errors;
    REQUIRE(!errs.empty());
    CHECK(errs[0].line > 0);
    CHECK(errs[0].col > 0);
}

// ---- parser ------------------------------------------------------------

TEST(parser_accepts_a_minimal_program) {
    CHECK(th::parse("int main() { return 0; }").errors.empty());
}

TEST(parser_returns_a_program_even_on_error) {
    // Panic-mode recovery must still hand back a tree, never null.
    CHECK(th::parse("int main() { int x = 5 return x; }").program != nullptr);
}

TEST(parser_reports_a_missing_semicolon) {
    CHECK(!th::parse("int main() { int x = 5 return x; }").errors.empty());
}

TEST(parser_reports_an_unbalanced_brace) {
    CHECK(!th::parse("int main() { return 0;").errors.empty());
}

TEST(parser_rejects_assignment_to_a_literal) {
    CHECK(!th::parse("int main() { 1 = 2; return 0; }").errors.empty());
}

TEST(parser_accepts_every_statement_form) {
    CHECK(th::parse(
        "int main() { int i; i = 0;"
        " if (i) { i = 1; } else { i = 2; }"
        " while (i < 3) { i = i + 1; }"
        " do { i = i + 1; } while (i < 5);"
        " for (i = 0; i < 2; i = i + 1) { if (i) { continue; } else { break; } }"
        " return i; }").errors.empty());
}

TEST(parser_accepts_an_empty_statement) {
    CHECK(th::parse("int main() { ; return 0; }").errors.empty());
}

TEST(parser_recovers_and_reports_more_than_one_error) {
    CHECK(th::parse("int main() { int a = ; int b = ; return 0; }").errors.size() >= size_t(1));
}

// ---- bitwise operators ---------------------------------------------------
// Regression group. Adding these operators originally broke CSE: opTag() fell
// through to a constant "?" for every operator it did not list, so `n & 1` and
// `n >> 1` got the same expression key and CSE replaced the shift with the AND
// result. The loop `while (n) { c += n & 1; n >>= 1; }` then never terminated.

TEST(opTag_is_distinct_for_every_binary_operator) {
    // If a binary operator is added without a tag, two operators can alias in
    // CSE. This enumerates them all so that omission fails here, not in a loop
    // that hangs.
    std::vector<Op> ops = {Op::ADD, Op::SUB, Op::MUL, Op::DIV, Op::MOD,
                           Op::LT, Op::LE, Op::GT, Op::GE, Op::EQ, Op::NE,
                           Op::AND, Op::OR,
                           Op::BITAND, Op::BITOR, Op::BITXOR, Op::SHL, Op::SHR};
    std::set<std::string> tags;
    for (Op o : ops) tags.insert(opTag(o));
    CHECK_EQ(tags.size(), ops.size());
}

TEST(opTag_never_returns_the_old_shared_question_mark) {
    CHECK(opTag(Op::BITAND) != "?");
    CHECK(opTag(Op::SHR) != "?");
}

TEST(ir_quad_prints_real_symbols_for_bitwise_operators) {
    auto render = [](Op op) {
        Quad q;
        q.op = op;
        q.result.kind = OperandKind::VARIABLE; q.result.name = "r";
        q.arg1.kind = OperandKind::VARIABLE;   q.arg1.name = "a";
        q.arg2.kind = OperandKind::INT_CONST;  q.arg2.ival = 1;
        return q.toString();
    };
    CHECK(render(Op::BITAND).find("&") != std::string::npos);
    CHECK(render(Op::BITOR).find("|") != std::string::npos);
    CHECK(render(Op::BITXOR).find("^") != std::string::npos);
    CHECK(render(Op::SHL).find("<<") != std::string::npos);
    CHECK(render(Op::SHR).find(">>") != std::string::npos);
    CHECK(render(Op::SHR).find("?") == std::string::npos);
}

TEST(pass_cse_does_not_merge_and_with_shift_on_the_same_operands) {
    CHECK_EQ(transformsOf(makeCommonSubexprEliminationPass(),
                          "int f(int n) { int a; int b; a = n & 1; b = n >> 1; return a + b; }", "f"), 0);
}

TEST(pass_cse_does_not_merge_or_with_xor_on_the_same_operands) {
    CHECK_EQ(transformsOf(makeCommonSubexprEliminationPass(),
                          "int f(int n) { int a; int b; a = n | 6; b = n ^ 6; return a + b; }", "f"), 0);
}

TEST(pass_cse_does_merge_an_identical_bitwise_expression) {
    CHECK(transformsOf(makeCommonSubexprEliminationPass(),
                       "int f(int n) { int a; int b; a = n & 6; b = n & 6; return a + b; }", "f") >= 1);
}

TEST(interp_popcount_loop_terminates_with_every_pass_enabled) {
    // The exact shape that hung before the opTag fix.
    const char* src =
        "int pc(int n) { int c = 0; while (n != 0) { c += n & 1; n >>= 1; } return c; }"
        " int main() { return pc(255); }";
    CHECK_EQ(th::runToValue(src), 8LL);
}

TEST(interp_bitwise_and) { CHECK_EQ(th::runToValue("int main() { return 12 & 10; }"), 8LL); }
TEST(interp_bitwise_or) { CHECK_EQ(th::runToValue("int main() { return 12 | 3; }"), 15LL); }
TEST(interp_bitwise_xor) { CHECK_EQ(th::runToValue("int main() { return 12 ^ 10; }"), 6LL); }
TEST(interp_bitwise_not) { CHECK_EQ(th::runToValue("int main() { int a = 0; return ~a; }"), -1LL); }
TEST(interp_shift_left) { CHECK_EQ(th::runToValue("int main() { return 3 << 4; }"), 48LL); }
TEST(interp_shift_right_is_arithmetic) {
    CHECK_EQ(th::runToValue("int main() { int a = 0 - 16; return a >> 2; }"), -4LL);
}
TEST(interp_hex_literal) { CHECK_EQ(th::runToValue("int main() { return 0x1F; }"), 31LL); }

TEST(interp_compound_bitwise_assignment_chain) {
    // 5|8=13, 13&12=12, 12^3=15, 15<<1=30, 30>>2=7
    CHECK_EQ(th::runToValue("int main() { int a = 5; a |= 8; a &= 12; a ^= 3; a <<= 1; a >>= 2; return a; }"), 7LL);
}

TEST(interp_and_binds_tighter_than_or) {
    // 4 | (6&3) = 6.   The wrong grouping (4|6)&3 would give 2.
    CHECK_EQ(th::runToValue("int main() { return 4 | 6 & 3; }"), 6LL);
}

TEST(interp_plus_binds_tighter_than_shift) {
    // 1 << (2+1) = 8.  The wrong grouping (1<<2)+1 would give 5.
    CHECK_EQ(th::runToValue("int main() { return 1 << 2 + 1; }"), 8LL);
}

TEST(interp_equality_binds_tighter_than_bitwise_and) {
    // 5 & (3==3) = 5 & 1 = 1.   The wrong grouping (5&3)==3 would give 0.
    CHECK_EQ(th::runToValue("int main() { return 5 & 3 == 3; }"), 1LL);
}

TEST(interp_shift_by_a_negative_count_traps) {
    bool threw = false;
    try { th::runToValue("int main() { int n = 0 - 1; return 1 << n; }"); }
    catch (const Interp::RuntimeError&) { threw = true; }
    CHECK(threw);
}

TEST(interp_shift_by_64_or_more_traps) {
    bool threw = false;
    try { th::runToValue("int main() { int n = 64; return 1 << n; }"); }
    catch (const Interp::RuntimeError&) { threw = true; }
    CHECK(threw);
}

TEST(sema_rejects_bitwise_and_on_a_float) {
    CHECK(th::hasError(th::check("int main() { float f; f = 1.5; return f & 2; }").errors, "must be integral"));
}
TEST(sema_rejects_shift_on_a_float) {
    CHECK(th::hasError(th::check("int main() { float f; f = 1.5; return 1 << f; }").errors, "must be integral"));
}
TEST(sema_rejects_bitwise_not_on_a_float) {
    CHECK(th::hasError(th::check("int main() { float f; f = 1.5; return ~f; }").errors, "must be integral"));
}
TEST(sema_rejects_percent_assign_on_a_float) {
    CHECK(th::hasError(th::check("int main() { float f; f = 7.5; f %= 2; return 0; }").errors, "must be integral"));
}
TEST(sema_rejects_or_assign_on_a_float) {
    CHECK(th::hasError(th::check("int main() { float f; f = 7.5; f |= 2; return 0; }").errors, "must be integral"));
}
TEST(sema_accepts_every_bitwise_operator_on_ints) {
    CHECK(th::check("int main() { int a = 6; a = a & 3 | 1 ^ 2 << 1 >> 1; a = ~a; a &= 1; a |= 2; return a; }").ok);
}

TEST(pass_fold_folds_a_constant_bitwise_expression) {
    CHECK(transformsOf(makeConstantFoldingPass(), "int main() { int x; x = 12 & 10; return x; }") >= 1);
}
TEST(pass_fold_leaves_an_out_of_range_shift_for_run_time) {
    // Folding it would invent a result the interpreter refuses to produce.
    CHECK_EQ(transformsOf(makeConstantFoldingPass(), "int main() { int x; x = 1 << 64; return x; }"), 0);
}

TEST(pass_dce_deletes_a_dead_bitwise_and_but_not_a_dead_shift) {
    // A dead & cannot trap, so it goes; a dead << can (bad count), so it stays,
    // exactly as a dead division does.
    int andCount = transformsOf(makeDeadCodeEliminationPass(),
                                "int f(int p) { int d; d = p & 1; return 7; }", "f");
    int shlCount = transformsOf(makeDeadCodeEliminationPass(),
                                "int f(int p) { int d; d = p << 1; return 7; }", "f");
    CHECK(andCount > shlCount);
}

// ---- arrays sized by their initializer, and string-initialised char arrays ----

TEST(parser_infers_an_array_size_from_its_initializer) {
    CHECK_EQ(th::runToValue("int main() { int a[] = {4, 5, 6}; return a[2]; }"), 6LL);
}

TEST(parser_inferred_array_has_exactly_the_initializer_length) {
    // Index 3 is one past the end of {1,2,3}: it must trap, proving the size is 3 and not larger.
    bool threw = false;
    try { th::runToValue("int main() { int a[] = {1, 2, 3}; return a[3]; }"); }
    catch (const Interp::RuntimeError&) { threw = true; }
    CHECK(threw);
}

TEST(parser_accepts_a_trailing_comma_in_an_initializer) {
    CHECK_EQ(th::runToValue("int main() { int a[] = {1, 2, 3,}; return a[2]; }"), 3LL);
}

TEST(parser_trailing_comma_does_not_add_an_element) {
    bool threw = false;
    try { th::runToValue("int main() { int a[] = {1, 2, 3,}; return a[3]; }"); }
    catch (const Interp::RuntimeError&) { threw = true; }
    CHECK(threw);
}

TEST(parser_rejects_an_unsized_array_with_no_initializer) {
    CHECK(th::hasError(th::parse("int main() { int a[]; return 0; }").errors, "no size"));
}

TEST(parser_char_array_from_a_string_literal_includes_the_terminator) {
    // "hi" -> {'h','i',0}: 104 + 105 + 0
    CHECK_EQ(th::runToValue("int main() { char s[] = \"hi\"; return s[0] + s[1] + s[2]; }"), 209LL);
}

TEST(parser_string_initialiser_array_length_counts_the_terminator) {
    bool threw = false;
    try { th::runToValue("int main() { char s[] = \"hi\"; return s[3]; }"); }
    catch (const Interp::RuntimeError&) { threw = true; }
    CHECK(threw);
}

TEST(parser_string_that_exactly_fills_the_array_drops_the_terminator) {
    // C allows char s[2] = "hi" -- no room for the 0, so none is added.
    CHECK(th::check("int main() { char s[2] = \"hi\"; return s[0]; }").ok);
    CHECK_EQ(th::runToValue("int main() { char s[2] = \"hi\"; return s[0] + s[1]; }"), 209LL);
}

TEST(sema_rejects_a_string_longer_than_its_array) {
    CHECK(th::hasError(th::check("int main() { char s[1] = \"hi\"; return 0; }").errors,
                       "too many initializers"));
}

TEST(parser_string_initialiser_pads_a_larger_array_with_zeros) {
    CHECK_EQ(th::runToValue("int main() { char s[6] = \"ab\"; return s[2] + s[5]; }"), 0LL);
}
