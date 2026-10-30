// A Bison parser, made by GNU Bison 3.8.2.

// Skeleton implementation for Bison LALR(1) parsers in C++

// Copyright (C) 2002-2015, 2018-2021 Free Software Foundation, Inc.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

// As a special exception, you may create a larger work that contains
// part or all of the Bison parser skeleton and distribute that work
// under terms of your choice, so long as that work isn't itself a
// parser generator using the skeleton or a modified version thereof
// as a parser skeleton.  Alternatively, if you modify or redistribute
// the parser skeleton itself, you may (at your option) remove this
// special exception, which will cause the skeleton and the resulting
// Bison output files to be licensed under the GNU General Public
// License without this special exception.

// This special exception was added by the Free Software Foundation in
// version 2.2 of Bison.

// DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
// especially those whose name start with YY_ or yy_.  They are
// private implementation details that can be changed or removed.

// "%code top" blocks.
#line 45 "src/parser/parser_bison.y"

// Bison's skeleton counts errors in a variable it never reads.
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif

#line 46 "src/parser/parser_bison.cpp"




#include "parser_bison.hpp"


// Unqualified %code blocks.
#line 109 "src/parser/parser_bison.y"

namespace minic { namespace bisonp {
Parser::symbol_type yylex(State& st);
}}
namespace {
using namespace minic;
template <class T> T* at(T* n, const minic::bisonp::Pos& p) { n->line = p.line; n->col = p.col; return n; }
template <class T> T* at(T* n, const Node& from) { n->line = from.line; n->col = from.col; return n; }
std::unique_ptr<Expr> binary(BinOp op, std::unique_ptr<Expr> l, std::unique_ptr<Expr> r, const minic::bisonp::Pos& p) {
    return std::unique_ptr<Expr>(at(new BinaryExpr(op, std::move(l), std::move(r)), p));
}
std::unique_ptr<Expr> unary(UnOp op, std::unique_ptr<Expr> e, const minic::bisonp::Pos& p) {
    return std::unique_ptr<Expr>(at(new UnaryExpr(op, std::move(e)), p));
}
// The effect of one prefix_item (codes as assigned in the grammar).
void apply(ParseContext::TypeWords& w, int code) {
    switch (code) {
        case 0: w.isStatic = true; return;
        case 1: return;   // const, extern: no meaning here
        case 2: w.sawUnsigned = true; break;
        case 4: w.sawLong = true; break;
        case 5: w.sawShort = true; break;
        default: break;
    }
    w.sawIntWord = w.sawAny = true;
    w.t.base = BaseType::INT;
}
char decodeChar(const std::string& lex) {
    if (lex.size() >= 3 && lex[1] == '\\') {
        switch (lex[2]) {
            case 'n': return '\n';  case 't': return '\t';  case 'r': return '\r';  case '0': return '\0';
            case '\\': return '\\'; case '\'': return '\'';
            default: return lex[2];
        }
    }
    return lex.size() >= 2 ? lex[1] : '\0';
}
}  // namespace

#line 95 "src/parser/parser_bison.cpp"


#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> // FIXME: INFRINGES ON USER NAME SPACE.
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif


// Whether we are compiled with exception support.
#ifndef YY_EXCEPTIONS
# if defined __GNUC__ && !defined __EXCEPTIONS
#  define YY_EXCEPTIONS 0
# else
#  define YY_EXCEPTIONS 1
# endif
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K].location)
/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

# ifndef YYLLOC_DEFAULT
#  define YYLLOC_DEFAULT(Current, Rhs, N)                               \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).begin  = YYRHSLOC (Rhs, 1).begin;                   \
          (Current).end    = YYRHSLOC (Rhs, N).end;                     \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).begin = (Current).end = YYRHSLOC (Rhs, 0).end;      \
        }                                                               \
    while (false)
# endif


// Enable debugging if requested.
#if YYDEBUG

// A pseudo ostream that takes yydebug_ into account.
# define YYCDEBUG if (yydebug_) (*yycdebug_)

# define YY_SYMBOL_PRINT(Title, Symbol)         \
  do {                                          \
    if (yydebug_)                               \
    {                                           \
      *yycdebug_ << Title << ' ';               \
      yy_print_ (*yycdebug_, Symbol);           \
      *yycdebug_ << '\n';                       \
    }                                           \
  } while (false)

# define YY_REDUCE_PRINT(Rule)          \
  do {                                  \
    if (yydebug_)                       \
      yy_reduce_print_ (Rule);          \
  } while (false)

# define YY_STACK_PRINT()               \
  do {                                  \
    if (yydebug_)                       \
      yy_stack_print_ ();                \
  } while (false)

#else // !YYDEBUG

# define YYCDEBUG if (false) std::cerr
# define YY_SYMBOL_PRINT(Title, Symbol)  YY_USE (Symbol)
# define YY_REDUCE_PRINT(Rule)           static_cast<void> (0)
# define YY_STACK_PRINT()                static_cast<void> (0)

#endif // !YYDEBUG

#define yyerrok         (yyerrstatus_ = 0)
#define yyclearin       (yyla.clear ())

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYRECOVERING()  (!!yyerrstatus_)

#line 33 "src/parser/parser_bison.y"
namespace minic { namespace bisonp {
#line 188 "src/parser/parser_bison.cpp"

  /// Build a parser object.
  Parser::Parser (State& st_yyarg)
#if YYDEBUG
    : yydebug_ (false),
      yycdebug_ (&std::cerr),
#else
    :
#endif
      st (st_yyarg)
  {}

  Parser::~Parser ()
  {}

  Parser::syntax_error::~syntax_error () YY_NOEXCEPT YY_NOTHROW
  {}

  /*---------.
  | symbol.  |
  `---------*/



  // by_state.
  Parser::by_state::by_state () YY_NOEXCEPT
    : state (empty_state)
  {}

  Parser::by_state::by_state (const by_state& that) YY_NOEXCEPT
    : state (that.state)
  {}

  void
  Parser::by_state::clear () YY_NOEXCEPT
  {
    state = empty_state;
  }

  void
  Parser::by_state::move (by_state& that)
  {
    state = that.state;
    that.clear ();
  }

  Parser::by_state::by_state (state_type s) YY_NOEXCEPT
    : state (s)
  {}

  Parser::symbol_kind_type
  Parser::by_state::kind () const YY_NOEXCEPT
  {
    if (state == empty_state)
      return symbol_kind::S_YYEMPTY;
    else
      return YY_CAST (symbol_kind_type, yystos_[+state]);
  }

  Parser::stack_symbol_type::stack_symbol_type ()
  {}

