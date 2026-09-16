// MiniC-Opt — indented AST printer (implementation)
// Owner: Member 1 (Front End)
#include "ast_printer.h"
#include <sstream>

namespace minic {

void AstPrinter::line(const std::string& text) {
    for (int i = 0; i < depth_; ++i) out_ << "  ";
    out_ << text << "\n";
}

std::string AstPrinter::typeSpecStr(const TypeSpec& t) {
    std::ostringstream ss;
    ss << toString(t.base);
    if (t.isArray) {
        ss << "[";
        if (t.arraySize >= 0) ss << t.arraySize;
        ss << "]";
    }
    return ss.str();
}

// ---- expressions -------------------------------------------------------

void AstPrinter::visit(IntLit& n)   { line("IntLit " + std::to_string(n.value)); }
void AstPrinter::visit(FloatLit& n) { line("FloatLit " + std::to_string(n.value)); }
void AstPrinter::visit(CharLit& n)  { line(std::string("CharLit '") + n.value + "'"); }
void AstPrinter::visit(IdentExpr& n){ line("IdentExpr " + n.name); }

void AstPrinter::visit(IndexExpr& n) {
    line("IndexExpr");
    acceptChild(n.base.get());
    acceptChild(n.index.get());
}

void AstPrinter::visit(CallExpr& n) {
    line("CallExpr " + n.callee + " (" + std::to_string(n.args.size()) + " args)");
    for (auto& a : n.args) acceptChild(a.get());
}

void AstPrinter::visit(UnaryExpr& n) {
    line(std::string("UnaryExpr ") + toString(n.op));
    acceptChild(n.operand.get());
}

void AstPrinter::visit(BinaryExpr& n) {
    line(std::string("BinaryExpr ") + toString(n.op));
    acceptChild(n.lhs.get());
    acceptChild(n.rhs.get());
}

void AstPrinter::visit(AssignExpr& n) {
    line(std::string("AssignExpr ") + toString(n.op));
    acceptChild(n.target.get());
    acceptChild(n.value.get());
}

void AstPrinter::visit(ConvertExpr& n) {
    line("ConvertExpr -> " + typeSpecStr(n.targetType));
    acceptChild(n.operand.get());
}

// ---- statements ----------------------------------------------------------

void AstPrinter::visit(ExprStmt& n) {
    line("ExprStmt");
    acceptChild(n.expr.get());
}

void AstPrinter::visit(CompoundStmt& n) {
    line("CompoundStmt (" + std::to_string(n.locals.size()) + " locals, "
         + std::to_string(n.stmts.size()) + " stmts)");
    for (auto& d : n.locals) acceptChild(d.get());
    for (auto& s : n.stmts)  acceptChild(s.get());
}

void AstPrinter::visit(IfStmt& n) {
    line("IfStmt");
    acceptChild(n.cond.get());
    acceptChild(n.thenStmt.get());
    if (n.elseStmt) acceptChild(n.elseStmt.get());
}

void AstPrinter::visit(WhileStmt& n) {
    line("WhileStmt");
    acceptChild(n.cond.get());
    acceptChild(n.body.get());
}

void AstPrinter::visit(ForStmt& n) {
    line("ForStmt");
    acceptChild(n.init.get());
    acceptChild(n.cond.get());
    acceptChild(n.step.get());
    acceptChild(n.body.get());
}

void AstPrinter::visit(DoStmt& n) {
    line("DoStmt");
    acceptChild(n.body.get());
    acceptChild(n.cond.get());
}

void AstPrinter::visit(ReturnStmt& n) {
    line("ReturnStmt");
    acceptChild(n.value.get());
}

void AstPrinter::visit(BreakStmt&)    { line("BreakStmt"); }
void AstPrinter::visit(ContinueStmt&) { line("ContinueStmt"); }

// ---- declarations ---------------------------------------------------------

void AstPrinter::visit(VarDecl& n) {
    line("VarDecl " + typeSpecStr(n.type) + " " + n.name);
    acceptChild(n.init.get());
    for (auto& e : n.initList) acceptChild(e.get());
}

void AstPrinter::visit(FuncDecl& n) {
    std::ostringstream sig;
    sig << "FuncDecl " << typeSpecStr(n.returnType) << " " << n.name << "(";
    for (size_t i = 0; i < n.params.size(); ++i) {
        if (i) sig << ", ";
        sig << typeSpecStr(n.params[i].type) << " " << n.params[i].name;
    }
    sig << ")";
    line(sig.str());
    acceptChild(n.body.get());
}

void AstPrinter::visit(Program& n) {
    line("Program (" + std::to_string(n.decls.size()) + " top-level decls)");
    for (auto& d : n.decls) acceptChild(d.get());
}

} // namespace minic
