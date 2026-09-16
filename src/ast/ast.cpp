// MiniC-Opt — AST support (operator/type name tables)
// Owner: Member 1 (Front End)
#include "ast.h"

namespace minic {

const char* toString(BinOp op) {
    switch (op) {
        case BinOp::ADD: return "+";  case BinOp::SUB: return "-";
        case BinOp::MUL: return "*";  case BinOp::DIV: return "/";  case BinOp::MOD: return "%";
        case BinOp::LT:  return "<";  case BinOp::LE:  return "<="; case BinOp::GT:  return ">";
        case BinOp::GE:  return ">="; case BinOp::EQ:  return "=="; case BinOp::NE:  return "!=";
        case BinOp::LOGAND: return "&&"; case BinOp::LOGOR: return "||";
    }
    return "?";
}

const char* toString(UnOp op) {
    switch (op) {
        case UnOp::NEG: return "-";  case UnOp::POS: return "+";  case UnOp::NOT: return "!";
        case UnOp::PRE_INC: return "++(pre)"; case UnOp::PRE_DEC: return "--(pre)";
        case UnOp::POST_INC: return "++(post)"; case UnOp::POST_DEC: return "--(post)";
    }
    return "?";
}

const char* toString(AssignOp op) {
    switch (op) {
        case AssignOp::ASSIGN:       return "=";
        case AssignOp::PLUS_ASSIGN:  return "+=";
        case AssignOp::MINUS_ASSIGN: return "-=";
        case AssignOp::STAR_ASSIGN:  return "*=";
        case AssignOp::SLASH_ASSIGN: return "/=";
    }
    return "?";
}

const char* toString(BaseType t) {
    switch (t) {
        case BaseType::INT:   return "int";
        case BaseType::CHAR:  return "char";
        case BaseType::FLOAT: return "float";
        case BaseType::VOID:  return "void";
    }
    return "?";
}

} // namespace minic