  Parser::stack_symbol_type::stack_symbol_type (YY_RVREF (stack_symbol_type) that)
    : super_type (YY_MOVE (that.state), YY_MOVE (that.location))
  {
    switch (that.kind ())
    {
      case symbol_kind::S_assign_op: // assign_op
        value.YY_MOVE_OR_COPY< AssignOp > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_named: // named
        value.YY_MOVE_OR_COPY< DeclHead > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_param: // param
        value.YY_MOVE_OR_COPY< ParamInfo > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_dim: // dim
        value.YY_MOVE_OR_COPY< ParseContext::Dim > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_init_node: // init_node
        value.YY_MOVE_OR_COPY< ParseContext::InitNode > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_init_opt: // init_opt
        value.YY_MOVE_OR_COPY< ParseContext::Initializer > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_decl_specs: // decl_specs
      case symbol_kind::S_prefix_list: // prefix_list
      case symbol_kind::S_type_end: // type_end
      case symbol_kind::S_struct_spec: // struct_spec
      case symbol_kind::S_enum_spec: // enum_spec
        value.YY_MOVE_OR_COPY< ParseContext::TypeWords > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_IDENT: // "an identifier"
      case symbol_kind::S_TYPEDEF_NAME: // "a type name"
      case symbol_kind::S_INT_LIT: // "an integer"
      case symbol_kind::S_FLOAT_LIT: // "a number"
      case symbol_kind::S_CHAR_LIT: // "a character"
      case symbol_kind::S_STRING_LIT: // "a string"
      case symbol_kind::S_INT: // "'int'"
      case symbol_kind::S_CHAR: // "'char'"
      case symbol_kind::S_FLOAT: // "'float'"
      case symbol_kind::S_VOID: // "'void'"
      case symbol_kind::S_LONG: // "'long'"
      case symbol_kind::S_SHORT: // "'short'"
      case symbol_kind::S_UNSIGNED: // "'unsigned'"
      case symbol_kind::S_SIGNED: // "'signed'"
      case symbol_kind::S_DOUBLE: // "'double'"
      case symbol_kind::S_CONST: // "'const'"
      case symbol_kind::S_STATIC: // "'static'"
      case symbol_kind::S_EXTERN: // "'extern'"
      case symbol_kind::S_IF: // "'if'"
      case symbol_kind::S_ELSE: // "'else'"
      case symbol_kind::S_WHILE: // "'while'"
      case symbol_kind::S_FOR: // "'for'"
      case symbol_kind::S_DO: // "'do'"
      case symbol_kind::S_RETURN: // "'return'"
      case symbol_kind::S_BREAK: // "'break'"
      case symbol_kind::S_CONTINUE: // "'continue'"
      case symbol_kind::S_STRUCT: // "'struct'"
      case symbol_kind::S_TYPEDEF: // "'typedef'"
      case symbol_kind::S_ENUM: // "'enum'"
      case symbol_kind::S_SIZEOF: // "'sizeof'"
      case symbol_kind::S_SWITCH: // "'switch'"
      case symbol_kind::S_CASE: // "'case'"
      case symbol_kind::S_DEFAULT: // "'default'"
      case symbol_kind::S_GOTO: // "'goto'"
      case symbol_kind::S_PLUS: // "'+'"
      case symbol_kind::S_MINUS: // "'-'"
      case symbol_kind::S_STAR: // "'*'"
      case symbol_kind::S_SLASH: // "'/'"
      case symbol_kind::S_PERCENT: // "'%'"
      case symbol_kind::S_ASSIGN: // "'='"
      case symbol_kind::S_PLUS_ASSIGN: // "'+='"
      case symbol_kind::S_MINUS_ASSIGN: // "'-='"
      case symbol_kind::S_STAR_ASSIGN: // "'*='"
      case symbol_kind::S_SLASH_ASSIGN: // "'/='"
      case symbol_kind::S_PERCENT_ASSIGN: // "'%='"
      case symbol_kind::S_AND_ASSIGN: // "'&='"
      case symbol_kind::S_OR_ASSIGN: // "'|='"
      case symbol_kind::S_XOR_ASSIGN: // "'^='"
      case symbol_kind::S_SHL_ASSIGN: // "'<<='"
      case symbol_kind::S_SHR_ASSIGN: // "'>>='"
      case symbol_kind::S_EQ: // "'=='"
      case symbol_kind::S_NE: // "'!='"
      case symbol_kind::S_LT: // "'<'"
      case symbol_kind::S_LE: // "'<='"
      case symbol_kind::S_GT: // "'>'"
      case symbol_kind::S_GE: // "'>='"
      case symbol_kind::S_ANDAND: // "'&&'"
      case symbol_kind::S_OROR: // "'||'"
      case symbol_kind::S_NOT: // "'!'"
      case symbol_kind::S_BITAND: // "'&'"
      case symbol_kind::S_BITOR: // "'|'"
      case symbol_kind::S_BITXOR: // "'^'"
      case symbol_kind::S_BITNOT: // "'~'"
      case symbol_kind::S_SHL: // "'<<'"
      case symbol_kind::S_SHR: // "'>>'"
      case symbol_kind::S_INC: // "'++'"
      case symbol_kind::S_DEC: // "'--'"
      case symbol_kind::S_DOT: // "'.'"
      case symbol_kind::S_ARROW: // "'->'"
      case symbol_kind::S_QUESTION: // "'?'"
      case symbol_kind::S_COLON: // "':'"
      case symbol_kind::S_LPAREN: // "'('"
      case symbol_kind::S_RPAREN: // "')'"
      case symbol_kind::S_LBRACE: // "'{'"
      case symbol_kind::S_RBRACE: // "'}'"
      case symbol_kind::S_LBRACKET: // "'['"
      case symbol_kind::S_RBRACKET: // "']'"
      case symbol_kind::S_COMMA: // "','"
      case symbol_kind::S_SEMI: // "';'"
      case symbol_kind::S_LEXERROR: // "an invalid token"
        value.YY_MOVE_OR_COPY< Token > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_type_name: // type_name
      case symbol_kind::S_115_5: // @5
        value.YY_MOVE_OR_COPY< TypeSpec > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_decl_pushed: // decl_pushed
      case symbol_kind::S_prefix_item: // prefix_item
      case symbol_kind::S_94_2: // @2
      case symbol_kind::S_stars: // stars
      case symbol_kind::S_stars1: // stars1
        value.YY_MOVE_OR_COPY< int > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_compound: // compound
        value.YY_MOVE_OR_COPY< std::unique_ptr<CompoundStmt> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_expr_opt: // expr_opt
      case symbol_kind::S_expression: // expression
      case symbol_kind::S_assignment: // assignment
      case symbol_kind::S_conditional: // conditional
      case symbol_kind::S_logical_or: // logical_or
      case symbol_kind::S_logical_and: // logical_and
      case symbol_kind::S_bit_or: // bit_or
      case symbol_kind::S_bit_xor: // bit_xor
      case symbol_kind::S_bit_and: // bit_and
      case symbol_kind::S_equality: // equality
      case symbol_kind::S_relational: // relational
      case symbol_kind::S_shift: // shift
      case symbol_kind::S_additive: // additive
      case symbol_kind::S_multiplicative: // multiplicative
      case symbol_kind::S_cast: // cast
      case symbol_kind::S_unary: // unary
      case symbol_kind::S_postfix: // postfix
      case symbol_kind::S_primary: // primary
        value.YY_MOVE_OR_COPY< std::unique_ptr<Expr> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_statement: // statement
        value.YY_MOVE_OR_COPY< std::unique_ptr<Stmt> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_var_declarator: // var_declarator
        value.YY_MOVE_OR_COPY< std::unique_ptr<VarDecl> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_params: // params
      case symbol_kind::S_param_list: // param_list
        value.YY_MOVE_OR_COPY< std::vector<ParamInfo> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_dims: // dims
        value.YY_MOVE_OR_COPY< std::vector<ParseContext::Dim> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_init_list: // init_list
      case symbol_kind::S_init_items: // init_items
        value.YY_MOVE_OR_COPY< std::vector<ParseContext::InitNode> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_args: // args
        value.YY_MOVE_OR_COPY< std::vector<std::unique_ptr<Expr>> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_block_items: // block_items
      case symbol_kind::S_block_item: // block_item
      case symbol_kind::S_local_decl: // local_decl
        value.YY_MOVE_OR_COPY< std::vector<std::unique_ptr<Stmt>> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_var_declarators: // var_declarators
        value.YY_MOVE_OR_COPY< std::vector<std::unique_ptr<VarDecl>> > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

#if 201103L <= YY_CPLUSPLUS
    // that is emptied.
    that.state = empty_state;
#endif
  }

  Parser::stack_symbol_type::stack_symbol_type (state_type s, YY_MOVE_REF (symbol_type) that)
    : super_type (s, YY_MOVE (that.location))
  {
    switch (that.kind ())
    {
      case symbol_kind::S_assign_op: // assign_op
        value.move< AssignOp > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_named: // named
        value.move< DeclHead > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_param: // param
        value.move< ParamInfo > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_dim: // dim
        value.move< ParseContext::Dim > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_init_node: // init_node
        value.move< ParseContext::InitNode > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_init_opt: // init_opt
        value.move< ParseContext::Initializer > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_decl_specs: // decl_specs
      case symbol_kind::S_prefix_list: // prefix_list
      case symbol_kind::S_type_end: // type_end
      case symbol_kind::S_struct_spec: // struct_spec
      case symbol_kind::S_enum_spec: // enum_spec
        value.move< ParseContext::TypeWords > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_IDENT: // "an identifier"
      case symbol_kind::S_TYPEDEF_NAME: // "a type name"
      case symbol_kind::S_INT_LIT: // "an integer"
      case symbol_kind::S_FLOAT_LIT: // "a number"
      case symbol_kind::S_CHAR_LIT: // "a character"
      case symbol_kind::S_STRING_LIT: // "a string"
      case symbol_kind::S_INT: // "'int'"
      case symbol_kind::S_CHAR: // "'char'"
      case symbol_kind::S_FLOAT: // "'float'"
      case symbol_kind::S_VOID: // "'void'"
      case symbol_kind::S_LONG: // "'long'"
      case symbol_kind::S_SHORT: // "'short'"
      case symbol_kind::S_UNSIGNED: // "'unsigned'"
      case symbol_kind::S_SIGNED: // "'signed'"
      case symbol_kind::S_DOUBLE: // "'double'"
      case symbol_kind::S_CONST: // "'const'"
      case symbol_kind::S_STATIC: // "'static'"
      case symbol_kind::S_EXTERN: // "'extern'"
      case symbol_kind::S_IF: // "'if'"
      case symbol_kind::S_ELSE: // "'else'"
      case symbol_kind::S_WHILE: // "'while'"
      case symbol_kind::S_FOR: // "'for'"
      case symbol_kind::S_DO: // "'do'"
      case symbol_kind::S_RETURN: // "'return'"
      case symbol_kind::S_BREAK: // "'break'"
      case symbol_kind::S_CONTINUE: // "'continue'"
      case symbol_kind::S_STRUCT: // "'struct'"
      case symbol_kind::S_TYPEDEF: // "'typedef'"
      case symbol_kind::S_ENUM: // "'enum'"
      case symbol_kind::S_SIZEOF: // "'sizeof'"
      case symbol_kind::S_SWITCH: // "'switch'"
      case symbol_kind::S_CASE: // "'case'"
      case symbol_kind::S_DEFAULT: // "'default'"
      case symbol_kind::S_GOTO: // "'goto'"
      case symbol_kind::S_PLUS: // "'+'"
      case symbol_kind::S_MINUS: // "'-'"
      case symbol_kind::S_STAR: // "'*'"
      case symbol_kind::S_SLASH: // "'/'"
      case symbol_kind::S_PERCENT: // "'%'"
      case symbol_kind::S_ASSIGN: // "'='"
      case symbol_kind::S_PLUS_ASSIGN: // "'+='"
      case symbol_kind::S_MINUS_ASSIGN: // "'-='"
      case symbol_kind::S_STAR_ASSIGN: // "'*='"
      case symbol_kind::S_SLASH_ASSIGN: // "'/='"
      case symbol_kind::S_PERCENT_ASSIGN: // "'%='"
      case symbol_kind::S_AND_ASSIGN: // "'&='"
      case symbol_kind::S_OR_ASSIGN: // "'|='"
      case symbol_kind::S_XOR_ASSIGN: // "'^='"
      case symbol_kind::S_SHL_ASSIGN: // "'<<='"
      case symbol_kind::S_SHR_ASSIGN: // "'>>='"
      case symbol_kind::S_EQ: // "'=='"
      case symbol_kind::S_NE: // "'!='"
      case symbol_kind::S_LT: // "'<'"
      case symbol_kind::S_LE: // "'<='"
      case symbol_kind::S_GT: // "'>'"
      case symbol_kind::S_GE: // "'>='"
      case symbol_kind::S_ANDAND: // "'&&'"
      case symbol_kind::S_OROR: // "'||'"
      case symbol_kind::S_NOT: // "'!'"
      case symbol_kind::S_BITAND: // "'&'"
      case symbol_kind::S_BITOR: // "'|'"
      case symbol_kind::S_BITXOR: // "'^'"
      case symbol_kind::S_BITNOT: // "'~'"
      case symbol_kind::S_SHL: // "'<<'"
      case symbol_kind::S_SHR: // "'>>'"
      case symbol_kind::S_INC: // "'++'"
      case symbol_kind::S_DEC: // "'--'"
      case symbol_kind::S_DOT: // "'.'"
      case symbol_kind::S_ARROW: // "'->'"
      case symbol_kind::S_QUESTION: // "'?'"
      case symbol_kind::S_COLON: // "':'"
      case symbol_kind::S_LPAREN: // "'('"
      case symbol_kind::S_RPAREN: // "')'"
      case symbol_kind::S_LBRACE: // "'{'"
      case symbol_kind::S_RBRACE: // "'}'"
      case symbol_kind::S_LBRACKET: // "'['"
      case symbol_kind::S_RBRACKET: // "']'"
      case symbol_kind::S_COMMA: // "','"
      case symbol_kind::S_SEMI: // "';'"
      case symbol_kind::S_LEXERROR: // "an invalid token"
        value.move< Token > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_type_name: // type_name
      case symbol_kind::S_115_5: // @5
        value.move< TypeSpec > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_decl_pushed: // decl_pushed
      case symbol_kind::S_prefix_item: // prefix_item
      case symbol_kind::S_94_2: // @2
      case symbol_kind::S_stars: // stars
      case symbol_kind::S_stars1: // stars1
        value.move< int > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_compound: // compound
        value.move< std::unique_ptr<CompoundStmt> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_expr_opt: // expr_opt
      case symbol_kind::S_expression: // expression
      case symbol_kind::S_assignment: // assignment
      case symbol_kind::S_conditional: // conditional
      case symbol_kind::S_logical_or: // logical_or
      case symbol_kind::S_logical_and: // logical_and
      case symbol_kind::S_bit_or: // bit_or
      case symbol_kind::S_bit_xor: // bit_xor
      case symbol_kind::S_bit_and: // bit_and
      case symbol_kind::S_equality: // equality
      case symbol_kind::S_relational: // relational
      case symbol_kind::S_shift: // shift
      case symbol_kind::S_additive: // additive
      case symbol_kind::S_multiplicative: // multiplicative
      case symbol_kind::S_cast: // cast
      case symbol_kind::S_unary: // unary
      case symbol_kind::S_postfix: // postfix
      case symbol_kind::S_primary: // primary
        value.move< std::unique_ptr<Expr> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_statement: // statement
        value.move< std::unique_ptr<Stmt> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_var_declarator: // var_declarator
        value.move< std::unique_ptr<VarDecl> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_params: // params
      case symbol_kind::S_param_list: // param_list
        value.move< std::vector<ParamInfo> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_dims: // dims
        value.move< std::vector<ParseContext::Dim> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_init_list: // init_list
      case symbol_kind::S_init_items: // init_items
        value.move< std::vector<ParseContext::InitNode> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_args: // args
        value.move< std::vector<std::unique_ptr<Expr>> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_block_items: // block_items
      case symbol_kind::S_block_item: // block_item
      case symbol_kind::S_local_decl: // local_decl
        value.move< std::vector<std::unique_ptr<Stmt>> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_var_declarators: // var_declarators
        value.move< std::vector<std::unique_ptr<VarDecl>> > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

    // that is emptied.
    that.kind_ = symbol_kind::S_YYEMPTY;
  }

#if YY_CPLUSPLUS < 201103L
  Parser::stack_symbol_type&
  Parser::stack_symbol_type::operator= (const stack_symbol_type& that)
  {
    state = that.state;
    switch (that.kind ())
    {
      case symbol_kind::S_assign_op: // assign_op
        value.copy< AssignOp > (that.value);
        break;

      case symbol_kind::S_named: // named
        value.copy< DeclHead > (that.value);
        break;

      case symbol_kind::S_param: // param
        value.copy< ParamInfo > (that.value);
        break;

      case symbol_kind::S_dim: // dim
        value.copy< ParseContext::Dim > (that.value);
        break;

      case symbol_kind::S_init_node: // init_node
        value.copy< ParseContext::InitNode > (that.value);
        break;

      case symbol_kind::S_init_opt: // init_opt
        value.copy< ParseContext::Initializer > (that.value);
        break;

      case symbol_kind::S_decl_specs: // decl_specs
      case symbol_kind::S_prefix_list: // prefix_list
      case symbol_kind::S_type_end: // type_end
      case symbol_kind::S_struct_spec: // struct_spec
      case symbol_kind::S_enum_spec: // enum_spec
        value.copy< ParseContext::TypeWords > (that.value);
        break;

      case symbol_kind::S_IDENT: // "an identifier"
      case symbol_kind::S_TYPEDEF_NAME: // "a type name"
      case symbol_kind::S_INT_LIT: // "an integer"
      case symbol_kind::S_FLOAT_LIT: // "a number"
      case symbol_kind::S_CHAR_LIT: // "a character"
      case symbol_kind::S_STRING_LIT: // "a string"
      case symbol_kind::S_INT: // "'int'"
      case symbol_kind::S_CHAR: // "'char'"
      case symbol_kind::S_FLOAT: // "'float'"
      case symbol_kind::S_VOID: // "'void'"
      case symbol_kind::S_LONG: // "'long'"
      case symbol_kind::S_SHORT: // "'short'"
      case symbol_kind::S_UNSIGNED: // "'unsigned'"
      case symbol_kind::S_SIGNED: // "'signed'"
      case symbol_kind::S_DOUBLE: // "'double'"
      case symbol_kind::S_CONST: // "'const'"
      case symbol_kind::S_STATIC: // "'static'"
      case symbol_kind::S_EXTERN: // "'extern'"
      case symbol_kind::S_IF: // "'if'"
      case symbol_kind::S_ELSE: // "'else'"
      case symbol_kind::S_WHILE: // "'while'"
      case symbol_kind::S_FOR: // "'for'"
      case symbol_kind::S_DO: // "'do'"
      case symbol_kind::S_RETURN: // "'return'"
      case symbol_kind::S_BREAK: // "'break'"
      case symbol_kind::S_CONTINUE: // "'continue'"
      case symbol_kind::S_STRUCT: // "'struct'"
      case symbol_kind::S_TYPEDEF: // "'typedef'"
      case symbol_kind::S_ENUM: // "'enum'"
      case symbol_kind::S_SIZEOF: // "'sizeof'"
      case symbol_kind::S_SWITCH: // "'switch'"
      case symbol_kind::S_CASE: // "'case'"
      case symbol_kind::S_DEFAULT: // "'default'"
      case symbol_kind::S_GOTO: // "'goto'"
      case symbol_kind::S_PLUS: // "'+'"
      case symbol_kind::S_MINUS: // "'-'"
      case symbol_kind::S_STAR: // "'*'"
      case symbol_kind::S_SLASH: // "'/'"
      case symbol_kind::S_PERCENT: // "'%'"
      case symbol_kind::S_ASSIGN: // "'='"
      case symbol_kind::S_PLUS_ASSIGN: // "'+='"
      case symbol_kind::S_MINUS_ASSIGN: // "'-='"
      case symbol_kind::S_STAR_ASSIGN: // "'*='"
      case symbol_kind::S_SLASH_ASSIGN: // "'/='"
      case symbol_kind::S_PERCENT_ASSIGN: // "'%='"
      case symbol_kind::S_AND_ASSIGN: // "'&='"
      case symbol_kind::S_OR_ASSIGN: // "'|='"
      case symbol_kind::S_XOR_ASSIGN: // "'^='"
      case symbol_kind::S_SHL_ASSIGN: // "'<<='"
      case symbol_kind::S_SHR_ASSIGN: // "'>>='"
      case symbol_kind::S_EQ: // "'=='"
      case symbol_kind::S_NE: // "'!='"
      case symbol_kind::S_LT: // "'<'"
      case symbol_kind::S_LE: // "'<='"
      case symbol_kind::S_GT: // "'>'"
      case symbol_kind::S_GE: // "'>='"
      case symbol_kind::S_ANDAND: // "'&&'"
      case symbol_kind::S_OROR: // "'||'"
      case symbol_kind::S_NOT: // "'!'"
      case symbol_kind::S_BITAND: // "'&'"
      case symbol_kind::S_BITOR: // "'|'"
      case symbol_kind::S_BITXOR: // "'^'"
      case symbol_kind::S_BITNOT: // "'~'"
      case symbol_kind::S_SHL: // "'<<'"
      case symbol_kind::S_SHR: // "'>>'"
      case symbol_kind::S_INC: // "'++'"
      case symbol_kind::S_DEC: // "'--'"
      case symbol_kind::S_DOT: // "'.'"
      case symbol_kind::S_ARROW: // "'->'"
      case symbol_kind::S_QUESTION: // "'?'"
      case symbol_kind::S_COLON: // "':'"
      case symbol_kind::S_LPAREN: // "'('"
      case symbol_kind::S_RPAREN: // "')'"
      case symbol_kind::S_LBRACE: // "'{'"
      case symbol_kind::S_RBRACE: // "'}'"
      case symbol_kind::S_LBRACKET: // "'['"
      case symbol_kind::S_RBRACKET: // "']'"
      case symbol_kind::S_COMMA: // "','"
      case symbol_kind::S_SEMI: // "';'"
      case symbol_kind::S_LEXERROR: // "an invalid token"
        value.copy< Token > (that.value);
        break;

      case symbol_kind::S_type_name: // type_name
      case symbol_kind::S_115_5: // @5
        value.copy< TypeSpec > (that.value);
        break;

      case symbol_kind::S_decl_pushed: // decl_pushed
      case symbol_kind::S_prefix_item: // prefix_item
      case symbol_kind::S_94_2: // @2
      case symbol_kind::S_stars: // stars
      case symbol_kind::S_stars1: // stars1
        value.copy< int > (that.value);
        break;

      case symbol_kind::S_compound: // compound
        value.copy< std::unique_ptr<CompoundStmt> > (that.value);
        break;

      case symbol_kind::S_expr_opt: // expr_opt
      case symbol_kind::S_expression: // expression
      case symbol_kind::S_assignment: // assignment
      case symbol_kind::S_conditional: // conditional
      case symbol_kind::S_logical_or: // logical_or
      case symbol_kind::S_logical_and: // logical_and
      case symbol_kind::S_bit_or: // bit_or
      case symbol_kind::S_bit_xor: // bit_xor
      case symbol_kind::S_bit_and: // bit_and
      case symbol_kind::S_equality: // equality
      case symbol_kind::S_relational: // relational
      case symbol_kind::S_shift: // shift
      case symbol_kind::S_additive: // additive
      case symbol_kind::S_multiplicative: // multiplicative
      case symbol_kind::S_cast: // cast
      case symbol_kind::S_unary: // unary
      case symbol_kind::S_postfix: // postfix
      case symbol_kind::S_primary: // primary
        value.copy< std::unique_ptr<Expr> > (that.value);
        break;

      case symbol_kind::S_statement: // statement
        value.copy< std::unique_ptr<Stmt> > (that.value);
        break;

      case symbol_kind::S_var_declarator: // var_declarator
        value.copy< std::unique_ptr<VarDecl> > (that.value);
        break;

      case symbol_kind::S_params: // params
      case symbol_kind::S_param_list: // param_list
        value.copy< std::vector<ParamInfo> > (that.value);
        break;

      case symbol_kind::S_dims: // dims
        value.copy< std::vector<ParseContext::Dim> > (that.value);
        break;

      case symbol_kind::S_init_list: // init_list
      case symbol_kind::S_init_items: // init_items
        value.copy< std::vector<ParseContext::InitNode> > (that.value);
        break;

      case symbol_kind::S_args: // args
        value.copy< std::vector<std::unique_ptr<Expr>> > (that.value);
        break;

      case symbol_kind::S_block_items: // block_items
      case symbol_kind::S_block_item: // block_item
      case symbol_kind::S_local_decl: // local_decl
        value.copy< std::vector<std::unique_ptr<Stmt>> > (that.value);
        break;

      case symbol_kind::S_var_declarators: // var_declarators
        value.copy< std::vector<std::unique_ptr<VarDecl>> > (that.value);
        break;

      default:
        break;
    }

    location = that.location;
    return *this;
  }

  Parser::stack_symbol_type&
  Parser::stack_symbol_type::operator= (stack_symbol_type& that)
  {
    state = that.state;
    switch (that.kind ())
    {
      case symbol_kind::S_assign_op: // assign_op
        value.move< AssignOp > (that.value);
        break;

      case symbol_kind::S_named: // named
        value.move< DeclHead > (that.value);
        break;

      case symbol_kind::S_param: // param
        value.move< ParamInfo > (that.value);
        break;

      case symbol_kind::S_dim: // dim
        value.move< ParseContext::Dim > (that.value);
        break;

      case symbol_kind::S_init_node: // init_node
        value.move< ParseContext::InitNode > (that.value);
        break;

      case symbol_kind::S_init_opt: // init_opt
        value.move< ParseContext::Initializer > (that.value);
        break;

      case symbol_kind::S_decl_specs: // decl_specs
      case symbol_kind::S_prefix_list: // prefix_list
      case symbol_kind::S_type_end: // type_end
      case symbol_kind::S_struct_spec: // struct_spec
      case symbol_kind::S_enum_spec: // enum_spec
        value.move< ParseContext::TypeWords > (that.value);
        break;

      case symbol_kind::S_IDENT: // "an identifier"
      case symbol_kind::S_TYPEDEF_NAME: // "a type name"
      case symbol_kind::S_INT_LIT: // "an integer"
      case symbol_kind::S_FLOAT_LIT: // "a number"
      case symbol_kind::S_CHAR_LIT: // "a character"
      case symbol_kind::S_STRING_LIT: // "a string"
      case symbol_kind::S_INT: // "'int'"
      case symbol_kind::S_CHAR: // "'char'"
      case symbol_kind::S_FLOAT: // "'float'"
      case symbol_kind::S_VOID: // "'void'"
      case symbol_kind::S_LONG: // "'long'"
      case symbol_kind::S_SHORT: // "'short'"
      case symbol_kind::S_UNSIGNED: // "'unsigned'"
      case symbol_kind::S_SIGNED: // "'signed'"
      case symbol_kind::S_DOUBLE: // "'double'"
      case symbol_kind::S_CONST: // "'const'"
      case symbol_kind::S_STATIC: // "'static'"
      case symbol_kind::S_EXTERN: // "'extern'"
      case symbol_kind::S_IF: // "'if'"
      case symbol_kind::S_ELSE: // "'else'"
      case symbol_kind::S_WHILE: // "'while'"
      case symbol_kind::S_FOR: // "'for'"
      case symbol_kind::S_DO: // "'do'"
      case symbol_kind::S_RETURN: // "'return'"
      case symbol_kind::S_BREAK: // "'break'"
      case symbol_kind::S_CONTINUE: // "'continue'"
      case symbol_kind::S_STRUCT: // "'struct'"
      case symbol_kind::S_TYPEDEF: // "'typedef'"
      case symbol_kind::S_ENUM: // "'enum'"
      case symbol_kind::S_SIZEOF: // "'sizeof'"
      case symbol_kind::S_SWITCH: // "'switch'"
      case symbol_kind::S_CASE: // "'case'"
      case symbol_kind::S_DEFAULT: // "'default'"
      case symbol_kind::S_GOTO: // "'goto'"
      case symbol_kind::S_PLUS: // "'+'"
      case symbol_kind::S_MINUS: // "'-'"
      case symbol_kind::S_STAR: // "'*'"
      case symbol_kind::S_SLASH: // "'/'"
      case symbol_kind::S_PERCENT: // "'%'"
      case symbol_kind::S_ASSIGN: // "'='"
      case symbol_kind::S_PLUS_ASSIGN: // "'+='"
      case symbol_kind::S_MINUS_ASSIGN: // "'-='"
      case symbol_kind::S_STAR_ASSIGN: // "'*='"
      case symbol_kind::S_SLASH_ASSIGN: // "'/='"
      case symbol_kind::S_PERCENT_ASSIGN: // "'%='"
      case symbol_kind::S_AND_ASSIGN: // "'&='"
      case symbol_kind::S_OR_ASSIGN: // "'|='"
      case symbol_kind::S_XOR_ASSIGN: // "'^='"
      case symbol_kind::S_SHL_ASSIGN: // "'<<='"
      case symbol_kind::S_SHR_ASSIGN: // "'>>='"
      case symbol_kind::S_EQ: // "'=='"
      case symbol_kind::S_NE: // "'!='"
      case symbol_kind::S_LT: // "'<'"
      case symbol_kind::S_LE: // "'<='"
      case symbol_kind::S_GT: // "'>'"
      case symbol_kind::S_GE: // "'>='"
      case symbol_kind::S_ANDAND: // "'&&'"
      case symbol_kind::S_OROR: // "'||'"
      case symbol_kind::S_NOT: // "'!'"
      case symbol_kind::S_BITAND: // "'&'"
      case symbol_kind::S_BITOR: // "'|'"
      case symbol_kind::S_BITXOR: // "'^'"
      case symbol_kind::S_BITNOT: // "'~'"
      case symbol_kind::S_SHL: // "'<<'"
      case symbol_kind::S_SHR: // "'>>'"
      case symbol_kind::S_INC: // "'++'"
      case symbol_kind::S_DEC: // "'--'"
      case symbol_kind::S_DOT: // "'.'"
      case symbol_kind::S_ARROW: // "'->'"
      case symbol_kind::S_QUESTION: // "'?'"
      case symbol_kind::S_COLON: // "':'"
      case symbol_kind::S_LPAREN: // "'('"
      case symbol_kind::S_RPAREN: // "')'"
      case symbol_kind::S_LBRACE: // "'{'"
      case symbol_kind::S_RBRACE: // "'}'"
      case symbol_kind::S_LBRACKET: // "'['"
      case symbol_kind::S_RBRACKET: // "']'"
      case symbol_kind::S_COMMA: // "','"
      case symbol_kind::S_SEMI: // "';'"
      case symbol_kind::S_LEXERROR: // "an invalid token"
        value.move< Token > (that.value);
        break;

      case symbol_kind::S_type_name: // type_name
      case symbol_kind::S_115_5: // @5
        value.move< TypeSpec > (that.value);
        break;

      case symbol_kind::S_decl_pushed: // decl_pushed
      case symbol_kind::S_prefix_item: // prefix_item
      case symbol_kind::S_94_2: // @2
      case symbol_kind::S_stars: // stars
      case symbol_kind::S_stars1: // stars1
        value.move< int > (that.value);
        break;

      case symbol_kind::S_compound: // compound
        value.move< std::unique_ptr<CompoundStmt> > (that.value);
        break;

      case symbol_kind::S_expr_opt: // expr_opt
      case symbol_kind::S_expression: // expression
      case symbol_kind::S_assignment: // assignment
      case symbol_kind::S_conditional: // conditional
      case symbol_kind::S_logical_or: // logical_or
      case symbol_kind::S_logical_and: // logical_and
      case symbol_kind::S_bit_or: // bit_or
      case symbol_kind::S_bit_xor: // bit_xor
      case symbol_kind::S_bit_and: // bit_and
      case symbol_kind::S_equality: // equality
      case symbol_kind::S_relational: // relational
      case symbol_kind::S_shift: // shift
      case symbol_kind::S_additive: // additive
      case symbol_kind::S_multiplicative: // multiplicative
      case symbol_kind::S_cast: // cast
      case symbol_kind::S_unary: // unary
      case symbol_kind::S_postfix: // postfix
      case symbol_kind::S_primary: // primary
        value.move< std::unique_ptr<Expr> > (that.value);
        break;

      case symbol_kind::S_statement: // statement
        value.move< std::unique_ptr<Stmt> > (that.value);
        break;

      case symbol_kind::S_var_declarator: // var_declarator
        value.move< std::unique_ptr<VarDecl> > (that.value);
        break;

      case symbol_kind::S_params: // params
      case symbol_kind::S_param_list: // param_list
        value.move< std::vector<ParamInfo> > (that.value);
        break;

      case symbol_kind::S_dims: // dims
        value.move< std::vector<ParseContext::Dim> > (that.value);
        break;

      case symbol_kind::S_init_list: // init_list
      case symbol_kind::S_init_items: // init_items
        value.move< std::vector<ParseContext::InitNode> > (that.value);
        break;

      case symbol_kind::S_args: // args
        value.move< std::vector<std::unique_ptr<Expr>> > (that.value);
        break;

      case symbol_kind::S_block_items: // block_items
      case symbol_kind::S_block_item: // block_item
      case symbol_kind::S_local_decl: // local_decl
        value.move< std::vector<std::unique_ptr<Stmt>> > (that.value);
        break;

      case symbol_kind::S_var_declarators: // var_declarators
        value.move< std::vector<std::unique_ptr<VarDecl>> > (that.value);
        break;

      default:
        break;
    }

    location = that.location;
    // that is emptied.
    that.state = empty_state;
    return *this;
  }
#endif

  template <typename Base>
  void
  Parser::yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const
  {
    if (yymsg)
      YY_SYMBOL_PRINT (yymsg, yysym);
  }

#if YYDEBUG
  template <typename Base>
  void
  Parser::yy_print_ (std::ostream& yyo, const basic_symbol<Base>& yysym) const
  {
    std::ostream& yyoutput = yyo;
    YY_USE (yyoutput);
    if (yysym.empty ())
      yyo << "empty symbol";
    else
      {
        symbol_kind_type yykind = yysym.kind ();
        yyo << (yykind < YYNTOKENS ? "token" : "nterm")
            << ' ' << yysym.name () << " ("
            << yysym.location << ": ";
        YY_USE (yykind);
        yyo << ')';
      }
  }
#endif

  void
  Parser::yypush_ (const char* m, YY_MOVE_REF (stack_symbol_type) sym)
  {
    if (m)
      YY_SYMBOL_PRINT (m, sym);
    yystack_.push (YY_MOVE (sym));
  }

  void
  Parser::yypush_ (const char* m, state_type s, YY_MOVE_REF (symbol_type) sym)
  {
#if 201103L <= YY_CPLUSPLUS
    yypush_ (m, stack_symbol_type (s, std::move (sym)));
#else
    stack_symbol_type ss (s, sym);
    yypush_ (m, ss);
#endif
  }

  void
  Parser::yypop_ (int n) YY_NOEXCEPT
  {
    yystack_.pop (n);
  }

#if YYDEBUG
  std::ostream&
  Parser::debug_stream () const
  {
    return *yycdebug_;
  }

  void
  Parser::set_debug_stream (std::ostream& o)
  {
    yycdebug_ = &o;
  }


  Parser::debug_level_type
  Parser::debug_level () const
  {
    return yydebug_;
  }

  void
  Parser::set_debug_level (debug_level_type l)
  {
    yydebug_ = l;
  }
#endif // YYDEBUG

  Parser::state_type
  Parser::yy_lr_goto_state_ (state_type yystate, int yysym)
  {
    int yyr = yypgoto_[yysym - YYNTOKENS] + yystate;
    if (0 <= yyr && yyr <= yylast_ && yycheck_[yyr] == yystate)
      return yytable_[yyr];
    else
      return yydefgoto_[yysym - YYNTOKENS];
  }

  bool
  Parser::yy_pact_value_is_default_ (int yyvalue) YY_NOEXCEPT
  {
    return yyvalue == yypact_ninf_;
  }

  bool
  Parser::yy_table_value_is_error_ (int yyvalue) YY_NOEXCEPT
  {
    return yyvalue == yytable_ninf_;
  }

  int
  Parser::operator() ()
  {
    return parse ();
  }

  int
  Parser::parse ()
  {
    int yyn;
    /// Length of the RHS of the rule being reduced.
    int yylen = 0;

    // Error handling.
    int yynerrs_ = 0;
    int yyerrstatus_ = 0;

    /// The lookahead symbol.
    symbol_type yyla;

    /// The locations where the error started and ended.
    stack_symbol_type yyerror_range[3];

    /// The return value of parse ().
    int yyresult;

#if YY_EXCEPTIONS
    try
#endif // YY_EXCEPTIONS
      {
    YYCDEBUG << "Starting parse\n";


    /* Initialize the stack.  The initial state will be set in
       yynewstate, since the latter expects the semantical and the
       location values to have been already stored, initialize these
       stacks with a primary value.  */
    yystack_.clear ();
    yypush_ (YY_NULLPTR, 0, YY_MOVE (yyla));

  /*-----------------------------------------------.
  | yynewstate -- push a new symbol on the stack.  |
  `-----------------------------------------------*/
  yynewstate:
    YYCDEBUG << "Entering state " << int (yystack_[0].state) << '\n';
    YY_STACK_PRINT ();

    // Accept?
    if (yystack_[0].state == yyfinal_)
      YYACCEPT;

    goto yybackup;


  /*-----------.
  | yybackup.  |
  `-----------*/
  yybackup:
    // Try to take a decision without lookahead.
    yyn = yypact_[+yystack_[0].state];
    if (yy_pact_value_is_default_ (yyn))
      goto yydefault;

    // Read a lookahead token.
    if (yyla.empty ())
      {
        YYCDEBUG << "Reading a token\n";
#if YY_EXCEPTIONS
        try
#endif // YY_EXCEPTIONS
          {
            symbol_type yylookahead (yylex (st));
            yyla.move (yylookahead);
          }
#if YY_EXCEPTIONS
        catch (const syntax_error& yyexc)
          {
            YYCDEBUG << "Caught exception: " << yyexc.what() << '\n';
            error (yyexc);
            goto yyerrlab1;
          }
#endif // YY_EXCEPTIONS
      }
    YY_SYMBOL_PRINT ("Next token is", yyla);

    if (yyla.kind () == symbol_kind::S_YYerror)
    {
      // The scanner already issued an error message, process directly
      // to error recovery.  But do not keep the error token as
      // lookahead, it is too special and may lead us to an endless
      // loop in error recovery. */
      yyla.kind_ = symbol_kind::S_YYUNDEF;
      goto yyerrlab1;
    }

    /* If the proper action on seeing token YYLA.TYPE is to reduce or
       to detect an error, take that action.  */
    yyn += yyla.kind ();
    if (yyn < 0 || yylast_ < yyn || yycheck_[yyn] != yyla.kind ())
      {
        goto yydefault;
      }

    // Reduce or error.
    yyn = yytable_[yyn];
    if (yyn <= 0)
      {
        if (yy_table_value_is_error_ (yyn))
          goto yyerrlab;
        yyn = -yyn;
        goto yyreduce;
      }

    // Count tokens shifted since error; after three, turn off error status.
    if (yyerrstatus_)
      --yyerrstatus_;

    // Shift the lookahead token.
    yypush_ ("Shifting", state_type (yyn), YY_MOVE (yyla));
    goto yynewstate;


  /*-----------------------------------------------------------.
  | yydefault -- do the default action for the current state.  |
  `-----------------------------------------------------------*/
  yydefault:
    yyn = yydefact_[+yystack_[0].state];
    if (yyn == 0)
      goto yyerrlab;
    goto yyreduce;


  /*-----------------------------.
  | yyreduce -- do a reduction.  |
  `-----------------------------*/
  yyreduce:
    yylen = yyr2_[yyn];
    {
      stack_symbol_type yylhs;
      yylhs.state = yy_lr_goto_state_ (yystack_[yylen].state, yyr1_[yyn]);
      /* Variants are always initialized to an empty instance of the
         correct type. The default '$$ = $1' action is NOT applied
         when using variants.  */
      switch (yyr1_[yyn])
    {
      case symbol_kind::S_assign_op: // assign_op
        yylhs.value.emplace< AssignOp > ();
        break;

      case symbol_kind::S_named: // named
        yylhs.value.emplace< DeclHead > ();
        break;

      case symbol_kind::S_param: // param
        yylhs.value.emplace< ParamInfo > ();
        break;

      case symbol_kind::S_dim: // dim
        yylhs.value.emplace< ParseContext::Dim > ();
        break;

      case symbol_kind::S_init_node: // init_node
        yylhs.value.emplace< ParseContext::InitNode > ();
        break;

      case symbol_kind::S_init_opt: // init_opt
        yylhs.value.emplace< ParseContext::Initializer > ();
        break;

      case symbol_kind::S_decl_specs: // decl_specs
      case symbol_kind::S_prefix_list: // prefix_list
      case symbol_kind::S_type_end: // type_end
      case symbol_kind::S_struct_spec: // struct_spec
      case symbol_kind::S_enum_spec: // enum_spec
        yylhs.value.emplace< ParseContext::TypeWords > ();
        break;

      case symbol_kind::S_IDENT: // "an identifier"
      case symbol_kind::S_TYPEDEF_NAME: // "a type name"
      case symbol_kind::S_INT_LIT: // "an integer"
      case symbol_kind::S_FLOAT_LIT: // "a number"
      case symbol_kind::S_CHAR_LIT: // "a character"
      case symbol_kind::S_STRING_LIT: // "a string"
      case symbol_kind::S_INT: // "'int'"
      case symbol_kind::S_CHAR: // "'char'"
      case symbol_kind::S_FLOAT: // "'float'"
      case symbol_kind::S_VOID: // "'void'"
      case symbol_kind::S_LONG: // "'long'"
      case symbol_kind::S_SHORT: // "'short'"
      case symbol_kind::S_UNSIGNED: // "'unsigned'"
      case symbol_kind::S_SIGNED: // "'signed'"
      case symbol_kind::S_DOUBLE: // "'double'"
      case symbol_kind::S_CONST: // "'const'"
      case symbol_kind::S_STATIC: // "'static'"
      case symbol_kind::S_EXTERN: // "'extern'"
      case symbol_kind::S_IF: // "'if'"
      case symbol_kind::S_ELSE: // "'else'"
      case symbol_kind::S_WHILE: // "'while'"
      case symbol_kind::S_FOR: // "'for'"
      case symbol_kind::S_DO: // "'do'"
      case symbol_kind::S_RETURN: // "'return'"
      case symbol_kind::S_BREAK: // "'break'"
      case symbol_kind::S_CONTINUE: // "'continue'"
      case symbol_kind::S_STRUCT: // "'struct'"
      case symbol_kind::S_TYPEDEF: // "'typedef'"
      case symbol_kind::S_ENUM: // "'enum'"
      case symbol_kind::S_SIZEOF: // "'sizeof'"
      case symbol_kind::S_SWITCH: // "'switch'"
      case symbol_kind::S_CASE: // "'case'"
      case symbol_kind::S_DEFAULT: // "'default'"
      case symbol_kind::S_GOTO: // "'goto'"
      case symbol_kind::S_PLUS: // "'+'"
      case symbol_kind::S_MINUS: // "'-'"
      case symbol_kind::S_STAR: // "'*'"
      case symbol_kind::S_SLASH: // "'/'"
      case symbol_kind::S_PERCENT: // "'%'"
      case symbol_kind::S_ASSIGN: // "'='"
      case symbol_kind::S_PLUS_ASSIGN: // "'+='"
      case symbol_kind::S_MINUS_ASSIGN: // "'-='"
      case symbol_kind::S_STAR_ASSIGN: // "'*='"
      case symbol_kind::S_SLASH_ASSIGN: // "'/='"
      case symbol_kind::S_PERCENT_ASSIGN: // "'%='"
      case symbol_kind::S_AND_ASSIGN: // "'&='"
      case symbol_kind::S_OR_ASSIGN: // "'|='"
      case symbol_kind::S_XOR_ASSIGN: // "'^='"
      case symbol_kind::S_SHL_ASSIGN: // "'<<='"
      case symbol_kind::S_SHR_ASSIGN: // "'>>='"
      case symbol_kind::S_EQ: // "'=='"
      case symbol_kind::S_NE: // "'!='"
      case symbol_kind::S_LT: // "'<'"
      case symbol_kind::S_LE: // "'<='"
      case symbol_kind::S_GT: // "'>'"
      case symbol_kind::S_GE: // "'>='"
      case symbol_kind::S_ANDAND: // "'&&'"
      case symbol_kind::S_OROR: // "'||'"
      case symbol_kind::S_NOT: // "'!'"
      case symbol_kind::S_BITAND: // "'&'"
      case symbol_kind::S_BITOR: // "'|'"
      case symbol_kind::S_BITXOR: // "'^'"
      case symbol_kind::S_BITNOT: // "'~'"
      case symbol_kind::S_SHL: // "'<<'"
      case symbol_kind::S_SHR: // "'>>'"
      case symbol_kind::S_INC: // "'++'"
      case symbol_kind::S_DEC: // "'--'"
      case symbol_kind::S_DOT: // "'.'"
      case symbol_kind::S_ARROW: // "'->'"
      case symbol_kind::S_QUESTION: // "'?'"
      case symbol_kind::S_COLON: // "':'"
      case symbol_kind::S_LPAREN: // "'('"
      case symbol_kind::S_RPAREN: // "')'"
      case symbol_kind::S_LBRACE: // "'{'"
      case symbol_kind::S_RBRACE: // "'}'"
      case symbol_kind::S_LBRACKET: // "'['"
      case symbol_kind::S_RBRACKET: // "']'"
      case symbol_kind::S_COMMA: // "','"
      case symbol_kind::S_SEMI: // "';'"
      case symbol_kind::S_LEXERROR: // "an invalid token"
        yylhs.value.emplace< Token > ();
        break;

      case symbol_kind::S_type_name: // type_name
      case symbol_kind::S_115_5: // @5
        yylhs.value.emplace< TypeSpec > ();
        break;

      case symbol_kind::S_decl_pushed: // decl_pushed
      case symbol_kind::S_prefix_item: // prefix_item
      case symbol_kind::S_94_2: // @2
      case symbol_kind::S_stars: // stars
      case symbol_kind::S_stars1: // stars1
        yylhs.value.emplace< int > ();
        break;

      case symbol_kind::S_compound: // compound
        yylhs.value.emplace< std::unique_ptr<CompoundStmt> > ();
        break;

      case symbol_kind::S_expr_opt: // expr_opt
      case symbol_kind::S_expression: // expression
      case symbol_kind::S_assignment: // assignment
      case symbol_kind::S_conditional: // conditional
      case symbol_kind::S_logical_or: // logical_or
      case symbol_kind::S_logical_and: // logical_and
      case symbol_kind::S_bit_or: // bit_or
      case symbol_kind::S_bit_xor: // bit_xor
      case symbol_kind::S_bit_and: // bit_and
      case symbol_kind::S_equality: // equality
      case symbol_kind::S_relational: // relational
      case symbol_kind::S_shift: // shift
      case symbol_kind::S_additive: // additive
      case symbol_kind::S_multiplicative: // multiplicative
      case symbol_kind::S_cast: // cast
      case symbol_kind::S_unary: // unary
      case symbol_kind::S_postfix: // postfix
      case symbol_kind::S_primary: // primary
        yylhs.value.emplace< std::unique_ptr<Expr> > ();
        break;

      case symbol_kind::S_statement: // statement
        yylhs.value.emplace< std::unique_ptr<Stmt> > ();
        break;

      case symbol_kind::S_var_declarator: // var_declarator
        yylhs.value.emplace< std::unique_ptr<VarDecl> > ();
        break;

      case symbol_kind::S_params: // params
      case symbol_kind::S_param_list: // param_list
        yylhs.value.emplace< std::vector<ParamInfo> > ();
        break;

      case symbol_kind::S_dims: // dims
        yylhs.value.emplace< std::vector<ParseContext::Dim> > ();
        break;

      case symbol_kind::S_init_list: // init_list
      case symbol_kind::S_init_items: // init_items
        yylhs.value.emplace< std::vector<ParseContext::InitNode> > ();
        break;

      case symbol_kind::S_args: // args
        yylhs.value.emplace< std::vector<std::unique_ptr<Expr>> > ();
        break;

      case symbol_kind::S_block_items: // block_items
      case symbol_kind::S_block_item: // block_item
      case symbol_kind::S_local_decl: // local_decl
        yylhs.value.emplace< std::vector<std::unique_ptr<Stmt>> > ();
        break;

      case symbol_kind::S_var_declarators: // var_declarators
        yylhs.value.emplace< std::vector<std::unique_ptr<VarDecl>> > ();
        break;

      default:
        break;
    }


      // Default location.
      {
        stack_type::slice range (yystack_, yylen);
        YYLLOC_DEFAULT (yylhs.location, range, yylen);
        yyerror_range[1].location = yylhs.location;
      }

      // Perform the reduction.
      YY_REDUCE_PRINT (yyn);
#if YY_EXCEPTIONS
      try
#endif // YY_EXCEPTIONS
        {
          switch (yyn)
            {
  case 6: // top_decl: decl_pushed var_declarators "';'"
#line 201 "src/parser/parser_bison.y"
                                       {
          st.declType.resize(yystack_[2].value.as < int > ());
          for (auto& d : yystack_[1].value.as < std::vector<std::unique_ptr<VarDecl>> > ()) st.prog->decls.push_back(std::move(d));
      }
#line 1530 "src/parser/parser_bison.cpp"
    break;

  case 7: // top_decl: decl_pushed named "'('" params "')'" compound
#line 205 "src/parser/parser_bison.y"
                                                      {
          // A function definition. Every parameter must be named.
          TypeSpec rt = st.ctx.finishType(st.declType[yystack_[5].value.as < int > ()]);
          rt.ptrDepth += yystack_[4].value.as < DeclHead > ().stars;
          st.declType.resize(yystack_[5].value.as < int > ());
          auto fn = std::make_unique<FuncDecl>(rt, yystack_[4].value.as < DeclHead > ().name.lexeme);
          fn->line = yystack_[4].value.as < DeclHead > ().name.line; fn->col = yystack_[4].value.as < DeclHead > ().name.col;
          std::vector<ParamInfo>& ps = yystack_[2].value.as < std::vector<ParamInfo> > ();
          bool voidOnly = ps.size() == 1 && ps[0].plainVoid;
          if (!voidOnly)
              for (auto& p : ps) {
                  if (!p.named) {
                      st.sink.add(p.at.line, p.at.col, "expected a parameter name but found '" + p.found + "'");
                      continue;
                  }
                  fn->params.push_back(st.ctx.parameter(p.type, p.name, p.dims));
              }
          fn->body = std::move(yystack_[0].value.as < std::unique_ptr<CompoundStmt> > ());
          st.prog->decls.push_back(std::move(fn));
      }
#line 1555 "src/parser/parser_bison.cpp"
    break;

  case 8: // top_decl: decl_pushed named "'('" params "')'" "';'"
#line 225 "src/parser/parser_bison.y"
                                                  {
          // A prototype: functions are collected before any body is checked,
          // so it carries nothing this compiler needs and is discarded.
          st.declType.resize(yystack_[5].value.as < int > ());
      }
#line 1565 "src/parser/parser_bison.cpp"
    break;

  case 9: // top_decl: error "';'"
#line 230 "src/parser/parser_bison.y"
                   { yyerrok; st.declType.clear(); }
#line 1571 "src/parser/parser_bison.cpp"
    break;

  case 10: // top_decl: error "'}'"
#line 231 "src/parser/parser_bison.y"
                   { yyerrok; st.declType.clear(); }
#line 1577 "src/parser/parser_bison.cpp"
    break;

  case 11: // decl_pushed: decl_specs
#line 240 "src/parser/parser_bison.y"
                 { yylhs.value.as < int > () = static_cast<int>(st.declType.size()); st.declType.push_back(yystack_[0].value.as < ParseContext::TypeWords > ()); }
#line 1583 "src/parser/parser_bison.cpp"
    break;

  case 12: // decl_specs: type_end
#line 246 "src/parser/parser_bison.y"
                  { yylhs.value.as < ParseContext::TypeWords > () = yystack_[0].value.as < ParseContext::TypeWords > (); }
#line 1589 "src/parser/parser_bison.cpp"
    break;

  case 13: // decl_specs: prefix_list
#line 247 "src/parser/parser_bison.y"
                  {
          if (!yystack_[0].value.as < ParseContext::TypeWords > ().sawAny) {
              st.sink.add(st.last.line, st.last.col,
                          "expected a type (int, char, float, void, struct or a typedef name)");
              st.quietNext = true;
              YYERROR;
          }
          yylhs.value.as < ParseContext::TypeWords > () = yystack_[0].value.as < ParseContext::TypeWords > ();
      }
#line 1603 "src/parser/parser_bison.cpp"
    break;

  case 14: // decl_specs: prefix_list type_end
#line 256 "src/parser/parser_bison.y"
                           {
          yylhs.value.as < ParseContext::TypeWords > () = yystack_[1].value.as < ParseContext::TypeWords > ();
          yylhs.value.as < ParseContext::TypeWords > ().t = yystack_[0].value.as < ParseContext::TypeWords > ().t;
          yylhs.value.as < ParseContext::TypeWords > ().sawAny = true;
      }
#line 1613 "src/parser/parser_bison.cpp"
    break;

  case 15: // prefix_list: prefix_item
#line 265 "src/parser/parser_bison.y"
                              { yylhs.value.as < ParseContext::TypeWords > () = ParseContext::TypeWords{}; apply(yylhs.value.as < ParseContext::TypeWords > (), yystack_[0].value.as < int > ()); }
#line 1619 "src/parser/parser_bison.cpp"
    break;

  case 16: // prefix_list: prefix_list prefix_item
#line 266 "src/parser/parser_bison.y"
                              { yylhs.value.as < ParseContext::TypeWords > () = yystack_[1].value.as < ParseContext::TypeWords > (); apply(yylhs.value.as < ParseContext::TypeWords > (), yystack_[0].value.as < int > ()); }
#line 1625 "src/parser/parser_bison.cpp"
    break;

  case 17: // prefix_item: "'static'"
#line 270 "src/parser/parser_bison.y"
               { yylhs.value.as < int > () = 0; }
#line 1631 "src/parser/parser_bison.cpp"
    break;

  case 18: // prefix_item: "'const'"
#line 271 "src/parser/parser_bison.y"
               { yylhs.value.as < int > () = 1; }
#line 1637 "src/parser/parser_bison.cpp"
    break;

  case 19: // prefix_item: "'extern'"
#line 272 "src/parser/parser_bison.y"
               { yylhs.value.as < int > () = 1; }
#line 1643 "src/parser/parser_bison.cpp"
    break;

  case 20: // prefix_item: "'unsigned'"
#line 273 "src/parser/parser_bison.y"
               { yylhs.value.as < int > () = 2; }
#line 1649 "src/parser/parser_bison.cpp"
    break;

  case 21: // prefix_item: "'signed'"
#line 274 "src/parser/parser_bison.y"
               { yylhs.value.as < int > () = 3; }
#line 1655 "src/parser/parser_bison.cpp"
    break;

  case 22: // prefix_item: "'long'"
#line 275 "src/parser/parser_bison.y"
               { yylhs.value.as < int > () = 4; }
#line 1661 "src/parser/parser_bison.cpp"
    break;

  case 23: // prefix_item: "'short'"
#line 276 "src/parser/parser_bison.y"
               { yylhs.value.as < int > () = 5; }
#line 1667 "src/parser/parser_bison.cpp"
    break;

  case 24: // prefix_item: "'int'"
#line 277 "src/parser/parser_bison.y"
               { yylhs.value.as < int > () = 6; }
#line 1673 "src/parser/parser_bison.cpp"
    break;

  case 25: // type_end: "'char'"
#line 282 "src/parser/parser_bison.y"
                   { yylhs.value.as < ParseContext::TypeWords > ().t.base = BaseType::CHAR;  yylhs.value.as < ParseContext::TypeWords > ().sawAny = true; }
#line 1679 "src/parser/parser_bison.cpp"
    break;

  case 26: // type_end: "'float'"
#line 283 "src/parser/parser_bison.y"
                   { yylhs.value.as < ParseContext::TypeWords > ().t.base = BaseType::FLOAT; yylhs.value.as < ParseContext::TypeWords > ().sawAny = true; }
#line 1685 "src/parser/parser_bison.cpp"
    break;

  case 27: // type_end: "'double'"
#line 284 "src/parser/parser_bison.y"
                   { yylhs.value.as < ParseContext::TypeWords > ().t.base = BaseType::FLOAT; yylhs.value.as < ParseContext::TypeWords > ().t.isDouble = true; yylhs.value.as < ParseContext::TypeWords > ().sawAny = true; }
#line 1691 "src/parser/parser_bison.cpp"
    break;

  case 28: // type_end: "'void'"
#line 285 "src/parser/parser_bison.y"
                   { yylhs.value.as < ParseContext::TypeWords > ().t.base = BaseType::VOID;  yylhs.value.as < ParseContext::TypeWords > ().sawAny = true; }
#line 1697 "src/parser/parser_bison.cpp"
    break;

  case 29: // type_end: struct_spec
#line 286 "src/parser/parser_bison.y"
                   { yylhs.value.as < ParseContext::TypeWords > () = yystack_[0].value.as < ParseContext::TypeWords > (); }
#line 1703 "src/parser/parser_bison.cpp"
    break;

  case 30: // type_end: enum_spec
#line 287 "src/parser/parser_bison.y"
                   { yylhs.value.as < ParseContext::TypeWords > () = yystack_[0].value.as < ParseContext::TypeWords > (); }
#line 1709 "src/parser/parser_bison.cpp"
    break;

  case 31: // type_end: "a type name"
#line 288 "src/parser/parser_bison.y"
                   { yylhs.value.as < ParseContext::TypeWords > ().t = st.ctx.typedefs[yystack_[0].value.as < Token > ().lexeme]; yylhs.value.as < ParseContext::TypeWords > ().sawAny = true; }
#line 1715 "src/parser/parser_bison.cpp"
    break;

  case 32: // struct_spec: "'struct'" "an identifier"
#line 292 "src/parser/parser_bison.y"
                   {
          yylhs.value.as < ParseContext::TypeWords > ().t = TypeSpec{};
          yylhs.value.as < ParseContext::TypeWords > ().t.base = BaseType::STRUCT;
          yylhs.value.as < ParseContext::TypeWords > ().t.structId = st.ctx.structReference(yystack_[0].value.as < Token > ().lexeme, yystack_[0].value.as < Token > ().line, yystack_[0].value.as < Token > ().col);
          yylhs.value.as < ParseContext::TypeWords > ().sawAny = true;
      }
#line 1726 "src/parser/parser_bison.cpp"
    break;

  case 33: // $@1: %empty
#line 298 "src/parser/parser_bison.y"
                          { st.ctx.beginStruct(yystack_[1].value.as < Token > ().lexeme, yystack_[1].value.as < Token > ().line, yystack_[1].value.as < Token > ().col); }
#line 1732 "src/parser/parser_bison.cpp"
    break;

  case 34: // struct_spec: "'struct'" "an identifier" "'{'" $@1 fields "'}'"
#line 298 "src/parser/parser_bison.y"
                                                                                            {
          yylhs.value.as < ParseContext::TypeWords > ().t = TypeSpec{};
          yylhs.value.as < ParseContext::TypeWords > ().t.base = BaseType::STRUCT;
          yylhs.value.as < ParseContext::TypeWords > ().t.structId = st.ctx.structs.find(yystack_[4].value.as < Token > ().lexeme);
          st.ctx.finishStruct();
          yylhs.value.as < ParseContext::TypeWords > ().sawAny = true;
      }
#line 1744 "src/parser/parser_bison.cpp"
    break;

  case 35: // @2: %empty
#line 305 "src/parser/parser_bison.y"
                         { yylhs.value.as < int > () = st.ctx.beginStruct("", yystack_[0].value.as < Token > ().line, yystack_[0].value.as < Token > ().col); }
#line 1750 "src/parser/parser_bison.cpp"
    break;

  case 36: // struct_spec: "'struct'" "'{'" @2 fields "'}'"
#line 305 "src/parser/parser_bison.y"
                                                                                         {
          yylhs.value.as < ParseContext::TypeWords > ().t = TypeSpec{};
          yylhs.value.as < ParseContext::TypeWords > ().t.base = BaseType::STRUCT;
          yylhs.value.as < ParseContext::TypeWords > ().t.structId = yystack_[2].value.as < int > ();
          st.ctx.finishStruct();
          yylhs.value.as < ParseContext::TypeWords > ().sawAny = true;
      }
#line 1762 "src/parser/parser_bison.cpp"
    break;

  case 38: // fields: fields decl_pushed field_declarators "';'"
#line 316 "src/parser/parser_bison.y"
                                                { st.declType.resize(yystack_[2].value.as < int > ()); }
#line 1768 "src/parser/parser_bison.cpp"
    break;

  case 41: // field_declarator: named dims
#line 325 "src/parser/parser_bison.y"
                 {
          TypeSpec t = st.ctx.finishType(st.declType.back());
          t.ptrDepth += yystack_[1].value.as < DeclHead > ().stars;
          st.ctx.applyDims(t, yystack_[0].value.as < std::vector<ParseContext::Dim> > (), false);
          st.ctx.addField(t, yystack_[1].value.as < DeclHead > ().name);
      }
#line 1779 "src/parser/parser_bison.cpp"
    break;

  case 42: // enum_spec: "'enum'" "an identifier"
#line 334 "src/parser/parser_bison.y"
                                                         { yylhs.value.as < ParseContext::TypeWords > ().sawAny = true; }
#line 1785 "src/parser/parser_bison.cpp"
    break;

  case 43: // $@3: %empty
#line 335 "src/parser/parser_bison.y"
                        { st.ctx.resetEnum(); }
#line 1791 "src/parser/parser_bison.cpp"
    break;

  case 44: // enum_spec: "'enum'" "an identifier" "'{'" $@3 enumerators "'}'"
#line 335 "src/parser/parser_bison.y"
                                                                   { yylhs.value.as < ParseContext::TypeWords > ().sawAny = true; }
#line 1797 "src/parser/parser_bison.cpp"
    break;

  case 45: // $@4: %empty
#line 336 "src/parser/parser_bison.y"
                  { st.ctx.resetEnum(); }
#line 1803 "src/parser/parser_bison.cpp"
    break;

  case 46: // enum_spec: "'enum'" "'{'" $@4 enumerators "'}'"
#line 336 "src/parser/parser_bison.y"
                                                                   { yylhs.value.as < ParseContext::TypeWords > ().sawAny = true; }
#line 1809 "src/parser/parser_bison.cpp"
    break;

  case 52: // enumerator: "an identifier"
#line 351 "src/parser/parser_bison.y"
            { st.ctx.addEnumerator(yystack_[0].value.as < Token > ().lexeme, false, 0); }
#line 1815 "src/parser/parser_bison.cpp"
    break;

  case 53: // enumerator: "an identifier" "'='" conditional
#line 352 "src/parser/parser_bison.y"
                               {
          st.ctx.addEnumerator(yystack_[2].value.as < Token > ().lexeme, true, st.ctx.constInt(*yystack_[0].value.as < std::unique_ptr<Expr> > (), yystack_[0].location.line, yystack_[0].location.col, "an enumerator value"));
      }
#line 1823 "src/parser/parser_bison.cpp"
    break;

  case 54: // typedef_decl: "'typedef'" decl_pushed typedef_list "';'"
#line 358 "src/parser/parser_bison.y"
                                            { st.declType.resize(yystack_[2].value.as < int > ()); }
#line 1829 "src/parser/parser_bison.cpp"
    break;

  case 57: // typedef_item: named dims
#line 367 "src/parser/parser_bison.y"
                 {
          TypeSpec t = st.ctx.finishType(st.declType.back());
          t.ptrDepth += yystack_[1].value.as < DeclHead > ().stars;
          st.ctx.applyDims(t, yystack_[0].value.as < std::vector<ParseContext::Dim> > (), false);
          st.ctx.typedefs[yystack_[1].value.as < DeclHead > ().name.lexeme] = t;
      }
#line 1840 "src/parser/parser_bison.cpp"
    break;

  case 58: // stars: %empty
#line 377 "src/parser/parser_bison.y"
             { yylhs.value.as < int > () = 0; }
#line 1846 "src/parser/parser_bison.cpp"
    break;

  case 59: // stars: stars1
#line 378 "src/parser/parser_bison.y"
             { yylhs.value.as < int > () = yystack_[0].value.as < int > (); }
#line 1852 "src/parser/parser_bison.cpp"
    break;

  case 60: // stars1: "'*'"
#line 382 "src/parser/parser_bison.y"
                   { yylhs.value.as < int > () = 1; }
#line 1858 "src/parser/parser_bison.cpp"
    break;

  case 61: // stars1: stars1 "'*'"
#line 383 "src/parser/parser_bison.y"
                   { yylhs.value.as < int > () = yystack_[1].value.as < int > () + 1; }
#line 1864 "src/parser/parser_bison.cpp"
    break;

  case 62: // stars1: stars1 "'const'"
#line 384 "src/parser/parser_bison.y"
                   { yylhs.value.as < int > () = yystack_[1].value.as < int > (); }
#line 1870 "src/parser/parser_bison.cpp"
    break;

  case 63: // named: "an identifier"
#line 392 "src/parser/parser_bison.y"
                          { yylhs.value.as < DeclHead > ().name = yystack_[0].value.as < Token > (); }
#line 1876 "src/parser/parser_bison.cpp"
    break;

  case 64: // named: stars1 "an identifier"
#line 393 "src/parser/parser_bison.y"
                          { yylhs.value.as < DeclHead > ().stars = yystack_[1].value.as < int > (); yylhs.value.as < DeclHead > ().name = yystack_[0].value.as < Token > (); }
#line 1882 "src/parser/parser_bison.cpp"
    break;

  case 65: // named: stars1 "a type name"
#line 394 "src/parser/parser_bison.y"
                          { yylhs.value.as < DeclHead > ().stars = yystack_[1].value.as < int > (); yylhs.value.as < DeclHead > ().name = yystack_[0].value.as < Token > (); }
#line 1888 "src/parser/parser_bison.cpp"
    break;

  case 66: // type_name: decl_specs stars
#line 398 "src/parser/parser_bison.y"
                       { yylhs.value.as < TypeSpec > () = st.ctx.finishType(yystack_[1].value.as < ParseContext::TypeWords > ()); yylhs.value.as < TypeSpec > ().ptrDepth += yystack_[0].value.as < int > (); }
#line 1894 "src/parser/parser_bison.cpp"
    break;

  case 67: // dims: %empty
#line 402 "src/parser/parser_bison.y"
               { }
#line 1900 "src/parser/parser_bison.cpp"
    break;

  case 68: // dims: dims dim
#line 403 "src/parser/parser_bison.y"
               { yylhs.value.as < std::vector<ParseContext::Dim> > () = std::move(yystack_[1].value.as < std::vector<ParseContext::Dim> > ()); yylhs.value.as < std::vector<ParseContext::Dim> > ().push_back(std::move(yystack_[0].value.as < ParseContext::Dim > ())); }
#line 1906 "src/parser/parser_bison.cpp"
    break;

  case 69: // dim: "'['" "']'"
#line 407 "src/parser/parser_bison.y"
                                    { yylhs.value.as < ParseContext::Dim > ().line = yystack_[1].value.as < Token > ().line; yylhs.value.as < ParseContext::Dim > ().col = yystack_[1].value.as < Token > ().col; }
#line 1912 "src/parser/parser_bison.cpp"
    break;

  case 70: // dim: "'['" conditional "']'"
#line 408 "src/parser/parser_bison.y"
                                    {
          yylhs.value.as < ParseContext::Dim > ().line = yystack_[2].value.as < Token > ().line; yylhs.value.as < ParseContext::Dim > ().col = yystack_[2].value.as < Token > ().col;
          yylhs.value.as < ParseContext::Dim > ().exprLine = yystack_[1].location.line; yylhs.value.as < ParseContext::Dim > ().exprCol = yystack_[1].location.col;
          yylhs.value.as < ParseContext::Dim > ().size = std::move(yystack_[1].value.as < std::unique_ptr<Expr> > ());
      }
#line 1922 "src/parser/parser_bison.cpp"
    break;

  case 71: // var_declarators: var_declarator
#line 418 "src/parser/parser_bison.y"
                                            { yylhs.value.as < std::vector<std::unique_ptr<VarDecl>> > ().push_back(std::move(yystack_[0].value.as < std::unique_ptr<VarDecl> > ())); }
#line 1928 "src/parser/parser_bison.cpp"
    break;

  case 72: // var_declarators: var_declarators "','" var_declarator
#line 419 "src/parser/parser_bison.y"
                                            { yylhs.value.as < std::vector<std::unique_ptr<VarDecl>> > () = std::move(yystack_[2].value.as < std::vector<std::unique_ptr<VarDecl>> > ()); yylhs.value.as < std::vector<std::unique_ptr<VarDecl>> > ().push_back(std::move(yystack_[0].value.as < std::unique_ptr<VarDecl> > ())); }
#line 1934 "src/parser/parser_bison.cpp"
    break;

  case 73: // @5: %empty
#line 423 "src/parser/parser_bison.y"
                           {
          // Dimensions are applied before the initializer is read, so their
          // diagnostics come first, as in the hand-written parser.
          yylhs.value.as < TypeSpec > () = st.ctx.finishType(st.declType.back());
          yylhs.value.as < TypeSpec > ().ptrDepth += yystack_[1].value.as < DeclHead > ().stars;
          st.ctx.applyDims(yylhs.value.as < TypeSpec > (), yystack_[0].value.as < std::vector<ParseContext::Dim> > (), true);
      }
#line 1946 "src/parser/parser_bison.cpp"
    break;

  case 74: // var_declarator: named dims @5 init_opt
#line 429 "src/parser/parser_bison.y"
                 {
          yylhs.value.as < std::unique_ptr<VarDecl> > () = st.ctx.declarator(yystack_[1].value.as < TypeSpec > (), yystack_[3].value.as < DeclHead > ().name, std::move(yystack_[0].value.as < ParseContext::Initializer > ()));
      }
#line 1954 "src/parser/parser_bison.cpp"
    break;

  case 75: // init_opt: %empty
#line 435 "src/parser/parser_bison.y"
             { yylhs.value.as < ParseContext::Initializer > ().kind = ParseContext::Initializer::NONE; }
#line 1960 "src/parser/parser_bison.cpp"
    break;

  case 76: // init_opt: "'='" init_node
#line 436 "src/parser/parser_bison.y"
                       {
          ParseContext::InitNode& n = yystack_[0].value.as < ParseContext::InitNode > ();
          const TypeSpec& t = yystack_[2].value.as< TypeSpec > ();
          if (n.isList) {
              yylhs.value.as < ParseContext::Initializer > ().kind = ParseContext::Initializer::LIST;
              yylhs.value.as < ParseContext::Initializer > ().list = std::move(n);
          } else if (t.isArray && n.expr->kind == NodeKind::StringLit &&
                     n.expr->line == yystack_[0].location.line && n.expr->col == yystack_[0].location.col) {
              // `char s[] = "abc"`: a bare string literal (not parenthesised).
              yylhs.value.as < ParseContext::Initializer > ().kind = ParseContext::Initializer::STRING;
              yylhs.value.as < ParseContext::Initializer > ().str = Token{Tok::STRING_LIT, static_cast<StringLit&>(*n.expr).value, n.expr->line, n.expr->col};
          } else {
              yylhs.value.as < ParseContext::Initializer > ().kind = ParseContext::Initializer::EXPR;
              yylhs.value.as < ParseContext::Initializer > ().expr = std::move(n.expr);
          }
      }
#line 1981 "src/parser/parser_bison.cpp"
    break;

  case 77: // init_node: assignment
#line 455 "src/parser/parser_bison.y"
                            { yylhs.value.as < ParseContext::InitNode > ().expr = std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()); yylhs.value.as < ParseContext::InitNode > ().line = yystack_[0].location.line; yylhs.value.as < ParseContext::InitNode > ().col = yystack_[0].location.col; }
#line 1987 "src/parser/parser_bison.cpp"
    break;

  case 78: // init_node: "'{'" init_list "'}'"
#line 456 "src/parser/parser_bison.y"
                              {
          yylhs.value.as < ParseContext::InitNode > ().isList = true;
          yylhs.value.as < ParseContext::InitNode > ().kids = std::move(yystack_[1].value.as < std::vector<ParseContext::InitNode> > ());
          yylhs.value.as < ParseContext::InitNode > ().line = yystack_[2].value.as < Token > ().line; yylhs.value.as < ParseContext::InitNode > ().col = yystack_[2].value.as < Token > ().col;
      }
#line 1997 "src/parser/parser_bison.cpp"
    break;

  case 79: // init_list: %empty
#line 464 "src/parser/parser_bison.y"
                       { }
#line 2003 "src/parser/parser_bison.cpp"
    break;

  case 80: // init_list: init_items
#line 465 "src/parser/parser_bison.y"
                       { yylhs.value.as < std::vector<ParseContext::InitNode> > () = std::move(yystack_[0].value.as < std::vector<ParseContext::InitNode> > ()); }
#line 2009 "src/parser/parser_bison.cpp"
    break;

  case 81: // init_list: init_items "','"
#line 466 "src/parser/parser_bison.y"
                       { yylhs.value.as < std::vector<ParseContext::InitNode> > () = std::move(yystack_[1].value.as < std::vector<ParseContext::InitNode> > ()); }
#line 2015 "src/parser/parser_bison.cpp"
    break;

  case 82: // init_items: init_node
#line 470 "src/parser/parser_bison.y"
                                 { yylhs.value.as < std::vector<ParseContext::InitNode> > ().push_back(std::move(yystack_[0].value.as < ParseContext::InitNode > ())); }
#line 2021 "src/parser/parser_bison.cpp"
    break;

  case 83: // init_items: init_items "','" init_node
#line 471 "src/parser/parser_bison.y"
                                 { yylhs.value.as < std::vector<ParseContext::InitNode> > () = std::move(yystack_[2].value.as < std::vector<ParseContext::InitNode> > ()); yylhs.value.as < std::vector<ParseContext::InitNode> > ().push_back(std::move(yystack_[0].value.as < ParseContext::InitNode > ())); }
#line 2027 "src/parser/parser_bison.cpp"
    break;

  case 84: // params: %empty
#line 477 "src/parser/parser_bison.y"
                 { }
#line 2033 "src/parser/parser_bison.cpp"
    break;

  case 85: // params: param_list
#line 478 "src/parser/parser_bison.y"
                 { yylhs.value.as < std::vector<ParamInfo> > () = std::move(yystack_[0].value.as < std::vector<ParamInfo> > ()); }
#line 2039 "src/parser/parser_bison.cpp"
    break;

  case 86: // params: param_list "','" "'.'" "'.'" "'.'"
#line 479 "src/parser/parser_bison.y"
                                   { yylhs.value.as < std::vector<ParamInfo> > () = std::move(yystack_[4].value.as < std::vector<ParamInfo> > ()); }
#line 2045 "src/parser/parser_bison.cpp"
    break;

  case 87: // param_list: param
#line 483 "src/parser/parser_bison.y"
                             { yylhs.value.as < std::vector<ParamInfo> > ().push_back(std::move(yystack_[0].value.as < ParamInfo > ())); }
#line 2051 "src/parser/parser_bison.cpp"
    break;

  case 88: // param_list: param_list "','" param
#line 484 "src/parser/parser_bison.y"
                             { yylhs.value.as < std::vector<ParamInfo> > () = std::move(yystack_[2].value.as < std::vector<ParamInfo> > ()); yylhs.value.as < std::vector<ParamInfo> > ().push_back(std::move(yystack_[0].value.as < ParamInfo > ())); }
#line 2057 "src/parser/parser_bison.cpp"
    break;

  case 89: // param: decl_specs
#line 488 "src/parser/parser_bison.y"
                 {
          yylhs.value.as < ParamInfo > ().type = st.ctx.finishType(yystack_[0].value.as < ParseContext::TypeWords > ());
          yylhs.value.as < ParamInfo > ().at = Pos{st.last.line, st.last.col};
          yylhs.value.as < ParamInfo > ().found = st.last.lexeme;
          yylhs.value.as < ParamInfo > ().plainVoid = yylhs.value.as < ParamInfo > ().type.base == BaseType::VOID;
      }
#line 2068 "src/parser/parser_bison.cpp"
    break;

  case 90: // param: decl_specs stars1
#line 494 "src/parser/parser_bison.y"
                        {
          yylhs.value.as < ParamInfo > ().type = st.ctx.finishType(yystack_[1].value.as < ParseContext::TypeWords > ());
          yylhs.value.as < ParamInfo > ().type.ptrDepth += yystack_[0].value.as < int > ();
          yylhs.value.as < ParamInfo > ().at = Pos{st.last.line, st.last.col};
          yylhs.value.as < ParamInfo > ().found = st.last.lexeme;
      }
#line 2079 "src/parser/parser_bison.cpp"
    break;

  case 91: // param: decl_specs named dims
#line 500 "src/parser/parser_bison.y"
                            {
          yylhs.value.as < ParamInfo > ().type = st.ctx.finishType(yystack_[2].value.as < ParseContext::TypeWords > ());
          yylhs.value.as < ParamInfo > ().type.ptrDepth += yystack_[1].value.as < DeclHead > ().stars;
          yylhs.value.as < ParamInfo > ().named = true;
          yylhs.value.as < ParamInfo > ().name = yystack_[1].value.as < DeclHead > ().name;
          yylhs.value.as < ParamInfo > ().dims = std::move(yystack_[0].value.as < std::vector<ParseContext::Dim> > ());
      }
#line 2091 "src/parser/parser_bison.cpp"
    break;

  case 92: // compound: "'{'" block_items "'}'"
#line 512 "src/parser/parser_bison.y"
                                {
          yylhs.value.as < std::unique_ptr<CompoundStmt> > () = std::make_unique<CompoundStmt>();
          yylhs.value.as < std::unique_ptr<CompoundStmt> > ()->line = yystack_[2].value.as < Token > ().line; yylhs.value.as < std::unique_ptr<CompoundStmt> > ()->col = yystack_[2].value.as < Token > ().col;
          yylhs.value.as < std::unique_ptr<CompoundStmt> > ()->stmts = std::move(yystack_[1].value.as < std::vector<std::unique_ptr<Stmt>> > ());
      }
#line 2101 "src/parser/parser_bison.cpp"
    break;

  case 93: // compound: "'{'" block_items "an identifier" "':'" "'}'"
#line 517 "src/parser/parser_bison.y"
                                            {
          // `label: }`: the label marks an empty statement.
          yylhs.value.as < std::unique_ptr<CompoundStmt> > () = std::make_unique<CompoundStmt>();
          yylhs.value.as < std::unique_ptr<CompoundStmt> > ()->line = yystack_[4].value.as < Token > ().line; yylhs.value.as < std::unique_ptr<CompoundStmt> > ()->col = yystack_[4].value.as < Token > ().col;
          yylhs.value.as < std::unique_ptr<CompoundStmt> > ()->stmts = std::move(yystack_[3].value.as < std::vector<std::unique_ptr<Stmt>> > ());
          std::unique_ptr<Stmt> empty(at(new ExprStmt(nullptr), yystack_[2].location));
          yylhs.value.as < std::unique_ptr<CompoundStmt> > ()->stmts.emplace_back(at(new LabelStmt(yystack_[2].value.as < Token > ().lexeme, std::move(empty)), yystack_[2].location));
      }
#line 2114 "src/parser/parser_bison.cpp"
    break;

  case 94: // compound: "'{'" block_items error "'}'"
#line 525 "src/parser/parser_bison.y"
                                      {
          yyerrok;
          yylhs.value.as < std::unique_ptr<CompoundStmt> > () = std::make_unique<CompoundStmt>();
          yylhs.value.as < std::unique_ptr<CompoundStmt> > ()->line = yystack_[3].value.as < Token > ().line; yylhs.value.as < std::unique_ptr<CompoundStmt> > ()->col = yystack_[3].value.as < Token > ().col;
          yylhs.value.as < std::unique_ptr<CompoundStmt> > ()->stmts = std::move(yystack_[2].value.as < std::vector<std::unique_ptr<Stmt>> > ());
      }
#line 2125 "src/parser/parser_bison.cpp"
    break;

  case 95: // block_items: %empty
#line 534 "src/parser/parser_bison.y"
             { }
#line 2131 "src/parser/parser_bison.cpp"
    break;

  case 96: // block_items: block_items block_item
#line 535 "src/parser/parser_bison.y"
                             {
          yylhs.value.as < std::vector<std::unique_ptr<Stmt>> > () = std::move(yystack_[1].value.as < std::vector<std::unique_ptr<Stmt>> > ());
          for (auto& s : yystack_[0].value.as < std::vector<std::unique_ptr<Stmt>> > ()) yylhs.value.as < std::vector<std::unique_ptr<Stmt>> > ().push_back(std::move(s));
      }
#line 2140 "src/parser/parser_bison.cpp"
    break;

  case 97: // block_item: statement
#line 542 "src/parser/parser_bison.y"
                      { yylhs.value.as < std::vector<std::unique_ptr<Stmt>> > ().push_back(std::move(yystack_[0].value.as < std::unique_ptr<Stmt> > ())); }
#line 2146 "src/parser/parser_bison.cpp"
    break;

  case 98: // block_item: local_decl
#line 543 "src/parser/parser_bison.y"
                      { yylhs.value.as < std::vector<std::unique_ptr<Stmt>> > () = std::move(yystack_[0].value.as < std::vector<std::unique_ptr<Stmt>> > ()); }
#line 2152 "src/parser/parser_bison.cpp"
    break;

  case 99: // block_item: typedef_decl
#line 544 "src/parser/parser_bison.y"
                      { }
#line 2158 "src/parser/parser_bison.cpp"
    break;

  case 100: // block_item: decl_specs "';'"
#line 545 "src/parser/parser_bison.y"
                      { }
#line 2164 "src/parser/parser_bison.cpp"
    break;

  case 101: // block_item: error "';'"
#line 546 "src/parser/parser_bison.y"
                      { yyerrok; }
#line 2170 "src/parser/parser_bison.cpp"
    break;

  case 102: // local_decl: decl_pushed var_declarators "';'"
#line 550 "src/parser/parser_bison.y"
                                       {
          bool isStatic = st.declType[yystack_[2].value.as < int > ()].isStatic;
          st.declType.resize(yystack_[2].value.as < int > ());
          for (auto& d : yystack_[1].value.as < std::vector<std::unique_ptr<VarDecl>> > ()) {
              d->isStatic = isStatic;
              auto ds = std::make_unique<DeclStmt>();
              ds->line = d->line; ds->col = d->col;
              ds->decl = std::move(d);
              yylhs.value.as < std::vector<std::unique_ptr<Stmt>> > ().push_back(std::move(ds));
          }
      }
#line 2186 "src/parser/parser_bison.cpp"
    break;

  case 103: // statement: compound
#line 564 "src/parser/parser_bison.y"
               { yylhs.value.as < std::unique_ptr<Stmt> > () = std::move(yystack_[0].value.as < std::unique_ptr<CompoundStmt> > ()); }
#line 2192 "src/parser/parser_bison.cpp"
    break;

  case 104: // statement: "'if'" "'('" expression "')'" statement
#line 565 "src/parser/parser_bison.y"
                                                                  {
          yylhs.value.as < std::unique_ptr<Stmt> > ().reset(at(new IfStmt(std::move(yystack_[2].value.as < std::unique_ptr<Expr> > ()), std::move(yystack_[0].value.as < std::unique_ptr<Stmt> > ()), nullptr), yystack_[4].location));
      }
#line 2200 "src/parser/parser_bison.cpp"
    break;

  case 105: // statement: "'if'" "'('" expression "')'" statement "'else'" statement
#line 568 "src/parser/parser_bison.y"
                                                           {
          yylhs.value.as < std::unique_ptr<Stmt> > ().reset(at(new IfStmt(std::move(yystack_[4].value.as < std::unique_ptr<Expr> > ()), std::move(yystack_[2].value.as < std::unique_ptr<Stmt> > ()), std::move(yystack_[0].value.as < std::unique_ptr<Stmt> > ())), yystack_[6].location));
      }
#line 2208 "src/parser/parser_bison.cpp"
    break;

  case 106: // statement: "'while'" "'('" expression "')'" statement
#line 571 "src/parser/parser_bison.y"
                                               {
          yylhs.value.as < std::unique_ptr<Stmt> > ().reset(at(new WhileStmt(std::move(yystack_[2].value.as < std::unique_ptr<Expr> > ()), std::move(yystack_[0].value.as < std::unique_ptr<Stmt> > ())), yystack_[4].location));
      }
#line 2216 "src/parser/parser_bison.cpp"
    break;

  case 107: // statement: "'do'" statement "'while'" "'('" expression "')'" "';'"
#line 574 "src/parser/parser_bison.y"
                                                       {
          yylhs.value.as < std::unique_ptr<Stmt> > ().reset(at(new DoStmt(std::move(yystack_[5].value.as < std::unique_ptr<Stmt> > ()), std::move(yystack_[2].value.as < std::unique_ptr<Expr> > ())), yystack_[6].location));
      }
#line 2224 "src/parser/parser_bison.cpp"
    break;

  case 108: // statement: "'for'" "'('" expr_opt "';'" expr_opt "';'" expr_opt "')'" statement
#line 577 "src/parser/parser_bison.y"
                                                                       {
          yylhs.value.as < std::unique_ptr<Stmt> > ().reset(at(new ForStmt(std::move(yystack_[6].value.as < std::unique_ptr<Expr> > ()), std::move(yystack_[4].value.as < std::unique_ptr<Expr> > ()), std::move(yystack_[2].value.as < std::unique_ptr<Expr> > ()), std::move(yystack_[0].value.as < std::unique_ptr<Stmt> > ())), yystack_[8].location));
      }
#line 2232 "src/parser/parser_bison.cpp"
    break;

  case 109: // statement: "'for'" "'('" local_decl expr_opt "';'" expr_opt "')'" statement
#line 580 "src/parser/parser_bison.y"
                                                                    {
          // `for (int i = 0; ...)` is desugared into { int i = 0; for (; ...) body }.
          auto block = std::make_unique<CompoundStmt>();
          block->line = yystack_[7].location.line; block->col = yystack_[7].location.col;
          for (auto& d : yystack_[5].value.as < std::vector<std::unique_ptr<Stmt>> > ()) block->stmts.push_back(std::move(d));
          block->stmts.emplace_back(at(new ForStmt(nullptr, std::move(yystack_[4].value.as < std::unique_ptr<Expr> > ()), std::move(yystack_[2].value.as < std::unique_ptr<Expr> > ()), std::move(yystack_[0].value.as < std::unique_ptr<Stmt> > ())), yystack_[7].location));
          yylhs.value.as < std::unique_ptr<Stmt> > () = std::move(block);
      }
#line 2245 "src/parser/parser_bison.cpp"
    break;

  case 110: // statement: "'return'" expr_opt "';'"
#line 588 "src/parser/parser_bison.y"
                             { yylhs.value.as < std::unique_ptr<Stmt> > ().reset(at(new ReturnStmt(std::move(yystack_[1].value.as < std::unique_ptr<Expr> > ())), yystack_[2].location)); }
#line 2251 "src/parser/parser_bison.cpp"
    break;

  case 111: // statement: "'break'" "';'"
#line 589 "src/parser/parser_bison.y"
                             { yylhs.value.as < std::unique_ptr<Stmt> > ().reset(at(new BreakStmt(), yystack_[1].location)); }
#line 2257 "src/parser/parser_bison.cpp"
    break;

  case 112: // statement: "'continue'" "';'"
#line 590 "src/parser/parser_bison.y"
                             { yylhs.value.as < std::unique_ptr<Stmt> > ().reset(at(new ContinueStmt(), yystack_[1].location)); }
#line 2263 "src/parser/parser_bison.cpp"
    break;

  case 113: // statement: "'goto'" "an identifier" "';'"
#line 591 "src/parser/parser_bison.y"
                             { yylhs.value.as < std::unique_ptr<Stmt> > ().reset(at(new GotoStmt(yystack_[1].value.as < Token > ().lexeme), yystack_[2].location)); }
#line 2269 "src/parser/parser_bison.cpp"
    break;

  case 114: // statement: "an identifier" "':'" statement
#line 592 "src/parser/parser_bison.y"
                             { yylhs.value.as < std::unique_ptr<Stmt> > ().reset(at(new LabelStmt(yystack_[2].value.as < Token > ().lexeme, std::move(yystack_[0].value.as < std::unique_ptr<Stmt> > ())), yystack_[2].location)); }
#line 2275 "src/parser/parser_bison.cpp"
    break;

  case 115: // statement: switch_head switch_items "'}'"
#line 593 "src/parser/parser_bison.y"
                                      {
          yylhs.value.as < std::unique_ptr<Stmt> > ().reset(st.switches.back());
          st.switches.pop_back();
      }
#line 2284 "src/parser/parser_bison.cpp"
    break;

  case 116: // statement: switch_head switch_items error "'}'"
#line 597 "src/parser/parser_bison.y"
                                            {
          // Recovery inside a switch body stops at its '}', as in a block.
          yyerrok;
          yylhs.value.as < std::unique_ptr<Stmt> > ().reset(st.switches.back());
          st.switches.pop_back();
      }
#line 2295 "src/parser/parser_bison.cpp"
    break;

  case 117: // statement: expression "';'"
#line 603 "src/parser/parser_bison.y"
                      { yylhs.value.as < std::unique_ptr<Stmt> > ().reset(at(new ExprStmt(std::move(yystack_[1].value.as < std::unique_ptr<Expr> > ())), yystack_[1].location)); }
#line 2301 "src/parser/parser_bison.cpp"
    break;

  case 118: // statement: "';'"
#line 604 "src/parser/parser_bison.y"
                      { yylhs.value.as < std::unique_ptr<Stmt> > ().reset(at(new ExprStmt(nullptr), yystack_[0].location)); }
#line 2307 "src/parser/parser_bison.cpp"
    break;

  case 119: // switch_head: "'switch'" "'('" expression "')'" "'{'"
#line 608 "src/parser/parser_bison.y"
                                             {
          auto* sw = new SwitchStmt();
          sw->line = yystack_[4].location.line; sw->col = yystack_[4].location.col;
          sw->cond = std::move(yystack_[2].value.as < std::unique_ptr<Expr> > ());
          st.switches.push_back(sw);
      }
#line 2318 "src/parser/parser_bison.cpp"
    break;

  case 122: // switch_item: "'case'" conditional "':'"
#line 622 "src/parser/parser_bison.y"
                             {
          SwitchCase sc;
          sc.line = yystack_[2].value.as < Token > ().line; sc.col = yystack_[2].value.as < Token > ().col;
          sc.value = st.ctx.constInt(*yystack_[1].value.as < std::unique_ptr<Expr> > (), yystack_[1].location.line, yystack_[1].location.col, "a case label");
          st.switches.back()->cases.push_back(std::move(sc));
      }
#line 2329 "src/parser/parser_bison.cpp"
    break;

  case 123: // switch_item: "'default'" "':'"
#line 628 "src/parser/parser_bison.y"
                    {
          SwitchCase sc;
          sc.line = yystack_[1].value.as < Token > ().line; sc.col = yystack_[1].value.as < Token > ().col;
          sc.isDefault = true;
          st.switches.back()->cases.push_back(std::move(sc));
      }
#line 2340 "src/parser/parser_bison.cpp"
    break;

  case 124: // switch_item: block_item
#line 634 "src/parser/parser_bison.y"
                 {
          SwitchStmt* sw = st.switches.back();
          if (sw->cases.empty()) {
              if (!yystack_[0].value.as < std::vector<std::unique_ptr<Stmt>> > ().empty())
                  st.sink.add(yystack_[0].location.line, yystack_[0].location.col, "statement before the first 'case' or 'default' in a switch");
          } else {
              for (auto& s : yystack_[0].value.as < std::vector<std::unique_ptr<Stmt>> > ()) sw->cases.back().stmts.push_back(std::move(s));
          }
      }
#line 2354 "src/parser/parser_bison.cpp"
    break;

  case 125: // expr_opt: %empty
#line 646 "src/parser/parser_bison.y"
                 { }
#line 2360 "src/parser/parser_bison.cpp"
    break;

  case 126: // expr_opt: expression
#line 647 "src/parser/parser_bison.y"
                 { yylhs.value.as < std::unique_ptr<Expr> > () = std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()); }
#line 2366 "src/parser/parser_bison.cpp"
    break;

  case 127: // expression: assignment
#line 653 "src/parser/parser_bison.y"
                 { yylhs.value.as < std::unique_ptr<Expr> > () = std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()); }
#line 2372 "src/parser/parser_bison.cpp"
    break;

  case 128: // expression: expression "','" assignment
#line 654 "src/parser/parser_bison.y"
                                  { yylhs.value.as < std::unique_ptr<Expr> > ().reset(at(new CommaExpr(std::move(yystack_[2].value.as < std::unique_ptr<Expr> > ()), std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ())), yystack_[1].location)); }
#line 2378 "src/parser/parser_bison.cpp"
    break;

  case 129: // assignment: conditional
#line 658 "src/parser/parser_bison.y"
                  { yylhs.value.as < std::unique_ptr<Expr> > () = std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()); }
#line 2384 "src/parser/parser_bison.cpp"
    break;

  case 130: // assignment: conditional assign_op assignment
#line 659 "src/parser/parser_bison.y"
                                       {
          NodeKind k = yystack_[2].value.as < std::unique_ptr<Expr> > ()->kind;
          bool deref = k == NodeKind::UnaryExpr && static_cast<UnaryExpr&>(*yystack_[2].value.as < std::unique_ptr<Expr> > ()).op == UnOp::DEREF;
          if (k != NodeKind::IdentExpr && k != NodeKind::IndexExpr && k != NodeKind::MemberExpr && !deref)
              st.sink.add(yystack_[2].location.line, yystack_[2].location.col,
                          "left-hand side of assignment must be a variable, array element, struct field or dereferenced pointer");
          yylhs.value.as < std::unique_ptr<Expr> > ().reset(at(new AssignExpr(yystack_[1].value.as < AssignOp > (), std::move(yystack_[2].value.as < std::unique_ptr<Expr> > ()), std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ())), yystack_[2].location));
      }
#line 2397 "src/parser/parser_bison.cpp"
    break;

  case 131: // assign_op: "'='"
#line 670 "src/parser/parser_bison.y"
                     { yylhs.value.as < AssignOp > () = AssignOp::ASSIGN; }
#line 2403 "src/parser/parser_bison.cpp"
    break;

  case 132: // assign_op: "'+='"
#line 671 "src/parser/parser_bison.y"
                     { yylhs.value.as < AssignOp > () = AssignOp::PLUS_ASSIGN; }
#line 2409 "src/parser/parser_bison.cpp"
    break;

  case 133: // assign_op: "'-='"
#line 672 "src/parser/parser_bison.y"
                     { yylhs.value.as < AssignOp > () = AssignOp::MINUS_ASSIGN; }
#line 2415 "src/parser/parser_bison.cpp"
    break;

  case 134: // assign_op: "'*='"
#line 673 "src/parser/parser_bison.y"
                     { yylhs.value.as < AssignOp > () = AssignOp::STAR_ASSIGN; }
#line 2421 "src/parser/parser_bison.cpp"
    break;

  case 135: // assign_op: "'/='"
#line 674 "src/parser/parser_bison.y"
                     { yylhs.value.as < AssignOp > () = AssignOp::SLASH_ASSIGN; }
#line 2427 "src/parser/parser_bison.cpp"
    break;

  case 136: // assign_op: "'%='"
#line 675 "src/parser/parser_bison.y"
                     { yylhs.value.as < AssignOp > () = AssignOp::PERCENT_ASSIGN; }
#line 2433 "src/parser/parser_bison.cpp"
    break;

  case 137: // assign_op: "'&='"
#line 676 "src/parser/parser_bison.y"
                     { yylhs.value.as < AssignOp > () = AssignOp::AND_ASSIGN; }
#line 2439 "src/parser/parser_bison.cpp"
    break;

  case 138: // assign_op: "'|='"
#line 677 "src/parser/parser_bison.y"
                     { yylhs.value.as < AssignOp > () = AssignOp::OR_ASSIGN; }
#line 2445 "src/parser/parser_bison.cpp"
    break;

  case 139: // assign_op: "'^='"
#line 678 "src/parser/parser_bison.y"
                     { yylhs.value.as < AssignOp > () = AssignOp::XOR_ASSIGN; }
#line 2451 "src/parser/parser_bison.cpp"
    break;

  case 140: // assign_op: "'<<='"
#line 679 "src/parser/parser_bison.y"
                     { yylhs.value.as < AssignOp > () = AssignOp::SHL_ASSIGN; }
#line 2457 "src/parser/parser_bison.cpp"
    break;

  case 141: // assign_op: "'>>='"
#line 680 "src/parser/parser_bison.y"
                     { yylhs.value.as < AssignOp > () = AssignOp::SHR_ASSIGN; }
#line 2463 "src/parser/parser_bison.cpp"
    break;

  case 142: // conditional: logical_or
#line 684 "src/parser/parser_bison.y"
                 { yylhs.value.as < std::unique_ptr<Expr> > () = std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()); }
#line 2469 "src/parser/parser_bison.cpp"
    break;

