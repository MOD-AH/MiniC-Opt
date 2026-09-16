// MiniC-Opt — indented AST printer, wired to `minic --dump-ast` in Step 2.
// Owner: Member 1 (Front End)
#pragma once
#include "ast.h"
#include <ostream>

namespace minic {

class AstPrinter : public Visitor {
public:
    explicit AstPrinter(std::ostream& out) : out_(out) {}

    void print(Program& p) { p.accept(*this); }

    void visit(IntLit&) override;
    void visit(FloatLit&) override;
    void visit(CharLit&) override;
    void visit(IdentExpr&) override;
    void visit(IndexExpr&) override;
    void visit(CallExpr&) override;
    void visit(UnaryExpr&) override;
    void visit(BinaryExpr&) override;
    void visit(AssignExpr&) override;
    void visit(ConvertExpr&) override;

    void visit(ExprStmt&) override;
    void visit(CompoundStmt&) override;
    void visit(IfStmt&) override;
    void visit(WhileStmt&) override;
    void visit(ForStmt&) override;
    void visit(DoStmt&) override;
    void visit(ReturnStmt&) override;
    void visit(BreakStmt&) override;
    void visit(ContinueStmt&) override;

    void visit(VarDecl&) override;
    void visit(FuncDecl&) override;
    void visit(Program&) override;

private:
    std::ostream& out_;
    int depth_ = 0;

    void line(const std::string& text);
    // Runs `fn` with depth_ incremented by one, then restores it — every
    // recursive descent into a child goes through this so indentation can
    // never drift out of sync if a visit() method returns early.
    template <typename Fn> void nested(Fn&& fn) {
        ++depth_; fn(); --depth_;
    }
    void acceptChild(Node* n) {
        if (n) nested([&] { n->accept(*this); });
    }
    static std::string typeSpecStr(const TypeSpec& t);
};

} // namespace minic
