// MiniC-Opt — Step 1 smoke test: hand-build the AST for tests/programs/gcd.c
// and print it, so the node shapes can be checked by eye against
// docs/grammar.ebnf before the parser (Step 2) exists to build trees itself.
//
//   int gcd(int a, int b) {
//       while (b != 0) {
//           int t = b;
//           b = a % b;
//           a = t;
//       }
//       return a;
//   }
//   int main() {
//       return gcd(1071, 462);
//   }
#include "ast.h"
#include "ast_printer.h"
#include <iostream>
#include <memory>

using namespace minic;

static TypeSpec intType() { return TypeSpec{BaseType::INT, false, -1}; }

int main() {
    Program prog;

    // ---- int gcd(int a, int b) { ... } ----
    auto gcd = std::make_unique<FuncDecl>(intType(), "gcd");
    gcd->params.push_back(Param{intType(), "a"});
    gcd->params.push_back(Param{intType(), "b"});

    auto body = std::make_unique<CompoundStmt>();

    // while (b != 0) { int t = b; b = a % b; a = t; }
    auto whileCond = std::make_unique<BinaryExpr>(
        BinOp::NE, std::make_unique<IdentExpr>("b"), std::make_unique<IntLit>(0));

    auto whileBody = std::make_unique<CompoundStmt>();
    auto tDecl = std::make_unique<VarDecl>(intType(), "t");
    tDecl->init = std::make_unique<IdentExpr>("b");
    whileBody->locals.push_back(std::move(tDecl));

    auto assignB = std::make_unique<AssignExpr>(
        AssignOp::ASSIGN, std::make_unique<IdentExpr>("b"),
        std::make_unique<BinaryExpr>(BinOp::MOD, std::make_unique<IdentExpr>("a"), std::make_unique<IdentExpr>("b")));
    whileBody->stmts.push_back(std::make_unique<ExprStmt>(std::move(assignB)));

    auto assignA = std::make_unique<AssignExpr>(
        AssignOp::ASSIGN, std::make_unique<IdentExpr>("a"), std::make_unique<IdentExpr>("t"));
    whileBody->stmts.push_back(std::make_unique<ExprStmt>(std::move(assignA)));

    body->stmts.push_back(std::make_unique<WhileStmt>(std::move(whileCond), std::move(whileBody)));
    body->stmts.push_back(std::make_unique<ReturnStmt>(std::make_unique<IdentExpr>("a")));

    gcd->body = std::move(body);
    prog.decls.push_back(std::move(gcd));

    // ---- int main() { return gcd(1071, 462); } ----
    auto mainFn = std::make_unique<FuncDecl>(intType(), "main");
    auto mainBody = std::make_unique<CompoundStmt>();
    auto call = std::make_unique<CallExpr>("gcd");
    call->args.push_back(std::make_unique<IntLit>(1071));
    call->args.push_back(std::make_unique<IntLit>(462));
    mainBody->stmts.push_back(std::make_unique<ReturnStmt>(std::move(call)));
    mainFn->body = std::move(mainBody);
    prog.decls.push_back(std::move(mainFn));

    // ---- print ----
    AstPrinter printer(std::cout);
    printer.print(prog);
    return 0;
}