  case 143: // conditional: logical_or "'?'" expression "':'" conditional
#line 685 "src/parser/parser_bison.y"
                                                       {
          yylhs.value.as < std::unique_ptr<Expr> > ().reset(at(new CondExpr(std::move(yystack_[4].value.as < std::unique_ptr<Expr> > ()), std::move(yystack_[2].value.as < std::unique_ptr<Expr> > ()), std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ())), yystack_[3].location));
      }
#line 2477 "src/parser/parser_bison.cpp"
    break;

  case 144: // logical_or: logical_and
#line 691 "src/parser/parser_bison.y"
                  { yylhs.value.as < std::unique_ptr<Expr> > () = std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()); }
#line 2483 "src/parser/parser_bison.cpp"
    break;

  case 145: // logical_or: logical_or "'||'" logical_and
#line 692 "src/parser/parser_bison.y"
                                  { yylhs.value.as < std::unique_ptr<Expr> > () = binary(BinOp::LOGOR, std::move(yystack_[2].value.as < std::unique_ptr<Expr> > ()), std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()), yystack_[1].location); }
#line 2489 "src/parser/parser_bison.cpp"
    break;

  case 146: // logical_and: bit_or
#line 696 "src/parser/parser_bison.y"
             { yylhs.value.as < std::unique_ptr<Expr> > () = std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()); }
