// MiniC-Opt — symbol table
// Owner: Member 2 (Background / Requirements Analyst)
//
// A stack of scoped hash tables, exactly as the report describes: each
// enterScope() pushes a fresh table, each exitScope() pops it, and lookup
// walks the stack from the innermost scope outward so an inner declaration
// shadows an outer one of the same name. Used internally by Sema (Step 3)
// to check the program; it does not need to outlive analysis — offsets and
// resolved types it computes are written back onto the AST itself (see
// VarDecl::storageOffset, Param::storageOffset and Expr::resolvedType in
// ast.h) for IR generation (Step 4) to read directly.
#pragma once
#include "ast.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace minic {

enum class SymbolKind { VARIABLE, PARAMETER, FUNCTION };

struct Symbol {
    SymbolKind  kind;
    TypeSpec    type;              // for FUNCTION: the return type
    std::string name;
    int         scopeLevel = 0;
    int         offset     = 0;    // meaningless for FUNCTION
    std::vector<TypeSpec> paramTypes;   // only populated for FUNCTION
};

class SymbolTable {
public:
    SymbolTable() { enterScope(); }   // the outermost (global) scope

    void enterScope() { scopes_.emplace_back(); }
    void exitScope()  { scopes_.pop_back(); }
    int  currentLevel() const { return static_cast<int>(scopes_.size()) - 1; }

    // Declares `sym` in the innermost scope. False (no insertion) means the
    // name is already declared in THIS scope — a redeclaration, which the
    // caller (Sema) reports; shadowing an outer scope is fine and not
    // reported.
    bool declare(Symbol sym) {
        auto& innermost = scopes_.back();
        if (innermost.count(sym.name)) return false;
        sym.scopeLevel = currentLevel();
        innermost.emplace(sym.name, std::move(sym));
        return true;
    }

    // Innermost-outward lookup. Null if not declared in any enclosing scope.
    Symbol* lookup(const std::string& name) {
        for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) return &found->second;
        }
        return nullptr;
    }

    // Hands out the next storage slot in the CURRENT function's frame.
    // Reset with resetOffsets() at the start of each FuncDecl (Sema does
    // this) so offsets are function-local, matching one activation record
    // per call rather than one global counter for the whole program.
    int nextOffset() { return offsetCounter_++; }
    void resetOffsets() { offsetCounter_ = 0; }

private:
    std::vector<std::unordered_map<std::string, Symbol>> scopes_;
    int offsetCounter_ = 0;
};

} // namespace minic
