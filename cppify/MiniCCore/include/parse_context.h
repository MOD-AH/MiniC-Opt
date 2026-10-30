// Cppify — declaration semantics shared by both parsers (roadmap 4.2)
//
// The hand-written recursive-descent parser and the Bison LALR parser differ
// in how they recognise syntax. What a declaration MEANS is not syntax: the
// struct layout, the typedef and enum tables (which also feed back into
// lexing: C cannot be parsed without knowing which names are types), the
// constant folding of array sizes and case labels, brace-initializer
// flattening and array-length inference. That lives here, once, so the two
// parsers produce the same AST by construction wherever they agree on the
// syntax, and the same diagnostics for these semantic checks.
#pragma once
#include "ast.h"
#include "diagnostic.h"
#include "token.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace minic {

class ParseContext {
public:
    explicit ParseContext(DiagnosticSink& sink);

    StructTable structs;
    std::unordered_map<std::string, TypeSpec>  typedefs;
    std::unordered_map<std::string, long long> enumConsts;

    // The type words of one declaration-specifier list, as seen in order.
    struct TypeWords {
        bool sawAny = false, sawUnsigned = false, sawShort = false, sawLong = false, sawIntWord = false;
        bool isStatic = false;
        TypeSpec t;
    };
    // Applies width and signedness to the base type (LP64, signed char).
    TypeSpec finishType(const TypeWords& w) const;

    // struct definitions: `struct Name {` opens one (they nest: a field's
    // type may define another), each field is added as soon as it is parsed
    // (so its diagnostics come out in source order), `}` closes it.
    int  structReference(const std::string& name, int line, int col);   // `struct Name` without a body
    int  beginStruct(const std::string& name, int line, int col);
    void addField(TypeSpec type, const Token& name);
    void finishStruct();

    void addEnumerator(const std::string& name, bool hasValue, long long value);
    void resetEnum() { nextEnum_ = 0; }

    // Constant integer expressions (array sizes, case labels, enumerators).
    bool evalConst(const Expr& e, long long& out) const;
    long long constInt(const Expr& e, int line, int col, const char* what);

    // One `[N]` (size null for `[]`): '[' at (line, col), the size
    // expression's first token at (exprLine, exprCol).
    struct Dim { std::unique_ptr<Expr> size; int line = 0, col = 0, exprLine = 0, exprCol = 0; };
    void applyDims(TypeSpec& t, std::vector<Dim>& dims, bool allowEmptyFirst);
    void applyDim(TypeSpec& t, Dim& d, bool first, bool allowEmptyFirst);
    // Drops a struct whose body failed to parse (it stays incomplete).
    void abandonStruct() { open_.pop_back(); }

    // A brace initializer as written, before it is flattened against a type.
    struct InitNode {
        std::unique_ptr<Expr> expr;
        std::vector<InitNode> kids;
        bool isList = false;
        int line = 0, col = 0;
    };
    // `= ...` of one declarator: nothing, a brace list, a string literal
    // filling a char array, or an expression.
    struct Initializer {
        enum Kind { NONE, LIST, STRING, EXPR } kind = NONE;
        InitNode list;
        Token str{Tok::STRING_LIT, "", 0, 0};
        std::unique_ptr<Expr> expr;
    };
    // `type` already has the declarator's dimensions applied (applyDims runs
    // before the initializer is parsed, so diagnostics stay in source order).
    std::unique_ptr<VarDecl> declarator(TypeSpec type, const Token& name, Initializer init);

    // `int a[]`, `int a[10]` and `int m[][4]` parameters all pass a pointer.
    Param parameter(TypeSpec type, const Token& name, std::vector<Dim>& dims);

    long long sizeofType(const TypeSpec& t) const { return structs.bytesOf(t); }

private:
    DiagnosticSink& sink_;
    long long nextEnum_ = 0;
    struct OpenStruct { int id; std::vector<FieldInfo> fields; int slotOff = 0, byteOff = 0, maxAlign = 1; };
    std::vector<OpenStruct> open_;

    void flattenInit(const TypeSpec& t, InitNode& init, std::vector<std::unique_ptr<Expr>>& out);
    void flattenOne(const TypeSpec& t, std::vector<InitNode>& kids, size_t& cur,
                    std::vector<std::unique_ptr<Expr>>& out, int line, int col);
    void flattenAgg(const TypeSpec& t, std::vector<InitNode>& kids, size_t& cur,
                    std::vector<std::unique_ptr<Expr>>& out, int line, int col);
    void zeroFill(const TypeSpec& t, std::vector<std::unique_ptr<Expr>>& out, int line, int col);
};

}  // namespace minic