#line 2495 "src/parser/parser_bison.cpp"
    break;

  case 147: // logical_and: logical_and "'&&'" bit_or
#line 697 "src/parser/parser_bison.y"
                                { yylhs.value.as < std::unique_ptr<Expr> > () = binary(BinOp::LOGAND, std::move(yystack_[2].value.as < std::unique_ptr<Expr> > ()), std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()), yystack_[1].location); }
#line 2501 "src/parser/parser_bison.cpp"
    break;

  case 148: // bit_or: bit_xor
#line 701 "src/parser/parser_bison.y"
              { yylhs.value.as < std::unique_ptr<Expr> > () = std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()); }
#line 2507 "src/parser/parser_bison.cpp"
    break;

  case 149: // bit_or: bit_or "'|'" bit_xor
#line 702 "src/parser/parser_bison.y"
                           { yylhs.value.as < std::unique_ptr<Expr> > () = binary(BinOp::BITOR, std::move(yystack_[2].value.as < std::unique_ptr<Expr> > ()), std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()), yystack_[1].location); }
#line 2513 "src/parser/parser_bison.cpp"
    break;

  case 150: // bit_xor: bit_and
#line 706 "src/parser/parser_bison.y"
              { yylhs.value.as < std::unique_ptr<Expr> > () = std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()); }
