// MiniC-Opt — recursive-descent / Pratt parser
// Owner: Member 1 (Front End)
//
// One function per non-terminal in docs/grammar.ebnf, except the
// expression sub-grammar (assignment down to primary), which is
// precedence-climbing rather than one function per precedence level, per
// the grammar file's own note. On a parse error the parser records a
// Diagnostic, throws internally, and the nearest statement/declaration
// boundary catches it and resynchronises (panic mode) on ';' or '}' so a
// source file with several mistakes reports several diagnostics in one run
// instead of stopping at the first one.
#pragma once
#include "ast.h"
#include "token.h"
#include "diagnostic.h"
#include <vector>
#include <memory>

namespace minic {

class Parser {
public:
    Parser(std::vector<Token> tokens, std::string filename);

    // Top-level entry point. Never returns null — a file that fails to
    // parse at all still yields an (possibly empty) Program, with the
    // failure(s) recorded in errors().
    std::unique_ptr<Program> parseProgram();

    const std::vector<Diagnostic>& errors() const { return sink_.all(); }

private:
    std::vector<Token> toks_;
    size_t              pos_ = 0;
    std::string         file_;
    DiagnosticSink       sink_;

    // Thrown by expect()/errorHere() after the diagnostic is recorded;
    // caught at a statement/declaration boundary to drive panic-mode
    // recovery. Never escapes parseProgram().
    struct ParseError {};

    // ---- token stream helpers ----
    const Token& peek(size_t off = 0) const;
    const Token& previous() const;
    bool         check(Tok t) const;
    bool         isAtEnd() const;
    const Token& advance();
    bool         match(Tok t);
    const Token& expect(Tok t, const std::string& what);
    [[noreturn]] void errorHere(const std::string& msg);
    void synchronize();

    // ---- declarations (program.ebnf: declaration, var_decl, func_decl) ----
    void parseTopLevelDeclaration(std::vector<std::unique_ptr<Decl>>& out);
    TypeSpec parseTypeSpec();
    bool     atTypeSpecStart() const;
    // Parses the declarator list of ONE var_decl production — possibly
    // several comma-separated names sharing `type` — starting from the
    // already-consumed first declarator name `firstName`. The grammar's
    // single var_decl production is desugared here into one VarDecl node
    // per name (see the comment on VarDecl in ast.h).
    std::vector<std::unique_ptr<VarDecl>> parseVarDeclDeclarators(TypeSpec type, const Token& firstName);
    std::unique_ptr<VarDecl> parseOneDeclarator(TypeSpec type, const Token& nameTok);
    std::unique_ptr<FuncDecl> parseFuncDeclRest(TypeSpec returnType, const Token& nameTok);

    // ---- statements ----
    std::unique_ptr<CompoundStmt> parseCompoundStmt();
    std::unique_ptr<Stmt> parseStatement();
    std::unique_ptr<Stmt> parseExprStmt();
    std::unique_ptr<Stmt> parseIfStmt();
    std::unique_ptr<Stmt> parseWhileStmt();
    std::unique_ptr<Stmt> parseDoStmt();
    std::unique_ptr<Stmt> parseForStmt();
    std::unique_ptr<Stmt> parseReturnStmt();
    std::unique_ptr<Stmt> parseBreakStmt();
    std::unique_ptr<Stmt> parseContinueStmt();

    // ---- expressions, precedence-climbing (lowest precedence first) ----
    std::unique_ptr<Expr> parseExpression();
    std::unique_ptr<Expr> parseAssignment();
    std::unique_ptr<Expr> parseLogicalOr();
    std::unique_ptr<Expr> parseLogicalAnd();
    std::unique_ptr<Expr> parseEquality();
    std::unique_ptr<Expr> parseRelational();
    std::unique_ptr<Expr> parseAdditive();
    std::unique_ptr<Expr> parseMultiplicative();
    std::unique_ptr<Expr> parseUnary();
    std::unique_ptr<Expr> parsePostfix();
    std::unique_ptr<Expr> parsePrimary();
    std::vector<std::unique_ptr<Expr>> parseArgs();
};

} // namespace minic