#line 2519 "src/parser/parser_bison.cpp"
    break;

  case 151: // bit_xor: bit_xor "'^'" bit_and
#line 707 "src/parser/parser_bison.y"
                             { yylhs.value.as < std::unique_ptr<Expr> > () = binary(BinOp::BITXOR, std::move(yystack_[2].value.as < std::unique_ptr<Expr> > ()), std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()), yystack_[1].location); }
#line 2525 "src/parser/parser_bison.cpp"
    break;

  case 152: // bit_and: equality
#line 711 "src/parser/parser_bison.y"
               { yylhs.value.as < std::unique_ptr<Expr> > () = std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()); }
#line 2531 "src/parser/parser_bison.cpp"
    break;

  case 153: // bit_and: bit_and "'&'" equality
#line 712 "src/parser/parser_bison.y"
                              { yylhs.value.as < std::unique_ptr<Expr> > () = binary(BinOp::BITAND, std::move(yystack_[2].value.as < std::unique_ptr<Expr> > ()), std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()), yystack_[1].location); }
#line 2537 "src/parser/parser_bison.cpp"
    break;

  case 154: // equality: relational
#line 716 "src/parser/parser_bison.y"
                 { yylhs.value.as < std::unique_ptr<Expr> > () = std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()); }
#line 2543 "src/parser/parser_bison.cpp"
    break;

  case 155: // equality: equality "'=='" relational
#line 717 "src/parser/parser_bison.y"
                             { yylhs.value.as < std::unique_ptr<Expr> > () = binary(BinOp::EQ, std::move(yystack_[2].value.as < std::unique_ptr<Expr> > ()), std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()), yystack_[1].location); }
#line 2549 "src/parser/parser_bison.cpp"
    break;

  case 156: // equality: equality "'!='" relational
#line 718 "src/parser/parser_bison.y"
                             { yylhs.value.as < std::unique_ptr<Expr> > () = binary(BinOp::NE, std::move(yystack_[2].value.as < std::unique_ptr<Expr> > ()), std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()), yystack_[1].location); }
#line 2555 "src/parser/parser_bison.cpp"
    break;

  case 157: // relational: shift
#line 722 "src/parser/parser_bison.y"
            { yylhs.value.as < std::unique_ptr<Expr> > () = std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()); }
#line 2561 "src/parser/parser_bison.cpp"
    break;

  case 158: // relational: relational "'<'" shift
#line 723 "src/parser/parser_bison.y"
                          { yylhs.value.as < std::unique_ptr<Expr> > () = binary(BinOp::LT, std::move(yystack_[2].value.as < std::unique_ptr<Expr> > ()), std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()), yystack_[1].location); }
#line 2567 "src/parser/parser_bison.cpp"
    break;

  case 159: // relational: relational "'<='" shift
#line 724 "src/parser/parser_bison.y"
                          { yylhs.value.as < std::unique_ptr<Expr> > () = binary(BinOp::LE, std::move(yystack_[2].value.as < std::unique_ptr<Expr> > ()), std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()), yystack_[1].location); }
#line 2573 "src/parser/parser_bison.cpp"
    break;

  case 160: // relational: relational "'>'" shift
#line 725 "src/parser/parser_bison.y"
                          { yylhs.value.as < std::unique_ptr<Expr> > () = binary(BinOp::GT, std::move(yystack_[2].value.as < std::unique_ptr<Expr> > ()), std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()), yystack_[1].location); }
#line 2579 "src/parser/parser_bison.cpp"
    break;

  case 161: // relational: relational "'>='" shift
#line 726 "src/parser/parser_bison.y"
                          { yylhs.value.as < std::unique_ptr<Expr> > () = binary(BinOp::GE, std::move(yystack_[2].value.as < std::unique_ptr<Expr> > ()), std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()), yystack_[1].location); }
#line 2585 "src/parser/parser_bison.cpp"
    break;

  case 162: // shift: additive
#line 730 "src/parser/parser_bison.y"
               { yylhs.value.as < std::unique_ptr<Expr> > () = std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()); }
#line 2591 "src/parser/parser_bison.cpp"
    break;

  case 163: // shift: shift "'<<'" additive
#line 731 "src/parser/parser_bison.y"
                         { yylhs.value.as < std::unique_ptr<Expr> > () = binary(BinOp::SHL, std::move(yystack_[2].value.as < std::unique_ptr<Expr> > ()), std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()), yystack_[1].location); }
#line 2597 "src/parser/parser_bison.cpp"
    break;

  case 164: // shift: shift "'>>'" additive
#line 732 "src/parser/parser_bison.y"
                         { yylhs.value.as < std::unique_ptr<Expr> > () = binary(BinOp::SHR, std::move(yystack_[2].value.as < std::unique_ptr<Expr> > ()), std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()), yystack_[1].location); }
#line 2603 "src/parser/parser_bison.cpp"
    break;

  case 165: // additive: multiplicative
#line 736 "src/parser/parser_bison.y"
                     { yylhs.value.as < std::unique_ptr<Expr> > () = std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()); }
#line 2609 "src/parser/parser_bison.cpp"
    break;

  case 166: // additive: additive "'+'" multiplicative
#line 737 "src/parser/parser_bison.y"
                                    { yylhs.value.as < std::unique_ptr<Expr> > () = binary(BinOp::ADD, std::move(yystack_[2].value.as < std::unique_ptr<Expr> > ()), std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()), yystack_[1].location); }
#line 2615 "src/parser/parser_bison.cpp"
    break;

  case 167: // additive: additive "'-'" multiplicative
#line 738 "src/parser/parser_bison.y"
                                    { yylhs.value.as < std::unique_ptr<Expr> > () = binary(BinOp::SUB, std::move(yystack_[2].value.as < std::unique_ptr<Expr> > ()), std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()), yystack_[1].location); }
#line 2621 "src/parser/parser_bison.cpp"
    break;

  case 168: // multiplicative: cast
#line 742 "src/parser/parser_bison.y"
           { yylhs.value.as < std::unique_ptr<Expr> > () = std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()); }
#line 2627 "src/parser/parser_bison.cpp"
    break;

  case 169: // multiplicative: multiplicative "'*'" cast
#line 743 "src/parser/parser_bison.y"
                                  { yylhs.value.as < std::unique_ptr<Expr> > () = binary(BinOp::MUL, std::move(yystack_[2].value.as < std::unique_ptr<Expr> > ()), std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()), yystack_[1].location); }
#line 2633 "src/parser/parser_bison.cpp"
    break;

  case 170: // multiplicative: multiplicative "'/'" cast
#line 744 "src/parser/parser_bison.y"
                                  { yylhs.value.as < std::unique_ptr<Expr> > () = binary(BinOp::DIV, std::move(yystack_[2].value.as < std::unique_ptr<Expr> > ()), std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()), yystack_[1].location); }
#line 2639 "src/parser/parser_bison.cpp"
    break;

  case 171: // multiplicative: multiplicative "'%'" cast
#line 745 "src/parser/parser_bison.y"
                                  { yylhs.value.as < std::unique_ptr<Expr> > () = binary(BinOp::MOD, std::move(yystack_[2].value.as < std::unique_ptr<Expr> > ()), std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()), yystack_[1].location); }
#line 2645 "src/parser/parser_bison.cpp"
    break;

  case 172: // cast: unary
#line 751 "src/parser/parser_bison.y"
            { yylhs.value.as < std::unique_ptr<Expr> > () = std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()); }
#line 2651 "src/parser/parser_bison.cpp"
    break;

  case 173: // cast: "'('" type_name "')'" cast
#line 752 "src/parser/parser_bison.y"
                                   { yylhs.value.as < std::unique_ptr<Expr> > ().reset(at(new CastExpr(yystack_[2].value.as < TypeSpec > (), std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ())), yystack_[3].location)); }
#line 2657 "src/parser/parser_bison.cpp"
    break;

  case 174: // unary: postfix
#line 756 "src/parser/parser_bison.y"
              { yylhs.value.as < std::unique_ptr<Expr> > () = std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()); }
#line 2663 "src/parser/parser_bison.cpp"
    break;

  case 175: // unary: "'sizeof'" "'('" type_name dims "')'"
#line 757 "src/parser/parser_bison.y"
                                          {
          auto* sz = at(new SizeofExpr(), yystack_[4].location);
          sz->ofType = true;
          sz->type = yystack_[2].value.as < TypeSpec > ();
          st.ctx.applyDims(sz->type, yystack_[1].value.as < std::vector<ParseContext::Dim> > (), false);
          sz->value = st.ctx.sizeofType(sz->type);
          yylhs.value.as < std::unique_ptr<Expr> > ().reset(sz);
      }
#line 2676 "src/parser/parser_bison.cpp"
    break;

  case 176: // unary: "'sizeof'" unary
#line 765 "src/parser/parser_bison.y"
                   {
          auto* sz = at(new SizeofExpr(), yystack_[1].location);
          sz->operand = std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ());
          yylhs.value.as < std::unique_ptr<Expr> > ().reset(sz);
      }
#line 2686 "src/parser/parser_bison.cpp"
    break;

  case 177: // unary: "'&'" cast
#line 770 "src/parser/parser_bison.y"
                  { yylhs.value.as < std::unique_ptr<Expr> > () = unary(UnOp::ADDR,    std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()), yystack_[1].location); }
#line 2692 "src/parser/parser_bison.cpp"
    break;

  case 178: // unary: "'*'" cast
#line 771 "src/parser/parser_bison.y"
                  { yylhs.value.as < std::unique_ptr<Expr> > () = unary(UnOp::DEREF,   std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()), yystack_[1].location); }
#line 2698 "src/parser/parser_bison.cpp"
    break;

  case 179: // unary: "'!'" cast
#line 772 "src/parser/parser_bison.y"
                  { yylhs.value.as < std::unique_ptr<Expr> > () = unary(UnOp::NOT,     std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()), yystack_[1].location); }
#line 2704 "src/parser/parser_bison.cpp"
    break;

  case 180: // unary: "'~'" cast
#line 773 "src/parser/parser_bison.y"
                  { yylhs.value.as < std::unique_ptr<Expr> > () = unary(UnOp::BITNOT,  std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()), yystack_[1].location); }
#line 2710 "src/parser/parser_bison.cpp"
    break;

  case 181: // unary: "'-'" cast
#line 774 "src/parser/parser_bison.y"
                  { yylhs.value.as < std::unique_ptr<Expr> > () = unary(UnOp::NEG,     std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()), yystack_[1].location); }
#line 2716 "src/parser/parser_bison.cpp"
    break;

  case 182: // unary: "'+'" cast
#line 775 "src/parser/parser_bison.y"
                  { yylhs.value.as < std::unique_ptr<Expr> > () = unary(UnOp::POS,     std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()), yystack_[1].location); }
#line 2722 "src/parser/parser_bison.cpp"
    break;

  case 183: // unary: "'++'" cast
#line 776 "src/parser/parser_bison.y"
                  { yylhs.value.as < std::unique_ptr<Expr> > () = unary(UnOp::PRE_INC, std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()), yystack_[1].location); }
#line 2728 "src/parser/parser_bison.cpp"
    break;

  case 184: // unary: "'--'" cast
#line 777 "src/parser/parser_bison.y"
                  { yylhs.value.as < std::unique_ptr<Expr> > () = unary(UnOp::PRE_DEC, std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()), yystack_[1].location); }
#line 2734 "src/parser/parser_bison.cpp"
    break;

  case 185: // postfix: primary
#line 782 "src/parser/parser_bison.y"
              { yylhs.value.as < std::unique_ptr<Expr> > () = std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ()); }
#line 2740 "src/parser/parser_bison.cpp"
    break;

  case 186: // postfix: postfix "'['" expression "']'"
#line 783 "src/parser/parser_bison.y"
                                           {
          Expr& base = *yystack_[3].value.as < std::unique_ptr<Expr> > ();
          yylhs.value.as < std::unique_ptr<Expr> > ().reset(at(new IndexExpr(std::move(yystack_[3].value.as < std::unique_ptr<Expr> > ()), std::move(yystack_[1].value.as < std::unique_ptr<Expr> > ())), base));
      }
#line 2749 "src/parser/parser_bison.cpp"
    break;

  case 187: // postfix: postfix "'('" "')'"
#line 787 "src/parser/parser_bison.y"
                            {
          std::string callee;
          if (yystack_[2].value.as < std::unique_ptr<Expr> > ()->kind == NodeKind::IdentExpr) callee = static_cast<IdentExpr&>(*yystack_[2].value.as < std::unique_ptr<Expr> > ()).name;
          else st.sink.add(yystack_[2].value.as < std::unique_ptr<Expr> > ()->line, yystack_[2].value.as < std::unique_ptr<Expr> > ()->col, "'(' follows an expression that is not a function name");
          yylhs.value.as < std::unique_ptr<Expr> > ().reset(at(new CallExpr(callee), *yystack_[2].value.as < std::unique_ptr<Expr> > ()));
      }
#line 2760 "src/parser/parser_bison.cpp"
    break;

  case 188: // postfix: postfix "'('" args "')'"
#line 793 "src/parser/parser_bison.y"
                                 {
          std::string callee;
          if (yystack_[3].value.as < std::unique_ptr<Expr> > ()->kind == NodeKind::IdentExpr) callee = static_cast<IdentExpr&>(*yystack_[3].value.as < std::unique_ptr<Expr> > ()).name;
          else st.sink.add(yystack_[3].value.as < std::unique_ptr<Expr> > ()->line, yystack_[3].value.as < std::unique_ptr<Expr> > ()->col, "'(' follows an expression that is not a function name");
          auto* c = at(new CallExpr(callee), *yystack_[3].value.as < std::unique_ptr<Expr> > ());
          c->args = std::move(yystack_[1].value.as < std::vector<std::unique_ptr<Expr>> > ());
          yylhs.value.as < std::unique_ptr<Expr> > ().reset(c);
      }
#line 2773 "src/parser/parser_bison.cpp"
    break;

  case 189: // postfix: postfix "'.'" "an identifier"
#line 801 "src/parser/parser_bison.y"
                        {
          Expr& base = *yystack_[2].value.as < std::unique_ptr<Expr> > ();
          yylhs.value.as < std::unique_ptr<Expr> > ().reset(at(new MemberExpr(std::move(yystack_[2].value.as < std::unique_ptr<Expr> > ()), yystack_[0].value.as < Token > ().lexeme, false), base));
      }
#line 2782 "src/parser/parser_bison.cpp"
    break;

  case 190: // postfix: postfix "'->'" "an identifier"
#line 805 "src/parser/parser_bison.y"
                          {
          Expr& base = *yystack_[2].value.as < std::unique_ptr<Expr> > ();
          yylhs.value.as < std::unique_ptr<Expr> > ().reset(at(new MemberExpr(std::move(yystack_[2].value.as < std::unique_ptr<Expr> > ()), yystack_[0].value.as < Token > ().lexeme, true), base));
      }
#line 2791 "src/parser/parser_bison.cpp"
    break;

  case 191: // postfix: postfix "'++'"
#line 809 "src/parser/parser_bison.y"
                  {
          Expr& base = *yystack_[1].value.as < std::unique_ptr<Expr> > ();
          yylhs.value.as < std::unique_ptr<Expr> > ().reset(at(new UnaryExpr(UnOp::POST_INC, std::move(yystack_[1].value.as < std::unique_ptr<Expr> > ())), base));
      }
#line 2800 "src/parser/parser_bison.cpp"
    break;

  case 192: // postfix: postfix "'--'"
#line 813 "src/parser/parser_bison.y"
                  {
          Expr& base = *yystack_[1].value.as < std::unique_ptr<Expr> > ();
          yylhs.value.as < std::unique_ptr<Expr> > ().reset(at(new UnaryExpr(UnOp::POST_DEC, std::move(yystack_[1].value.as < std::unique_ptr<Expr> > ())), base));
      }
#line 2809 "src/parser/parser_bison.cpp"
    break;

  case 193: // args: assignment
#line 820 "src/parser/parser_bison.y"
                            { yylhs.value.as < std::vector<std::unique_ptr<Expr>> > ().push_back(std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ())); }
#line 2815 "src/parser/parser_bison.cpp"
    break;

  case 194: // args: args "','" assignment
#line 821 "src/parser/parser_bison.y"
                            { yylhs.value.as < std::vector<std::unique_ptr<Expr>> > () = std::move(yystack_[2].value.as < std::vector<std::unique_ptr<Expr>> > ()); yylhs.value.as < std::vector<std::unique_ptr<Expr>> > ().push_back(std::move(yystack_[0].value.as < std::unique_ptr<Expr> > ())); }
#line 2821 "src/parser/parser_bison.cpp"
    break;

  case 195: // primary: "an identifier"
#line 825 "src/parser/parser_bison.y"
            {
          auto ec = st.ctx.enumConsts.find(yystack_[0].value.as < Token > ().lexeme);
          if (ec != st.ctx.enumConsts.end()) yylhs.value.as < std::unique_ptr<Expr> > ().reset(at(new IntLit(ec->second), yystack_[0].location));
          else                               yylhs.value.as < std::unique_ptr<Expr> > ().reset(at(new IdentExpr(yystack_[0].value.as < Token > ().lexeme), yystack_[0].location));
      }
#line 2831 "src/parser/parser_bison.cpp"
    break;

  case 196: // primary: "an integer"
#line 830 "src/parser/parser_bison.y"
              {
          // stoull, not stoll: an unsigned literal may exceed LLONG_MAX.
          auto* e = at(new IntLit(static_cast<long long>(std::stoull(yystack_[0].value.as < Token > ().lexeme))), yystack_[0].location);
          for (char ch : yystack_[0].value.as < Token > ().lexeme) {
              if (ch == 'u' || ch == 'U') e->unsignedSuffix = true;
              if (ch == 'l' || ch == 'L') e->longSuffix = true;
          }
          yylhs.value.as < std::unique_ptr<Expr> > ().reset(e);
      }
#line 2845 "src/parser/parser_bison.cpp"
    break;

  case 197: // primary: "a number"
#line 839 "src/parser/parser_bison.y"
                 { yylhs.value.as < std::unique_ptr<Expr> > ().reset(at(new FloatLit(std::stod(yystack_[0].value.as < Token > ().lexeme)), yystack_[0].location)); }
#line 2851 "src/parser/parser_bison.cpp"
    break;

  case 198: // primary: "a character"
#line 840 "src/parser/parser_bison.y"
                 { yylhs.value.as < std::unique_ptr<Expr> > ().reset(at(new CharLit(decodeChar(yystack_[0].value.as < Token > ().lexeme)), yystack_[0].location)); }
#line 2857 "src/parser/parser_bison.cpp"
    break;

  case 199: // primary: "a string"
#line 841 "src/parser/parser_bison.y"
                 { yylhs.value.as < std::unique_ptr<Expr> > ().reset(at(new StringLit(yystack_[0].value.as < Token > ().lexeme), yystack_[0].location)); }
#line 2863 "src/parser/parser_bison.cpp"
    break;

  case 200: // primary: "'('" expression "')'"
#line 842 "src/parser/parser_bison.y"
                               { yylhs.value.as < std::unique_ptr<Expr> > () = std::move(yystack_[1].value.as < std::unique_ptr<Expr> > ()); }
#line 2869 "src/parser/parser_bison.cpp"
    break;


#line 2873 "src/parser/parser_bison.cpp"

            default:
              break;
            }
        }
#if YY_EXCEPTIONS
      catch (const syntax_error& yyexc)
        {
          YYCDEBUG << "Caught exception: " << yyexc.what() << '\n';
          error (yyexc);
          YYERROR;
        }
#endif // YY_EXCEPTIONS
      YY_SYMBOL_PRINT ("-> $$ =", yylhs);
      yypop_ (yylen);
      yylen = 0;

      // Shift the result of the reduction.
      yypush_ (YY_NULLPTR, YY_MOVE (yylhs));
    }
    goto yynewstate;


  /*--------------------------------------.
  | yyerrlab -- here on detecting error.  |
  `--------------------------------------*/
  yyerrlab:
    // If not already recovering from an error, report this error.
    if (!yyerrstatus_)
      {
        ++yynerrs_;
        context yyctx (*this, yyla);
        report_syntax_error (yyctx);
      }


    yyerror_range[1].location = yyla.location;
    if (yyerrstatus_ == 3)
      {
        /* If just tried and failed to reuse lookahead token after an
           error, discard it.  */

        // Return failure if at end of input.
        if (yyla.kind () == symbol_kind::S_YYEOF)
          YYABORT;
        else if (!yyla.empty ())
          {
            yy_destroy_ ("Error: discarding", yyla);
            yyla.clear ();
          }
      }

    // Else will try to reuse lookahead token after shifting the error token.
    goto yyerrlab1;


  /*---------------------------------------------------.
  | yyerrorlab -- error raised explicitly by YYERROR.  |
  `---------------------------------------------------*/
  yyerrorlab:
    /* Pacify compilers when the user code never invokes YYERROR and
       the label yyerrorlab therefore never appears in user code.  */
    if (false)
      YYERROR;

    /* Do not reclaim the symbols of the rule whose action triggered
       this YYERROR.  */
    yypop_ (yylen);
    yylen = 0;
    YY_STACK_PRINT ();
    goto yyerrlab1;


  /*-------------------------------------------------------------.
  | yyerrlab1 -- common code for both syntax error and YYERROR.  |
  `-------------------------------------------------------------*/
  yyerrlab1:
    yyerrstatus_ = 3;   // Each real token shifted decrements this.
    // Pop stack until we find a state that shifts the error token.
    for (;;)
      {
        yyn = yypact_[+yystack_[0].state];
        if (!yy_pact_value_is_default_ (yyn))
          {
            yyn += symbol_kind::S_YYerror;
            if (0 <= yyn && yyn <= yylast_
                && yycheck_[yyn] == symbol_kind::S_YYerror)
              {
                yyn = yytable_[yyn];
                if (0 < yyn)
                  break;
              }
          }

        // Pop the current state because it cannot handle the error token.
        if (yystack_.size () == 1)
          YYABORT;

        yyerror_range[1].location = yystack_[0].location;
        yy_destroy_ ("Error: popping", yystack_[0]);
        yypop_ ();
        YY_STACK_PRINT ();
      }
    {
      stack_symbol_type error_token;

      yyerror_range[2].location = yyla.location;
      YYLLOC_DEFAULT (error_token.location, yyerror_range, 2);

      // Shift the error token.
      error_token.state = state_type (yyn);
      yypush_ ("Shifting", YY_MOVE (error_token));
    }
    goto yynewstate;


  /*-------------------------------------.
  | yyacceptlab -- YYACCEPT comes here.  |
  `-------------------------------------*/
  yyacceptlab:
    yyresult = 0;
    goto yyreturn;


  /*-----------------------------------.
  | yyabortlab -- YYABORT comes here.  |
  `-----------------------------------*/
  yyabortlab:
    yyresult = 1;
    goto yyreturn;


  /*-----------------------------------------------------.
  | yyreturn -- parsing is finished, return the result.  |
  `-----------------------------------------------------*/
  yyreturn:
    if (!yyla.empty ())
      yy_destroy_ ("Cleanup: discarding lookahead", yyla);

    /* Do not reclaim the symbols of the rule whose action triggered
       this YYABORT or YYACCEPT.  */
    yypop_ (yylen);
    YY_STACK_PRINT ();
    while (1 < yystack_.size ())
      {
        yy_destroy_ ("Cleanup: popping", yystack_[0]);
        yypop_ ();
      }

    return yyresult;
  }
#if YY_EXCEPTIONS
    catch (...)
      {
        YYCDEBUG << "Exception caught: cleaning lookahead and stack\n";
        // Do not try to display the values of the reclaimed symbols,
        // as their printers might throw an exception.
        if (!yyla.empty ())
          yy_destroy_ (YY_NULLPTR, yyla);

        while (1 < yystack_.size ())
          {
            yy_destroy_ (YY_NULLPTR, yystack_[0]);
            yypop_ ();
          }
        throw;
      }
#endif // YY_EXCEPTIONS
  }

  void
  Parser::error (const syntax_error& yyexc)
  {
    error (yyexc.location, yyexc.what ());
  }

  const char *
  Parser::symbol_name (symbol_kind_type yysymbol)
  {
    static const char *const yy_sname[] =
    {
    "end of file", "error", "invalid token", "an identifier", "a type name",
  "an integer", "a number", "a character", "a string", "'int'", "'char'",
  "'float'", "'void'", "'long'", "'short'", "'unsigned'", "'signed'",
  "'double'", "'const'", "'static'", "'extern'", "'if'", "'else'",
  "'while'", "'for'", "'do'", "'return'", "'break'", "'continue'",
  "'struct'", "'typedef'", "'enum'", "'sizeof'", "'switch'", "'case'",
  "'default'", "'goto'", "'+'", "'-'", "'*'", "'/'", "'%'", "'='", "'+='",
  "'-='", "'*='", "'/='", "'%='", "'&='", "'|='", "'^='", "'<<='", "'>>='",
  "'=='", "'!='", "'<'", "'<='", "'>'", "'>='", "'&&'", "'||'", "'!'",
  "'&'", "'|'", "'^'", "'~'", "'<<'", "'>>'", "'++'", "'--'", "'.'",
  "'->'", "'?'", "':'", "'('", "')'", "'{'", "'}'", "'['", "']'", "','",
  "';'", "an invalid token", "LOWER_THAN_ELSE", "$accept", "program",
  "top_decl", "decl_pushed", "decl_specs", "prefix_list", "prefix_item",
  "type_end", "struct_spec", "$@1", "@2", "fields", "field_declarators",
  "field_declarator", "enum_spec", "$@3", "$@4", "enumerators",
  "enum_list", "enumerator", "typedef_decl", "typedef_list",
  "typedef_item", "stars", "stars1", "named", "type_name", "dims", "dim",
  "var_declarators", "var_declarator", "@5", "init_opt", "init_node",
  "init_list", "init_items", "params", "param_list", "param", "compound",
  "block_items", "block_item", "local_decl", "statement", "switch_head",
  "switch_items", "switch_item", "expr_opt", "expression", "assignment",
  "assign_op", "conditional", "logical_or", "logical_and", "bit_or",
  "bit_xor", "bit_and", "equality", "relational", "shift", "additive",
  "multiplicative", "cast", "unary", "postfix", "args", "primary", YY_NULLPTR
    };
    return yy_sname[yysymbol];
  }



  // Parser::context.
  Parser::context::context (const Parser& yyparser, const symbol_type& yyla)
    : yyparser_ (yyparser)
    , yyla_ (yyla)
  {}

  int
  Parser::context::expected_tokens (symbol_kind_type yyarg[], int yyargn) const
  {
    // Actual number of expected tokens
    int yycount = 0;

    const int yyn = yypact_[+yyparser_.yystack_[0].state];
    if (!yy_pact_value_is_default_ (yyn))
      {
        /* Start YYX at -YYN if negative to avoid negative indexes in
           YYCHECK.  In other words, skip the first -YYN actions for
           this state because they are default actions.  */
        const int yyxbegin = yyn < 0 ? -yyn : 0;
        // Stay within bounds of both yycheck and yytname.
        const int yychecklim = yylast_ - yyn + 1;
        const int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
        for (int yyx = yyxbegin; yyx < yyxend; ++yyx)
          if (yycheck_[yyx + yyn] == yyx && yyx != symbol_kind::S_YYerror
              && !yy_table_value_is_error_ (yytable_[yyx + yyn]))
            {
              if (!yyarg)
                ++yycount;
              else if (yycount == yyargn)
                return 0;
              else
                yyarg[yycount++] = YY_CAST (symbol_kind_type, yyx);
            }
      }

    if (yyarg && yycount == 0 && 0 < yyargn)
      yyarg[0] = symbol_kind::S_YYEMPTY;
    return yycount;
  }








  const short Parser::yypact_ninf_ = -249;

  const signed char Parser::yytable_ninf_ = -1;

  const short
  Parser::yypact_[] =
  {
    -249,   182,  -249,   -12,  -249,  -249,  -249,  -249,  -249,  -249,
    -249,  -249,  -249,  -249,  -249,  -249,  -249,     5,   793,     6,
    -249,    10,   -51,   793,  -249,  -249,  -249,  -249,  -249,  -249,
    -249,   -30,  -249,    10,  -249,   -28,  -249,  -249,  -249,    22,
     -42,    29,  -249,  -249,  -249,  -249,  -249,  -249,    65,  -249,
    -249,  -249,    36,  -249,  -249,  -249,  -249,   793,   -10,    10,
    -249,  -249,   535,    10,  -249,   -10,    36,    24,     1,     0,
    -249,    10,    20,    12,  -249,   111,  -249,    55,  -249,  -249,
     558,  -249,    10,  -249,    38,   684,  -249,    36,    22,  -249,
     -38,   770,  -249,  -249,  -249,  -249,  -249,   704,   684,   684,
     684,   684,   684,   684,   684,   684,   666,  -249,    34,   -37,
     106,    72,   107,   119,   101,    76,    90,   121,   112,  -249,
    -249,    33,  -249,   553,  -249,  -249,    80,  -249,  -249,  -249,
    -249,  -249,   -10,  -249,  -249,  -249,   104,  -249,   666,  -249,
    -249,  -249,  -249,  -249,  -249,  -249,  -249,  -249,   149,   130,
     -33,  -249,    78,  -249,   684,   684,   684,   684,   684,   684,
     684,   684,   684,   684,   684,   684,   684,   684,   684,   684,
     684,   684,   684,  -249,  -249,   207,   214,   593,   684,   553,
    -249,  -249,    10,  -249,   -10,   308,   148,  -249,  -249,    -6,
     684,  -249,   684,  -249,  -249,  -249,  -249,  -249,  -249,  -249,
    -249,  -249,  -249,  -249,   684,   106,   -44,    72,   107,   119,
     101,    76,    76,    90,    90,    90,    90,   121,   121,   112,
     112,  -249,  -249,  -249,  -249,  -249,  -249,  -249,   -25,    83,
    -249,   142,   140,  -249,    17,   150,   151,   152,   153,   462,
     684,   141,   143,   157,   248,  -249,  -249,    10,   188,  -249,
    -249,  -249,  -249,  -249,  -249,    89,  -249,   -47,  -249,  -249,
    -249,   684,  -249,   684,  -249,  -249,   553,  -249,  -249,   385,
     684,   684,   666,   197,   249,   190,   193,  -249,  -249,   684,
     194,    97,  -249,   229,  -249,  -249,  -249,  -249,  -249,  -249,
    -249,   -23,   -21,   684,   195,   462,   200,  -249,    -8,  -249,
    -249,    31,   684,   204,  -249,  -249,  -249,   462,   462,   198,
     684,   684,   202,  -249,   208,  -249,   258,  -249,   684,   203,
       4,  -249,  -249,   462,   210,   684,   205,  -249,   462,   212,
    -249,  -249,   462,  -249
  };

  const unsigned char
  Parser::yydefact_[] =
  {
       2,     0,     1,     0,    31,    24,    25,    26,    28,    22,
      23,    20,    21,    27,    18,    17,    19,     0,     0,     0,
       3,     0,    11,    13,    15,    12,    29,    30,     4,    10,
       9,    32,    35,     0,    11,    42,    45,    63,    60,     0,
      67,     0,    71,     5,    16,    14,    33,    37,     0,    55,
      67,    43,    47,    64,    65,    62,    61,    84,    73,     0,
       6,    37,     0,     0,    54,    57,    47,    52,     0,    48,
      50,    89,     0,    85,    87,     0,    68,    75,    67,    72,
       0,    36,     0,    56,     0,     0,    46,    49,    90,    67,
       0,     0,   195,   196,   197,   198,   199,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    69,     0,   142,
     144,   146,   148,   150,   152,   154,   157,   162,   165,   168,
     172,   174,   185,     0,    74,    34,     0,    39,    67,    44,
      53,    51,    91,    95,     8,     7,     0,    88,     0,   176,
     182,   181,   178,   179,   177,   180,   183,   184,    58,     0,
       0,   127,   129,    70,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   191,   192,     0,     0,     0,     0,    79,
      76,    77,     0,    38,    41,     0,     0,    67,    66,    59,
       0,   200,     0,   131,   132,   133,   134,   135,   136,   137,
     138,   139,   140,   141,     0,   145,     0,   147,   149,   151,
     153,   155,   156,   158,   159,   160,   161,   163,   164,   166,
     167,   169,   170,   171,   189,   190,   187,   193,     0,     0,
      82,     0,    80,    40,     0,   195,     0,     0,     0,     0,
     125,     0,     0,     0,     0,    92,   118,     0,    11,    99,
     103,    96,    98,    97,   120,     0,    86,     0,   173,   128,
     130,     0,   188,     0,   186,    78,    81,    94,   101,     0,
       0,     0,   125,   195,     0,     0,   126,   111,   112,     0,
       0,     0,   100,     0,   117,   175,   143,   194,    83,    93,
     114,     0,     0,   125,     0,     0,     0,   110,     0,   113,
     102,     0,     0,     0,   115,   124,   121,     0,     0,     0,
     125,     0,     0,   116,     0,   123,   104,   106,   125,     0,
       0,   119,   122,     0,     0,   125,     0,   105,     0,     0,
     107,   109,     0,   108
  };

  const short
  Parser::yypgoto_[] =
  {
    -249,  -249,  -249,     9,    -1,  -249,   260,   265,  -249,  -249,
    -249,   228,  -249,   110,  -249,  -249,  -249,   227,  -249,   209,
     294,  -249,   236,  -249,   -49,   -18,   162,   -45,  -249,    54,
     243,  -249,  -249,  -173,  -249,  -249,  -249,  -249,   213,   217,
    -249,    25,    58,  -232,  -249,  -249,  -249,  -248,  -104,  -119,
    -249,   -74,  -249,   189,   186,   191,   192,   196,    43,   -24,
      42,    47,   -84,   252,  -249,  -249,  -249
  };

  const short
  Parser::yydefgoto_[] =
  {
       0,     1,    20,   247,    34,    23,    24,    25,    26,    61,
      47,    62,   126,   127,    27,    66,    52,    68,    69,    70,
     249,    48,    49,   188,    39,    50,   149,    58,    76,    41,
      42,    77,   124,   180,   231,   232,    72,    73,    74,   250,
     185,   251,   252,   253,   254,   283,   306,   275,   255,   151,
     204,   152,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   228,   122
  };

  const short
  Parser::yytable_[] =
  {
      22,   108,   150,    40,   181,    65,   230,   274,    31,    35,
      21,   130,    55,    37,   140,   141,   142,   143,   144,   145,
     146,   147,    88,   154,   294,    53,    54,    33,   285,   261,
      43,    75,    57,    56,   150,   155,   192,   290,   133,    67,
      55,    78,   191,   134,   132,   309,    46,   192,    51,    38,
     262,   206,   307,    89,   308,   263,    71,   192,   227,   192,
     181,    56,   319,   290,   128,    29,    85,   312,    75,    30,
     324,    82,   192,   259,   229,   316,   317,   329,    86,   326,
      87,    32,    36,   184,   192,   260,   221,   222,   223,    82,
      71,   327,    91,   288,   267,    90,   331,   123,   268,   189,
     333,   173,   174,   175,   176,   148,   258,   177,   313,    59,
      60,   178,   268,   153,    92,   129,    93,    94,    95,    96,
     193,   194,   195,   196,   197,   198,   199,   200,   201,   202,
     203,   162,   163,   164,   165,   157,   276,   148,   213,   214,
     215,   216,   257,    97,   287,    63,    64,   181,    98,    99,
     100,   170,   171,   172,   160,   161,   166,   167,   168,   169,
     182,   183,   264,   192,   128,   156,   291,   292,   276,   192,
     284,   158,   101,   102,   186,   298,   103,    59,   300,   104,
     105,   159,     2,     3,   248,   106,     4,   286,    38,   276,
     107,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,   211,   212,   190,   276,   320,   217,   218,
     224,    17,    18,    19,   276,   219,   220,   225,   256,   265,
     266,   276,   277,   269,   278,   270,   271,   272,   314,    78,
     301,   279,   273,     4,    93,    94,    95,    96,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
     236,   280,   237,   238,   239,   240,   241,   242,    17,    18,
      19,    97,   243,   302,   303,   244,    98,    99,   100,   282,
     295,   297,   296,   192,   311,   299,   310,   315,   321,   318,
     323,   322,   248,    44,   325,   328,   330,   332,    45,    80,
     101,   102,   233,    84,   103,    28,   131,   104,   105,    83,
     187,   281,    79,   106,   137,   133,   304,   135,   305,   234,
     246,   235,     4,    93,    94,    95,    96,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,   236,
     293,   237,   238,   239,   240,   241,   242,    17,    18,    19,
      97,   243,   207,   205,   244,    98,    99,   100,   208,   139,
     209,     0,     0,     0,     0,   210,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   101,
     102,     0,     0,   103,     0,     0,   104,   105,     0,     0,
       0,     0,   106,     0,   133,   245,     0,     0,   273,   246,
      93,    94,    95,    96,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   236,     0,   237,   238,
     239,   240,   241,   242,     0,     0,     0,    97,   243,     0,
       0,   244,    98,    99,   100,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   101,   102,     0,     0,
     103,     0,     0,   104,   105,     0,     0,     0,     0,   106,
       0,   133,   289,     0,     0,   273,   246,    93,    94,    95,
      96,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   236,     0,   237,   238,   239,   240,   241,
     242,     0,     0,     0,    97,   243,     0,     0,   244,    98,
      99,   100,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   101,   102,     0,     0,   103,     0,     0,
     104,   105,     0,     0,     0,     0,   106,     0,   133,     4,
       0,     0,     0,   246,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    92,     0,    93,    94,
      95,    96,     4,     0,    17,     0,    19,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,     0,
       0,     0,     0,     0,     0,    97,     0,    17,     0,    19,
      98,    99,   100,     0,     0,     0,    92,     0,    93,    94,
      95,    96,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    81,     0,   101,   102,     0,     0,   103,     0,
       0,   104,   105,     0,     0,    97,     0,   106,     0,   179,
      98,    99,   100,     0,     0,   125,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   101,   102,     0,     0,   103,     0,
       0,   104,   105,     0,     0,     0,     0,   106,   226,    92,
       4,    93,    94,    95,    96,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    92,     0,    93,
      94,    95,    96,     0,     0,    17,     0,    19,    97,     0,
       0,     0,     0,    98,    99,   100,     0,    92,     0,    93,
      94,    95,    96,     0,     0,     0,    97,     0,     0,     0,
       0,    98,    99,   100,     0,     0,     0,   101,   102,     0,
       0,   103,     0,     0,   104,   105,    97,     0,     0,     0,
     106,    98,    99,   100,     0,   101,   102,     0,     0,   103,
       0,     0,   104,   105,     0,     0,     0,     0,   106,     0,
       0,     0,     0,     0,     0,   101,   102,     0,     0,   103,
       0,     0,   104,   105,     4,     0,     0,     0,   138,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,     0,     0,     0,     0,     0,     0,     4,     0,    17,
       0,    19,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,     0,     0,     0,     0,     0,     0,
       0,     0,    17,     0,    19,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     136
  };

  const short
  Parser::yycheck_[] =
  {
       1,    75,   106,    21,   123,    50,   179,   239,     3,     3,
       1,    85,    18,     3,    98,    99,   100,   101,   102,   103,
     104,   105,    71,    60,   272,     3,     4,    18,    75,    73,
      81,    78,    74,    39,   138,    72,    80,   269,    76,     3,
      18,    59,    75,    81,    89,   293,    76,    80,    76,    39,
      75,   155,    75,    71,    75,    80,    57,    80,   177,    80,
     179,    39,   310,   295,    82,    77,    42,    75,    78,    81,
     318,    62,    80,   192,   178,   307,   308,   325,    77,    75,
      80,    76,    76,   128,    80,   204,   170,   171,   172,    80,
      91,   323,    80,   266,    77,    75,   328,    42,    81,   148,
     332,    68,    69,    70,    71,   106,   190,    74,    77,    80,
      81,    78,    81,    79,     3,    77,     5,     6,     7,     8,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    55,    56,    57,    58,    63,   240,   138,   162,   163,
     164,   165,   187,    32,   263,    80,    81,   266,    37,    38,
      39,    39,    40,    41,    53,    54,    66,    67,    37,    38,
      80,    81,    79,    80,   182,    59,   270,   271,   272,    80,
      81,    64,    61,    62,    70,   279,    65,    80,    81,    68,
      69,    62,     0,     1,   185,    74,     4,   261,    39,   293,
      79,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,   160,   161,    75,   310,   311,   166,   167,
       3,    29,    30,    31,   318,   168,   169,     3,    70,    77,
      80,   325,    81,    73,    81,    74,    74,    74,   302,   247,
       1,    74,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,     3,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    81,
      73,    81,    23,    80,    74,    81,    81,    73,    76,    81,
      22,    73,   283,    23,    81,    75,    81,    75,    23,    61,
      61,    62,   182,    66,    65,     1,    87,    68,    69,    63,
     138,   247,    59,    74,    91,    76,    77,    90,   283,     1,
      81,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
     272,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,   156,   154,    36,    37,    38,    39,   157,    97,
     158,    -1,    -1,    -1,    -1,   159,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    61,
      62,    -1,    -1,    65,    -1,    -1,    68,    69,    -1,    -1,
      -1,    -1,    74,    -1,    76,    77,    -1,    -1,     3,    81,
       5,     6,     7,     8,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    21,    -1,    23,    24,
      25,    26,    27,    28,    -1,    -1,    -1,    32,    33,    -1,
      -1,    36,    37,    38,    39,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    61,    62,    -1,    -1,
      65,    -1,    -1,    68,    69,    -1,    -1,    -1,    -1,    74,
      -1,    76,    77,    -1,    -1,     3,    81,     5,     6,     7,
       8,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    21,    -1,    23,    24,    25,    26,    27,
      28,    -1,    -1,    -1,    32,    33,    -1,    -1,    36,    37,
      38,    39,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    61,    62,    -1,    -1,    65,    -1,    -1,
      68,    69,    -1,    -1,    -1,    -1,    74,    -1,    76,     4,
      -1,    -1,    -1,    81,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,     3,    -1,     5,     6,
       7,     8,     4,    -1,    29,    -1,    31,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    -1,
      -1,    -1,    -1,    -1,    -1,    32,    -1,    29,    -1,    31,
      37,    38,    39,    -1,    -1,    -1,     3,    -1,     5,     6,
       7,     8,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    77,    -1,    61,    62,    -1,    -1,    65,    -1,
      -1,    68,    69,    -1,    -1,    32,    -1,    74,    -1,    76,
      37,    38,    39,    -1,    -1,    77,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    61,    62,    -1,    -1,    65,    -1,
      -1,    68,    69,    -1,    -1,    -1,    -1,    74,    75,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,     3,    -1,     5,
       6,     7,     8,    -1,    -1,    29,    -1,    31,    32,    -1,
      -1,    -1,    -1,    37,    38,    39,    -1,     3,    -1,     5,
       6,     7,     8,    -1,    -1,    -1,    32,    -1,    -1,    -1,
      -1,    37,    38,    39,    -1,    -1,    -1,    61,    62,    -1,
      -1,    65,    -1,    -1,    68,    69,    32,    -1,    -1,    -1,
      74,    37,    38,    39,    -1,    61,    62,    -1,    -1,    65,
      -1,    -1,    68,    69,    -1,    -1,    -1,    -1,    74,    -1,
      -1,    -1,    -1,    -1,    -1,    61,    62,    -1,    -1,    65,
      -1,    -1,    68,    69,     4,    -1,    -1,    -1,    74,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    -1,    -1,    -1,    -1,    -1,    -1,     4,    -1,    29,
      -1,    31,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    29,    -1,    31,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      70
  };

  const unsigned char
  Parser::yystos_[] =
  {
       0,    85,     0,     1,     4,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    29,    30,    31,
      86,    87,    88,    89,    90,    91,    92,    98,   104,    77,
      81,     3,    76,    87,    88,     3,    76,     3,    39,   108,
     109,   113,   114,    81,    90,    91,    76,    94,   105,   106,
     109,    76,   100,     3,     4,    18,    39,    74,   111,    80,
      81,    93,    95,    80,    81,   111,    99,     3,   101,   102,
     103,    88,   120,   121,   122,    78,   112,   115,   109,   114,
      95,    77,    87,   106,   101,    42,    77,    80,   108,   109,
      75,    80,     3,     5,     6,     7,     8,    32,    37,    38,
      39,    61,    62,    65,    68,    69,    74,    79,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   146,
     147,   148,   150,    42,   116,    77,    96,    97,   109,    77,
     135,   103,   111,    76,    81,   123,    70,   122,    74,   147,
     146,   146,   146,   146,   146,   146,   146,   146,    88,   110,
     132,   133,   135,    79,    60,    72,    59,    63,    64,    62,
      53,    54,    55,    56,    57,    58,    66,    67,    37,    38,
      39,    40,    41,    68,    69,    70,    71,    74,    78,    76,
     117,   133,    80,    81,   111,   124,    70,   110,   107,   108,
      75,    75,    80,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,   134,   137,   132,   138,   139,   140,
     141,   142,   142,   143,   143,   143,   143,   144,   144,   145,
     145,   146,   146,   146,     3,     3,    75,   133,   149,   132,
     117,   118,   119,    97,     1,     3,    21,    23,    24,    25,
      26,    27,    28,    33,    36,    77,    81,    87,    88,   104,
     123,   125,   126,   127,   128,   132,    70,   111,   146,   133,
     133,    73,    75,    80,    79,    77,    80,    77,    81,    73,
      74,    74,    74,     3,   127,   131,   132,    81,    81,    74,
       3,   113,    81,   129,    81,    75,   135,   133,   117,    77,
     127,   132,   132,   126,   131,    73,    23,    81,   132,    81,
      81,     1,    34,    35,    77,   125,   130,    75,    75,   131,
      81,    74,    75,    77,   135,    73,   127,   127,    81,   131,
     132,    76,    73,    22,   131,    81,    75,   127,    75,   131,
      81,   127,    75,   127
  };

  const unsigned char
  Parser::yyr1_[] =
  {
       0,    84,    85,    85,    86,    86,    86,    86,    86,    86,
      86,    87,    88,    88,    88,    89,    89,    90,    90,    90,
      90,    90,    90,    90,    90,    91,    91,    91,    91,    91,
      91,    91,    92,    93,    92,    94,    92,    95,    95,    96,
      96,    97,    98,    99,    98,   100,    98,   101,   101,   101,
     102,   102,   103,   103,   104,   105,   105,   106,   107,   107,
     108,   108,   108,   109,   109,   109,   110,   111,   111,   112,
     112,   113,   113,   115,   114,   116,   116,   117,   117,   118,
     118,   118,   119,   119,   120,   120,   120,   121,   121,   122,
     122,   122,   123,   123,   123,   124,   124,   125,   125,   125,
     125,   125,   126,   127,   127,   127,   127,   127,   127,   127,
     127,   127,   127,   127,   127,   127,   127,   127,   127,   128,
     129,   129,   130,   130,   130,   131,   131,   132,   132,   133,
     133,   134,   134,   134,   134,   134,   134,   134,   134,   134,
     134,   134,   135,   135,   136,   136,   137,   137,   138,   138,
     139,   139,   140,   140,   141,   141,   141,   142,   142,   142,
     142,   142,   143,   143,   143,   144,   144,   144,   145,   145,
     145,   145,   146,   146,   147,   147,   147,   147,   147,   147,
     147,   147,   147,   147,   147,   148,   148,   148,   148,   148,
     148,   148,   148,   149,   149,   150,   150,   150,   150,   150,
     150
  };

  const signed char
  Parser::yyr2_[] =
  {
       0,     2,     0,     2,     1,     2,     3,     6,     6,     2,
       2,     1,     1,     1,     2,     1,     2,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     2,     0,     6,     0,     5,     0,     4,     1,
       3,     2,     2,     0,     6,     0,     5,     0,     1,     2,
       1,     3,     1,     3,     4,     1,     3,     2,     0,     1,
       1,     2,     2,     1,     2,     2,     2,     0,     2,     2,
       3,     1,     3,     0,     4,     0,     2,     1,     3,     0,
       1,     2,     1,     3,     0,     1,     5,     1,     3,     1,
       2,     3,     3,     5,     4,     0,     2,     1,     1,     1,
       2,     2,     3,     1,     5,     7,     5,     7,     9,     8,
       3,     2,     2,     3,     3,     3,     4,     2,     1,     5,
       0,     2,     3,     2,     1,     0,     1,     1,     3,     1,
       3,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     5,     1,     3,     1,     3,     1,     3,
       1,     3,     1,     3,     1,     3,     3,     1,     3,     3,
       3,     3,     1,     3,     3,     1,     3,     3,     1,     3,
       3,     3,     1,     4,     1,     5,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     1,     4,     3,     4,     3,
       3,     2,     2,     1,     3,     1,     1,     1,     1,     1,
       3
  };




#if YYDEBUG
  const short
  Parser::yyrline_[] =
  {
       0,   194,   194,   195,   199,   200,   201,   205,   225,   230,
     231,   240,   246,   247,   256,   265,   266,   270,   271,   272,
     273,   274,   275,   276,   277,   282,   283,   284,   285,   286,
     287,   288,   292,   298,   298,   305,   305,   315,   316,   320,
     321,   325,   334,   335,   335,   336,   336,   340,   341,   342,
     346,   347,   351,   352,   358,   362,   363,   367,   377,   378,
     382,   383,   384,   392,   393,   394,   398,   402,   403,   407,
     408,   418,   419,   423,   423,   435,   436,   455,   456,   464,
     465,   466,   470,   471,   477,   478,   479,   483,   484,   488,
     494,   500,   512,   517,   525,   534,   535,   542,   543,   544,
     545,   546,   550,   564,   565,   568,   571,   574,   577,   580,
     588,   589,   590,   591,   592,   593,   597,   603,   604,   608,
     617,   618,   622,   628,   634,   646,   647,   653,   654,   658,
     659,   670,   671,   672,   673,   674,   675,   676,   677,   678,
     679,   680,   684,   685,   691,   692,   696,   697,   701,   702,
     706,   707,   711,   712,   716,   717,   718,   722,   723,   724,
     725,   726,   730,   731,   732,   736,   737,   738,   742,   743,
     744,   745,   751,   752,   756,   757,   765,   770,   771,   772,
     773,   774,   775,   776,   777,   782,   783,   787,   793,   801,
     805,   809,   813,   820,   821,   825,   830,   839,   840,   841,
     842
  };

  void
  Parser::yy_stack_print_ () const
  {
    *yycdebug_ << "Stack now";
    for (stack_type::const_iterator
           i = yystack_.begin (),
           i_end = yystack_.end ();
         i != i_end; ++i)
      *yycdebug_ << ' ' << int (i->state);
    *yycdebug_ << '\n';
  }

  void
  Parser::yy_reduce_print_ (int yyrule) const
  {
    int yylno = yyrline_[yyrule];
    int yynrhs = yyr2_[yyrule];
    // Print the symbols being reduced, and their result.
    *yycdebug_ << "Reducing stack by rule " << yyrule - 1
               << " (line " << yylno << "):\n";
    // The symbols being reduced.
    for (int yyi = 0; yyi < yynrhs; yyi++)
      YY_SYMBOL_PRINT ("   $" << yyi + 1 << " =",
                       yystack_[(yynrhs) - (yyi + 1)]);
  }
#endif // YYDEBUG


#line 33 "src/parser/parser_bison.y"
} } // minic::bisonp
#line 3569 "src/parser/parser_bison.cpp"

#line 845 "src/parser/parser_bison.y"


namespace minic { namespace bisonp {

namespace {

bool isTypeWord(Tok t) {
    switch (t) {
        case Tok::KW_INT: case Tok::KW_CHAR: case Tok::KW_FLOAT: case Tok::KW_VOID: case Tok::KW_LONG:
        case Tok::KW_SHORT: case Tok::KW_UNSIGNED: case Tok::KW_SIGNED: case Tok::KW_DOUBLE:
            return true;
        default:
            return false;
    }
}

Parser::token_kind_type kindOf(Tok t) {
    using K = Parser::token;
    switch (t) {
        case Tok::IDENT: return K::IDENT;           case Tok::INT_LIT: return K::INT_LIT;
        case Tok::FLOAT_LIT: return K::FLOAT_LIT;   case Tok::CHAR_LIT: return K::CHAR_LIT;
        case Tok::STRING_LIT: return K::STRING_LIT;
        case Tok::KW_INT: return K::INT;            case Tok::KW_CHAR: return K::CHAR;
        case Tok::KW_FLOAT: return K::FLOAT;        case Tok::KW_VOID: return K::VOID;
        case Tok::KW_LONG: return K::LONG;          case Tok::KW_SHORT: return K::SHORT;
        case Tok::KW_UNSIGNED: return K::UNSIGNED;  case Tok::KW_SIGNED: return K::SIGNED;
        case Tok::KW_DOUBLE: return K::DOUBLE;      case Tok::KW_CONST: return K::CONST;
        case Tok::KW_STATIC: return K::STATIC;      case Tok::KW_EXTERN: return K::EXTERN;
        case Tok::KW_IF: return K::IF;              case Tok::KW_ELSE: return K::ELSE;
        case Tok::KW_WHILE: return K::WHILE;        case Tok::KW_FOR: return K::FOR;
        case Tok::KW_DO: return K::DO;              case Tok::KW_RETURN: return K::RETURN;
        case Tok::KW_BREAK: return K::BREAK;        case Tok::KW_CONTINUE: return K::CONTINUE;
        case Tok::KW_STRUCT: return K::STRUCT;      case Tok::KW_TYPEDEF: return K::TYPEDEF;
        case Tok::KW_ENUM: return K::ENUM;          case Tok::KW_SIZEOF: return K::SIZEOF;
        case Tok::KW_SWITCH: return K::SWITCH;      case Tok::KW_CASE: return K::CASE;
        case Tok::KW_DEFAULT: return K::DEFAULT;    case Tok::KW_GOTO: return K::GOTO;
        case Tok::OP_PLUS: return K::PLUS;          case Tok::OP_MINUS: return K::MINUS;
        case Tok::OP_STAR: return K::STAR;          case Tok::OP_SLASH: return K::SLASH;
        case Tok::OP_PERCENT: return K::PERCENT;    case Tok::OP_ASSIGN: return K::ASSIGN;
        case Tok::OP_PLUS_ASSIGN: return K::PLUS_ASSIGN;     case Tok::OP_MINUS_ASSIGN: return K::MINUS_ASSIGN;
        case Tok::OP_STAR_ASSIGN: return K::STAR_ASSIGN;     case Tok::OP_SLASH_ASSIGN: return K::SLASH_ASSIGN;
        case Tok::OP_PERCENT_ASSIGN: return K::PERCENT_ASSIGN; case Tok::OP_AND_ASSIGN: return K::AND_ASSIGN;
        case Tok::OP_OR_ASSIGN: return K::OR_ASSIGN;         case Tok::OP_XOR_ASSIGN: return K::XOR_ASSIGN;
        case Tok::OP_SHL_ASSIGN: return K::SHL_ASSIGN;       case Tok::OP_SHR_ASSIGN: return K::SHR_ASSIGN;
        case Tok::OP_EQ: return K::EQ;              case Tok::OP_NE: return K::NE;
        case Tok::OP_LT: return K::LT;              case Tok::OP_LE: return K::LE;
        case Tok::OP_GT: return K::GT;              case Tok::OP_GE: return K::GE;
        case Tok::OP_AND: return K::ANDAND;         case Tok::OP_OR: return K::OROR;
        case Tok::OP_NOT: return K::NOT;            case Tok::OP_BITAND: return K::BITAND;
        case Tok::OP_BITOR: return K::BITOR;        case Tok::OP_BITXOR: return K::BITXOR;
        case Tok::OP_BITNOT: return K::BITNOT;      case Tok::OP_SHL: return K::SHL;
        case Tok::OP_SHR: return K::SHR;            case Tok::OP_INC: return K::INC;
        case Tok::OP_DEC: return K::DEC;            case Tok::OP_DOT: return K::DOT;
        case Tok::OP_ARROW: return K::ARROW;        case Tok::OP_QUESTION: return K::QUESTION;
        case Tok::OP_COLON: return K::COLON;        case Tok::LPAREN: return K::LPAREN;
        case Tok::RPAREN: return K::RPAREN;         case Tok::LBRACE: return K::LBRACE;
        case Tok::RBRACE: return K::RBRACE;         case Tok::LBRACKET: return K::LBRACKET;
        case Tok::RBRACKET: return K::RBRACKET;     case Tok::COMMA: return K::COMMA;
        case Tok::SEMI: return K::SEMI;             case Tok::END_OF_FILE: return K::YYEOF;
        case Tok::ERROR: return K::LEXERROR;
    }
    return K::LEXERROR;
}

}  // namespace

// The token feed, with the typedef-name feedback described at the top.
Parser::symbol_type yylex(State& st) {
    const Token& t = st.toks[st.next < st.toks.size() ? st.next : st.toks.size() - 1];
    if (st.next < st.toks.size() - 1) ++st.next;
    st.last = t;
    Pos p{t.line, t.col};
    Tok kind = t.type;
    bool typedefName = kind == Tok::IDENT && st.ctx.typedefs.count(t.lexeme) && !st.typeSeen &&
                       st.prev != Tok::KW_STRUCT && st.prev != Tok::KW_ENUM && st.prev != Tok::OP_DOT &&
                       st.prev != Tok::OP_ARROW && st.prev != Tok::KW_GOTO;
    if (typedefName || isTypeWord(kind)) st.typeSeen = true;
    else if (kind != Tok::KW_STATIC && kind != Tok::KW_CONST && kind != Tok::KW_EXTERN) st.typeSeen = false;
    st.prev = kind;

    // The hand-written parser refuses expressions nested more than 256 deep
    // (deeper recursion would exhaust its stack). Bison's stack would not
    // overflow, but every later stage recurses over the tree, so the same
    // limit applies here, counted as unclosed brackets.
    if (kind == Tok::LPAREN || kind == Tok::LBRACKET) {
        if (st.open >= 256) {
            if (!st.tooDeep) {
                st.sink.add(t.line, t.col, "expression nests too deeply (limit 256)");
                st.quietNext = true;
                st.tooDeep = true;
            }
            return Parser::symbol_type(Parser::token::LEXERROR, t, p);
        }
        ++st.open;
    } else if ((kind == Tok::RPAREN || kind == Tok::RBRACKET) && st.open > 0) {
        --st.open;
    }
    if (typedefName) return Parser::symbol_type(Parser::token::TYPEDEF_NAME, t, p);
    if (kind == Tok::END_OF_FILE) return Parser::make_YYEOF(p);
    return Parser::symbol_type(kindOf(kind), t, p);
}

// In the hand-written parser's wording where the expected set allows it:
// "expected ';' but found 'return'", "expected an expression ...", "expected a
// type ...", else "unexpected 'x'". The position is the offending token's in
// every case, which is what the two parsers are compared on.
void Parser::report_syntax_error(const context& ctx) const {
    if (st.quietNext) { st.quietNext = false; return; }
    std::string lexeme = ctx.token() == symbol_kind::S_YYEOF ? "<end of file>" : st.last.lexeme;
    symbol_kind_type expected[YYNTOKENS];
    int n = ctx.expected_tokens(expected, YYNTOKENS);
    auto expects = [&](symbol_kind_type k) {
        for (int i = 0; i < n; ++i) if (expected[i] == k) return true;
        return false;
    };
    std::string what;
    if (n == 1)                                                  what = symbol_name(expected[0]);
    else if (expects(symbol_kind::S_SEMI))                       what = "';'";
    else if (expects(symbol_kind::S_IDENT) && expects(symbol_kind::S_INT_LIT)) what = "an expression";
    else if (expects(symbol_kind::S_INT) && !expects(symbol_kind::S_IDENT))
        what = "a type (int, char, float, void, struct or a typedef name)";
    std::string msg = what.empty() ? "unexpected '" + lexeme + "'"
                                   : "expected " + what + " but found '" + lexeme + "'";
    st.sink.add(ctx.location().line, ctx.location().col, msg);
}

void Parser::error(const location_type& loc, const std::string& msg) {
    st.sink.add(loc.line, loc.col, msg);
}

}}  // namespace minic::bisonp
