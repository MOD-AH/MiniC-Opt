// A Bison parser, made by GNU Bison 3.8.2.

// Skeleton interface for Bison LALR(1) parsers in C++

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


/**
 ** \file src/parser/parser_bison.hpp
 ** Define the minic::bisonp::parser class.
 */

// C++ LALR(1) parser skeleton written by Akim Demaille.

// DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
// especially those whose name start with YY_ or yy_.  They are
// private implementation details that can be changed or removed.

#ifndef YY_YY_SRC_PARSER_PARSER_BISON_HPP_INCLUDED
# define YY_YY_SRC_PARSER_PARSER_BISON_HPP_INCLUDED
// "%code requires" blocks.
#line 52 "src/parser/parser_bison.y"

#include "ast.h"
#include "diagnostic.h"
#include "parse_context.h"
#include "token.h"
#include <memory>
#include <string>
#include <vector>

namespace minic { namespace bisonp {

struct Pos {
    int line = 0, col = 0;
};

// A declarator's stars and name.
struct DeclHead {
    int stars = 0;
    Token name{Tok::IDENT, "", 0, 0};
};

// One parameter as written; a definition requires the name, a prototype not.
struct ParamInfo {
    TypeSpec type;
    bool named = false;
    Token name{Tok::IDENT, "", 0, 0};
    std::vector<ParseContext::Dim> dims;
    Pos at;                 // where the name is (or was expected)
    std::string found;      // the token found instead of a name
    bool plainVoid = false; // `void` alone: f(void)
};

// Everything one parse needs; the generated parser holds a reference.
struct State {
    State(std::vector<Token> t, DiagnosticSink& s) : toks(std::move(t)), sink(s), ctx(s) {}
    std::vector<Token> toks;
    size_t next = 0;
    DiagnosticSink& sink;
    ParseContext ctx;
    std::unique_ptr<Program> prog = std::make_unique<Program>();
    std::vector<ParseContext::TypeWords> declType;   // base type of the declarator list being parsed
    std::vector<SwitchStmt*> switches;
    bool typeSeen = false;     // a type word earlier in the current specifier run
    Tok prev = Tok::END_OF_FILE;
    Token last{Tok::END_OF_FILE, "", 0, 0};   // most recently fed token: the lookahead at a reduction
    int open = 0;              // unclosed ( and [, for the nesting limit
    bool quietNext = false;    // a diagnostic was already given for this syntax error
    bool tooDeep = false;      // the nesting limit was reported (once is enough)
};

}}  // namespace minic::bisonp

// A symbol's location is its first token's.
#define YYLLOC_DEFAULT(Cur, Rhs, N) \
    do { (Cur) = (N) ? YYRHSLOC(Rhs, 1) : YYRHSLOC(Rhs, 0); } while (0)

#line 106 "src/parser/parser_bison.hpp"


# include <cstdlib> // std::abort
# include <iostream>
# include <stdexcept>
# include <string>
# include <vector>

#if defined __cplusplus
# define YY_CPLUSPLUS __cplusplus
#else
# define YY_CPLUSPLUS 199711L
#endif

// Support move semantics when possible.
#if 201103L <= YY_CPLUSPLUS
# define YY_MOVE           std::move
# define YY_MOVE_OR_COPY   move
# define YY_MOVE_REF(Type) Type&&
# define YY_RVREF(Type)    Type&&
# define YY_COPY(Type)     Type
#else
# define YY_MOVE
# define YY_MOVE_OR_COPY   copy
# define YY_MOVE_REF(Type) Type&
# define YY_RVREF(Type)    const Type&
# define YY_COPY(Type)     const Type&
#endif

// Support noexcept when possible.
#if 201103L <= YY_CPLUSPLUS
# define YY_NOEXCEPT noexcept
# define YY_NOTHROW
#else
# define YY_NOEXCEPT
# define YY_NOTHROW throw ()
#endif

// Support constexpr when possible.
#if 201703 <= YY_CPLUSPLUS
# define YY_CONSTEXPR constexpr
#else
# define YY_CONSTEXPR
#endif



#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

#line 33 "src/parser/parser_bison.y"
namespace minic { namespace bisonp {
#line 242 "src/parser/parser_bison.hpp"




  /// A Bison parser.
  class Parser
  {
  public:
#ifdef YYSTYPE
# ifdef __GNUC__
#  pragma GCC message "bison: do not #define YYSTYPE in C++, use %define api.value.type"
# endif
    typedef YYSTYPE value_type;
#else
  /// A buffer to store and retrieve objects.
  ///
  /// Sort of a variant, but does not keep track of the nature
  /// of the stored data, since that knowledge is available
  /// via the current parser state.
  class value_type
  {
  public:
    /// Type of *this.
    typedef value_type self_type;

    /// Empty construction.
    value_type () YY_NOEXCEPT
      : yyraw_ ()
    {}

    /// Construct and fill.
    template <typename T>
    value_type (YY_RVREF (T) t)
    {
      new (yyas_<T> ()) T (YY_MOVE (t));
    }

#if 201103L <= YY_CPLUSPLUS
    /// Non copyable.
    value_type (const self_type&) = delete;
    /// Non copyable.
    self_type& operator= (const self_type&) = delete;
#endif

    /// Destruction, allowed only if empty.
    ~value_type () YY_NOEXCEPT
    {}

# if 201103L <= YY_CPLUSPLUS
    /// Instantiate a \a T in here from \a t.
    template <typename T, typename... U>
    T&
    emplace (U&&... u)
    {
      return *new (yyas_<T> ()) T (std::forward <U>(u)...);
    }
# else
    /// Instantiate an empty \a T in here.
    template <typename T>
    T&
    emplace ()
    {
      return *new (yyas_<T> ()) T ();
    }

    /// Instantiate a \a T in here from \a t.
    template <typename T>
    T&
    emplace (const T& t)
    {
      return *new (yyas_<T> ()) T (t);
    }
# endif

    /// Instantiate an empty \a T in here.
    /// Obsolete, use emplace.
    template <typename T>
    T&
    build ()
    {
      return emplace<T> ();
    }

    /// Instantiate a \a T in here from \a t.
    /// Obsolete, use emplace.
    template <typename T>
    T&
    build (const T& t)
    {
      return emplace<T> (t);
    }

    /// Accessor to a built \a T.
    template <typename T>
    T&
    as () YY_NOEXCEPT
    {
      return *yyas_<T> ();
    }

    /// Const accessor to a built \a T (for %printer).
    template <typename T>
    const T&
    as () const YY_NOEXCEPT
    {
      return *yyas_<T> ();
    }

    /// Swap the content with \a that, of same type.
    ///
    /// Both variants must be built beforehand, because swapping the actual
    /// data requires reading it (with as()), and this is not possible on
    /// unconstructed variants: it would require some dynamic testing, which
    /// should not be the variant's responsibility.
    /// Swapping between built and (possibly) non-built is done with
    /// self_type::move ().
    template <typename T>
    void
    swap (self_type& that) YY_NOEXCEPT
    {
      std::swap (as<T> (), that.as<T> ());
    }

    /// Move the content of \a that to this.
    ///
    /// Destroys \a that.
    template <typename T>
    void
    move (self_type& that)
    {
# if 201103L <= YY_CPLUSPLUS
      emplace<T> (std::move (that.as<T> ()));
# else
      emplace<T> ();
      swap<T> (that);
# endif
      that.destroy<T> ();
    }

# if 201103L <= YY_CPLUSPLUS
    /// Move the content of \a that to this.
    template <typename T>
    void
    move (self_type&& that)
    {
      emplace<T> (std::move (that.as<T> ()));
      that.destroy<T> ();
    }
#endif

    /// Copy the content of \a that to this.
    template <typename T>
    void
    copy (const self_type& that)
    {
      emplace<T> (that.as<T> ());
    }

    /// Destroy the stored \a T.
    template <typename T>
    void
    destroy ()
    {
      as<T> ().~T ();
    }

  private:
#if YY_CPLUSPLUS < 201103L
    /// Non copyable.
    value_type (const self_type&);
    /// Non copyable.
    self_type& operator= (const self_type&);
#endif

    /// Accessor to raw memory as \a T.
    template <typename T>
    T*
    yyas_ () YY_NOEXCEPT
    {
      void *yyp = yyraw_;
      return static_cast<T*> (yyp);
     }

    /// Const accessor to raw memory as \a T.
    template <typename T>
    const T*
    yyas_ () const YY_NOEXCEPT
    {
      const void *yyp = yyraw_;
      return static_cast<const T*> (yyp);
     }

    /// An auxiliary type to compute the largest semantic type.
    union union_type
    {
      // assign_op
      char dummy1[sizeof (AssignOp)];

      // named
      char dummy2[sizeof (DeclHead)];

      // param
      char dummy3[sizeof (ParamInfo)];

      // dim
      char dummy4[sizeof (ParseContext::Dim)];

      // init_node
      char dummy5[sizeof (ParseContext::InitNode)];

      // init_opt
      char dummy6[sizeof (ParseContext::Initializer)];

      // decl_specs
      // prefix_list
      // type_end
      // struct_spec
      // enum_spec
      char dummy7[sizeof (ParseContext::TypeWords)];

      // "an identifier"
      // "a type name"
      // "an integer"
      // "a number"
      // "a character"
      // "a string"
      // "'int'"
      // "'char'"
      // "'float'"
      // "'void'"
      // "'long'"
      // "'short'"
      // "'unsigned'"
      // "'signed'"
      // "'double'"
      // "'const'"
      // "'static'"
      // "'extern'"
      // "'if'"
      // "'else'"
      // "'while'"
      // "'for'"
      // "'do'"
      // "'return'"
      // "'break'"
      // "'continue'"
      // "'struct'"
      // "'typedef'"
      // "'enum'"
      // "'sizeof'"
      // "'switch'"
      // "'case'"
      // "'default'"
      // "'goto'"
      // "'+'"
      // "'-'"
      // "'*'"
      // "'/'"
      // "'%'"
      // "'='"
      // "'+='"
      // "'-='"
      // "'*='"
      // "'/='"
      // "'%='"
      // "'&='"
      // "'|='"
      // "'^='"
      // "'<<='"
      // "'>>='"
      // "'=='"
      // "'!='"
      // "'<'"
      // "'<='"
      // "'>'"
      // "'>='"
      // "'&&'"
      // "'||'"
      // "'!'"
      // "'&'"
      // "'|'"
      // "'^'"
      // "'~'"
      // "'<<'"
      // "'>>'"
      // "'++'"
      // "'--'"
      // "'.'"
      // "'->'"
      // "'?'"
      // "':'"
      // "'('"
      // "')'"
      // "'{'"
      // "'}'"
      // "'['"
      // "']'"
      // "','"
      // "';'"
      // "an invalid token"
      char dummy8[sizeof (Token)];

      // type_name
      // @5
      char dummy9[sizeof (TypeSpec)];

      // decl_pushed
      // prefix_item
      // @2
      // stars
      // stars1
      char dummy10[sizeof (int)];

      // compound
      char dummy11[sizeof (std::unique_ptr<CompoundStmt>)];

      // expr_opt
      // expression
      // assignment
      // conditional
      // logical_or
      // logical_and
      // bit_or
      // bit_xor
      // bit_and
      // equality
      // relational
      // shift
      // additive
      // multiplicative
      // cast
      // unary
      // postfix
      // primary
      char dummy12[sizeof (std::unique_ptr<Expr>)];

      // statement
      char dummy13[sizeof (std::unique_ptr<Stmt>)];

      // var_declarator
      char dummy14[sizeof (std::unique_ptr<VarDecl>)];

      // params
      // param_list
      char dummy15[sizeof (std::vector<ParamInfo>)];

      // dims
      char dummy16[sizeof (std::vector<ParseContext::Dim>)];

      // init_list
      // init_items
      char dummy17[sizeof (std::vector<ParseContext::InitNode>)];

      // args
      char dummy18[sizeof (std::vector<std::unique_ptr<Expr>>)];

      // block_items
      // block_item
      // local_decl
      char dummy19[sizeof (std::vector<std::unique_ptr<Stmt>>)];

      // var_declarators
      char dummy20[sizeof (std::vector<std::unique_ptr<VarDecl>>)];
    };

    /// The size of the largest semantic type.
    enum { size = sizeof (union_type) };

    /// A buffer to store semantic values.
    union
    {
      /// Strongest alignment constraints.
      long double yyalign_me_;
      /// A buffer large enough to store any of the semantic values.
      char yyraw_[size];
    };
  };

#endif
    /// Backward compatibility (Bison 3.8).
    typedef value_type semantic_type;

    /// Symbol locations.
    typedef minic::bisonp::Pos location_type;

    /// Syntax errors thrown from user actions.
    struct syntax_error : std::runtime_error
    {
      syntax_error (const location_type& l, const std::string& m)
        : std::runtime_error (m)
        , location (l)
      {}

      syntax_error (const syntax_error& s)
        : std::runtime_error (s.what ())
        , location (s.location)
      {}

      ~syntax_error () YY_NOEXCEPT YY_NOTHROW;

      location_type location;
    };

    /// Token kinds.
    struct token
    {
      enum token_kind_type
      {
        YYEMPTY = -2,
    YYEOF = 0,                     // "end of file"
    YYerror = 256,                 // error
    YYUNDEF = 257,                 // "invalid token"
    IDENT = 258,                   // "an identifier"
    TYPEDEF_NAME = 259,            // "a type name"
    INT_LIT = 260,                 // "an integer"
    FLOAT_LIT = 261,               // "a number"
    CHAR_LIT = 262,                // "a character"
    STRING_LIT = 263,              // "a string"
    INT = 264,                     // "'int'"
    CHAR = 265,                    // "'char'"
    FLOAT = 266,                   // "'float'"
    VOID = 267,                    // "'void'"
    LONG = 268,                    // "'long'"
    SHORT = 269,                   // "'short'"
    UNSIGNED = 270,                // "'unsigned'"
    SIGNED = 271,                  // "'signed'"
    DOUBLE = 272,                  // "'double'"
    CONST = 273,                   // "'const'"
    STATIC = 274,                  // "'static'"
    EXTERN = 275,                  // "'extern'"
    IF = 276,                      // "'if'"
    ELSE = 277,                    // "'else'"
    WHILE = 278,                   // "'while'"
    FOR = 279,                     // "'for'"
    DO = 280,                      // "'do'"
    RETURN = 281,                  // "'return'"
    BREAK = 282,                   // "'break'"
    CONTINUE = 283,                // "'continue'"
    STRUCT = 284,                  // "'struct'"
    TYPEDEF = 285,                 // "'typedef'"
    ENUM = 286,                    // "'enum'"
    SIZEOF = 287,                  // "'sizeof'"
    SWITCH = 288,                  // "'switch'"
    CASE = 289,                    // "'case'"
    DEFAULT = 290,                 // "'default'"
    GOTO = 291,                    // "'goto'"
    PLUS = 292,                    // "'+'"
    MINUS = 293,                   // "'-'"
    STAR = 294,                    // "'*'"
    SLASH = 295,                   // "'/'"
    PERCENT = 296,                 // "'%'"
    ASSIGN = 297,                  // "'='"
    PLUS_ASSIGN = 298,             // "'+='"
    MINUS_ASSIGN = 299,            // "'-='"
    STAR_ASSIGN = 300,             // "'*='"
    SLASH_ASSIGN = 301,            // "'/='"
    PERCENT_ASSIGN = 302,          // "'%='"
    AND_ASSIGN = 303,              // "'&='"
    OR_ASSIGN = 304,               // "'|='"
    XOR_ASSIGN = 305,              // "'^='"
    SHL_ASSIGN = 306,              // "'<<='"
    SHR_ASSIGN = 307,              // "'>>='"
    EQ = 308,                      // "'=='"
    NE = 309,                      // "'!='"
    LT = 310,                      // "'<'"
    LE = 311,                      // "'<='"
    GT = 312,                      // "'>'"
    GE = 313,                      // "'>='"
    ANDAND = 314,                  // "'&&'"
    OROR = 315,                    // "'||'"
    NOT = 316,                     // "'!'"
    BITAND = 317,                  // "'&'"
    BITOR = 318,                   // "'|'"
    BITXOR = 319,                  // "'^'"
    BITNOT = 320,                  // "'~'"
    SHL = 321,                     // "'<<'"
    SHR = 322,                     // "'>>'"
    INC = 323,                     // "'++'"
    DEC = 324,                     // "'--'"
    DOT = 325,                     // "'.'"
    ARROW = 326,                   // "'->'"
    QUESTION = 327,                // "'?'"
    COLON = 328,                   // "':'"
    LPAREN = 329,                  // "'('"
    RPAREN = 330,                  // "')'"
    LBRACE = 331,                  // "'{'"
    RBRACE = 332,                  // "'}'"
    LBRACKET = 333,                // "'['"
    RBRACKET = 334,                // "']'"
    COMMA = 335,                   // "','"
    SEMI = 336,                    // "';'"
    LEXERROR = 337,                // "an invalid token"
    LOWER_THAN_ELSE = 338          // LOWER_THAN_ELSE
      };
      /// Backward compatibility alias (Bison 3.6).
      typedef token_kind_type yytokentype;
    };

    /// Token kind, as returned by yylex.
    typedef token::token_kind_type token_kind_type;

    /// Backward compatibility alias (Bison 3.6).
    typedef token_kind_type token_type;

    /// Symbol kinds.
    struct symbol_kind
    {
      enum symbol_kind_type
      {
        YYNTOKENS = 84, ///< Number of tokens.
        S_YYEMPTY = -2,
        S_YYEOF = 0,                             // "end of file"
        S_YYerror = 1,                           // error
        S_YYUNDEF = 2,                           // "invalid token"
        S_IDENT = 3,                             // "an identifier"
        S_TYPEDEF_NAME = 4,                      // "a type name"
        S_INT_LIT = 5,                           // "an integer"
        S_FLOAT_LIT = 6,                         // "a number"
        S_CHAR_LIT = 7,                          // "a character"
        S_STRING_LIT = 8,                        // "a string"
        S_INT = 9,                               // "'int'"
        S_CHAR = 10,                             // "'char'"
        S_FLOAT = 11,                            // "'float'"
        S_VOID = 12,                             // "'void'"
        S_LONG = 13,                             // "'long'"
        S_SHORT = 14,                            // "'short'"
        S_UNSIGNED = 15,                         // "'unsigned'"
        S_SIGNED = 16,                           // "'signed'"
        S_DOUBLE = 17,                           // "'double'"
        S_CONST = 18,                            // "'const'"
        S_STATIC = 19,                           // "'static'"
        S_EXTERN = 20,                           // "'extern'"
        S_IF = 21,                               // "'if'"
        S_ELSE = 22,                             // "'else'"
        S_WHILE = 23,                            // "'while'"
        S_FOR = 24,                              // "'for'"
        S_DO = 25,                               // "'do'"
        S_RETURN = 26,                           // "'return'"
        S_BREAK = 27,                            // "'break'"
        S_CONTINUE = 28,                         // "'continue'"
        S_STRUCT = 29,                           // "'struct'"
        S_TYPEDEF = 30,                          // "'typedef'"
        S_ENUM = 31,                             // "'enum'"
        S_SIZEOF = 32,                           // "'sizeof'"
        S_SWITCH = 33,                           // "'switch'"
        S_CASE = 34,                             // "'case'"
        S_DEFAULT = 35,                          // "'default'"
        S_GOTO = 36,                             // "'goto'"
        S_PLUS = 37,                             // "'+'"
        S_MINUS = 38,                            // "'-'"
        S_STAR = 39,                             // "'*'"
        S_SLASH = 40,                            // "'/'"
        S_PERCENT = 41,                          // "'%'"
        S_ASSIGN = 42,                           // "'='"
        S_PLUS_ASSIGN = 43,                      // "'+='"
        S_MINUS_ASSIGN = 44,                     // "'-='"
        S_STAR_ASSIGN = 45,                      // "'*='"
        S_SLASH_ASSIGN = 46,                     // "'/='"
        S_PERCENT_ASSIGN = 47,                   // "'%='"
        S_AND_ASSIGN = 48,                       // "'&='"
        S_OR_ASSIGN = 49,                        // "'|='"
        S_XOR_ASSIGN = 50,                       // "'^='"
        S_SHL_ASSIGN = 51,                       // "'<<='"
        S_SHR_ASSIGN = 52,                       // "'>>='"
        S_EQ = 53,                               // "'=='"
        S_NE = 54,                               // "'!='"
        S_LT = 55,                               // "'<'"
        S_LE = 56,                               // "'<='"
        S_GT = 57,                               // "'>'"
        S_GE = 58,                               // "'>='"
        S_ANDAND = 59,                           // "'&&'"
        S_OROR = 60,                             // "'||'"
        S_NOT = 61,                              // "'!'"
        S_BITAND = 62,                           // "'&'"
        S_BITOR = 63,                            // "'|'"
        S_BITXOR = 64,                           // "'^'"
        S_BITNOT = 65,                           // "'~'"
        S_SHL = 66,                              // "'<<'"
        S_SHR = 67,                              // "'>>'"
        S_INC = 68,                              // "'++'"
        S_DEC = 69,                              // "'--'"
        S_DOT = 70,                              // "'.'"
        S_ARROW = 71,                            // "'->'"
        S_QUESTION = 72,                         // "'?'"
        S_COLON = 73,                            // "':'"
        S_LPAREN = 74,                           // "'('"
        S_RPAREN = 75,                           // "')'"
        S_LBRACE = 76,                           // "'{'"
        S_RBRACE = 77,                           // "'}'"
        S_LBRACKET = 78,                         // "'['"
        S_RBRACKET = 79,                         // "']'"
        S_COMMA = 80,                            // "','"
        S_SEMI = 81,                             // "';'"
        S_LEXERROR = 82,                         // "an invalid token"
        S_LOWER_THAN_ELSE = 83,                  // LOWER_THAN_ELSE
        S_YYACCEPT = 84,                         // $accept
        S_program = 85,                          // program
        S_top_decl = 86,                         // top_decl
        S_decl_pushed = 87,                      // decl_pushed
        S_decl_specs = 88,                       // decl_specs
        S_prefix_list = 89,                      // prefix_list
        S_prefix_item = 90,                      // prefix_item
        S_type_end = 91,                         // type_end
        S_struct_spec = 92,                      // struct_spec
        S_93_1 = 93,                             // $@1
        S_94_2 = 94,                             // @2
        S_fields = 95,                           // fields
        S_field_declarators = 96,                // field_declarators
        S_field_declarator = 97,                 // field_declarator
        S_enum_spec = 98,                        // enum_spec
        S_99_3 = 99,                             // $@3
        S_100_4 = 100,                           // $@4
        S_enumerators = 101,                     // enumerators
        S_enum_list = 102,                       // enum_list
        S_enumerator = 103,                      // enumerator
        S_typedef_decl = 104,                    // typedef_decl
        S_typedef_list = 105,                    // typedef_list
        S_typedef_item = 106,                    // typedef_item
        S_stars = 107,                           // stars
        S_stars1 = 108,                          // stars1
        S_named = 109,                           // named
        S_type_name = 110,                       // type_name
        S_dims = 111,                            // dims
        S_dim = 112,                             // dim
        S_var_declarators = 113,                 // var_declarators
        S_var_declarator = 114,                  // var_declarator
        S_115_5 = 115,                           // @5
        S_init_opt = 116,                        // init_opt
        S_init_node = 117,                       // init_node
        S_init_list = 118,                       // init_list
        S_init_items = 119,                      // init_items
        S_params = 120,                          // params
        S_param_list = 121,                      // param_list
        S_param = 122,                           // param
        S_compound = 123,                        // compound
        S_block_items = 124,                     // block_items
        S_block_item = 125,                      // block_item
        S_local_decl = 126,                      // local_decl
        S_statement = 127,                       // statement
        S_switch_head = 128,                     // switch_head
        S_switch_items = 129,                    // switch_items
        S_switch_item = 130,                     // switch_item
        S_expr_opt = 131,                        // expr_opt
        S_expression = 132,                      // expression
        S_assignment = 133,                      // assignment
        S_assign_op = 134,                       // assign_op
        S_conditional = 135,                     // conditional
        S_logical_or = 136,                      // logical_or
        S_logical_and = 137,                     // logical_and
        S_bit_or = 138,                          // bit_or
        S_bit_xor = 139,                         // bit_xor
        S_bit_and = 140,                         // bit_and
        S_equality = 141,                        // equality
        S_relational = 142,                      // relational
        S_shift = 143,                           // shift
        S_additive = 144,                        // additive
        S_multiplicative = 145,                  // multiplicative
        S_cast = 146,                            // cast
        S_unary = 147,                           // unary
        S_postfix = 148,                         // postfix
        S_args = 149,                            // args
        S_primary = 150                          // primary
      };
    };

    /// (Internal) symbol kind.
    typedef symbol_kind::symbol_kind_type symbol_kind_type;

    /// The number of tokens.
    static const symbol_kind_type YYNTOKENS = symbol_kind::YYNTOKENS;

    /// A complete symbol.
    ///
    /// Expects its Base type to provide access to the symbol kind
    /// via kind ().
    ///
    /// Provide access to semantic value and location.
    template <typename Base>
    struct basic_symbol : Base
    {
      /// Alias to Base.
      typedef Base super_type;

      /// Default constructor.
      basic_symbol () YY_NOEXCEPT
        : value ()
        , location ()
      {}

#if 201103L <= YY_CPLUSPLUS
      /// Move constructor.
      basic_symbol (basic_symbol&& that)
        : Base (std::move (that))
        , value ()
        , location (std::move (that.location))
      {
        switch (this->kind ())
    {
      case symbol_kind::S_assign_op: // assign_op
        value.move< AssignOp > (std::move (that.value));
        break;

      case symbol_kind::S_named: // named
        value.move< DeclHead > (std::move (that.value));
        break;

      case symbol_kind::S_param: // param
        value.move< ParamInfo > (std::move (that.value));
        break;

      case symbol_kind::S_dim: // dim
        value.move< ParseContext::Dim > (std::move (that.value));
        break;

      case symbol_kind::S_init_node: // init_node
        value.move< ParseContext::InitNode > (std::move (that.value));
        break;

      case symbol_kind::S_init_opt: // init_opt
        value.move< ParseContext::Initializer > (std::move (that.value));
        break;

      case symbol_kind::S_decl_specs: // decl_specs
      case symbol_kind::S_prefix_list: // prefix_list
      case symbol_kind::S_type_end: // type_end
      case symbol_kind::S_struct_spec: // struct_spec
      case symbol_kind::S_enum_spec: // enum_spec
        value.move< ParseContext::TypeWords > (std::move (that.value));
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
        value.move< Token > (std::move (that.value));
        break;

      case symbol_kind::S_type_name: // type_name
      case symbol_kind::S_115_5: // @5
        value.move< TypeSpec > (std::move (that.value));
        break;

      case symbol_kind::S_decl_pushed: // decl_pushed
      case symbol_kind::S_prefix_item: // prefix_item
      case symbol_kind::S_94_2: // @2
      case symbol_kind::S_stars: // stars
      case symbol_kind::S_stars1: // stars1
        value.move< int > (std::move (that.value));
        break;

      case symbol_kind::S_compound: // compound
        value.move< std::unique_ptr<CompoundStmt> > (std::move (that.value));
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
        value.move< std::unique_ptr<Expr> > (std::move (that.value));
        break;

      case symbol_kind::S_statement: // statement
        value.move< std::unique_ptr<Stmt> > (std::move (that.value));
        break;

      case symbol_kind::S_var_declarator: // var_declarator
        value.move< std::unique_ptr<VarDecl> > (std::move (that.value));
        break;

      case symbol_kind::S_params: // params
      case symbol_kind::S_param_list: // param_list
        value.move< std::vector<ParamInfo> > (std::move (that.value));
        break;

      case symbol_kind::S_dims: // dims
        value.move< std::vector<ParseContext::Dim> > (std::move (that.value));
        break;

      case symbol_kind::S_init_list: // init_list
      case symbol_kind::S_init_items: // init_items
        value.move< std::vector<ParseContext::InitNode> > (std::move (that.value));
        break;

      case symbol_kind::S_args: // args
        value.move< std::vector<std::unique_ptr<Expr>> > (std::move (that.value));
        break;

      case symbol_kind::S_block_items: // block_items
      case symbol_kind::S_block_item: // block_item
      case symbol_kind::S_local_decl: // local_decl
        value.move< std::vector<std::unique_ptr<Stmt>> > (std::move (that.value));
        break;

      case symbol_kind::S_var_declarators: // var_declarators
        value.move< std::vector<std::unique_ptr<VarDecl>> > (std::move (that.value));
        break;

      default:
        break;
    }

      }
#endif

      /// Copy constructor.
      basic_symbol (const basic_symbol& that);

      /// Constructors for typed symbols.
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, location_type&& l)
        : Base (t)
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const location_type& l)
        : Base (t)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, AssignOp&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const AssignOp& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, DeclHead&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const DeclHead& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ParamInfo&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ParamInfo& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ParseContext::Dim&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ParseContext::Dim& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ParseContext::InitNode&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ParseContext::InitNode& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ParseContext::Initializer&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ParseContext::Initializer& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ParseContext::TypeWords&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ParseContext::TypeWords& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, Token&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const Token& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, TypeSpec&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const TypeSpec& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, int&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const int& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, std::unique_ptr<CompoundStmt>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const std::unique_ptr<CompoundStmt>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, std::unique_ptr<Expr>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const std::unique_ptr<Expr>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, std::unique_ptr<Stmt>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const std::unique_ptr<Stmt>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, std::unique_ptr<VarDecl>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const std::unique_ptr<VarDecl>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, std::vector<ParamInfo>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const std::vector<ParamInfo>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, std::vector<ParseContext::Dim>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const std::vector<ParseContext::Dim>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, std::vector<ParseContext::InitNode>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const std::vector<ParseContext::InitNode>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, std::vector<std::unique_ptr<Expr>>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const std::vector<std::unique_ptr<Expr>>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, std::vector<std::unique_ptr<Stmt>>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const std::vector<std::unique_ptr<Stmt>>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, std::vector<std::unique_ptr<VarDecl>>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const std::vector<std::unique_ptr<VarDecl>>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

      /// Destroy the symbol.
      ~basic_symbol ()
      {
        clear ();
      }



      /// Destroy contents, and record that is empty.
      void clear () YY_NOEXCEPT
      {
        // User destructor.
        symbol_kind_type yykind = this->kind ();
        basic_symbol<Base>& yysym = *this;
        (void) yysym;
        switch (yykind)
        {
       default:
          break;
        }

        // Value type destructor.
switch (yykind)
    {
      case symbol_kind::S_assign_op: // assign_op
        value.template destroy< AssignOp > ();
        break;

      case symbol_kind::S_named: // named
        value.template destroy< DeclHead > ();
        break;

      case symbol_kind::S_param: // param
        value.template destroy< ParamInfo > ();
        break;

      case symbol_kind::S_dim: // dim
        value.template destroy< ParseContext::Dim > ();
        break;

      case symbol_kind::S_init_node: // init_node
        value.template destroy< ParseContext::InitNode > ();
        break;

      case symbol_kind::S_init_opt: // init_opt
        value.template destroy< ParseContext::Initializer > ();
        break;

      case symbol_kind::S_decl_specs: // decl_specs
      case symbol_kind::S_prefix_list: // prefix_list
      case symbol_kind::S_type_end: // type_end
      case symbol_kind::S_struct_spec: // struct_spec
      case symbol_kind::S_enum_spec: // enum_spec
        value.template destroy< ParseContext::TypeWords > ();
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
        value.template destroy< Token > ();
        break;

      case symbol_kind::S_type_name: // type_name
      case symbol_kind::S_115_5: // @5
        value.template destroy< TypeSpec > ();
        break;

      case symbol_kind::S_decl_pushed: // decl_pushed
      case symbol_kind::S_prefix_item: // prefix_item
      case symbol_kind::S_94_2: // @2
      case symbol_kind::S_stars: // stars
      case symbol_kind::S_stars1: // stars1
        value.template destroy< int > ();
        break;

      case symbol_kind::S_compound: // compound
        value.template destroy< std::unique_ptr<CompoundStmt> > ();
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
        value.template destroy< std::unique_ptr<Expr> > ();
        break;

      case symbol_kind::S_statement: // statement
        value.template destroy< std::unique_ptr<Stmt> > ();
        break;

      case symbol_kind::S_var_declarator: // var_declarator
        value.template destroy< std::unique_ptr<VarDecl> > ();
        break;

      case symbol_kind::S_params: // params
      case symbol_kind::S_param_list: // param_list
        value.template destroy< std::vector<ParamInfo> > ();
        break;

      case symbol_kind::S_dims: // dims
        value.template destroy< std::vector<ParseContext::Dim> > ();
        break;

      case symbol_kind::S_init_list: // init_list
      case symbol_kind::S_init_items: // init_items
        value.template destroy< std::vector<ParseContext::InitNode> > ();
        break;

      case symbol_kind::S_args: // args
        value.template destroy< std::vector<std::unique_ptr<Expr>> > ();
        break;

      case symbol_kind::S_block_items: // block_items
      case symbol_kind::S_block_item: // block_item
      case symbol_kind::S_local_decl: // local_decl
        value.template destroy< std::vector<std::unique_ptr<Stmt>> > ();
        break;

      case symbol_kind::S_var_declarators: // var_declarators
        value.template destroy< std::vector<std::unique_ptr<VarDecl>> > ();
        break;

      default:
        break;
    }

        Base::clear ();
      }

      /// The user-facing name of this symbol.
      const char *name () const YY_NOEXCEPT
      {
        return Parser::symbol_name (this->kind ());
      }

      /// Backward compatibility (Bison 3.6).
      symbol_kind_type type_get () const YY_NOEXCEPT;

      /// Whether empty.
      bool empty () const YY_NOEXCEPT;

      /// Destructive move, \a s is emptied into this.
      void move (basic_symbol& s);

      /// The semantic value.
      value_type value;

      /// The location.
      location_type location;

    private:
#if YY_CPLUSPLUS < 201103L
      /// Assignment operator.
      basic_symbol& operator= (const basic_symbol& that);
#endif
    };

    /// Type access provider for token (enum) based symbols.
    struct by_kind
    {
      /// The symbol kind as needed by the constructor.
      typedef token_kind_type kind_type;

      /// Default constructor.
      by_kind () YY_NOEXCEPT;

#if 201103L <= YY_CPLUSPLUS
      /// Move constructor.
      by_kind (by_kind&& that) YY_NOEXCEPT;
#endif

      /// Copy constructor.
      by_kind (const by_kind& that) YY_NOEXCEPT;

      /// Constructor from (external) token numbers.
      by_kind (kind_type t) YY_NOEXCEPT;



      /// Record that this symbol is empty.
      void clear () YY_NOEXCEPT;

      /// Steal the symbol kind from \a that.
      void move (by_kind& that);

      /// The (internal) type number (corresponding to \a type).
      /// \a empty when empty.
      symbol_kind_type kind () const YY_NOEXCEPT;

      /// Backward compatibility (Bison 3.6).
      symbol_kind_type type_get () const YY_NOEXCEPT;

      /// The symbol kind.
      /// \a S_YYEMPTY when empty.
      symbol_kind_type kind_;
    };

    /// Backward compatibility for a private implementation detail (Bison 3.6).
    typedef by_kind by_type;

    /// "External" symbols: returned by the scanner.
    struct symbol_type : basic_symbol<by_kind>
    {
      /// Superclass.
      typedef basic_symbol<by_kind> super_type;

      /// Empty symbol.
      symbol_type () YY_NOEXCEPT {}

      /// Constructor for valueless symbols, and symbols from each type.
#if 201103L <= YY_CPLUSPLUS
      symbol_type (int tok, location_type l)
        : super_type (token_kind_type (tok), std::move (l))
#else
      symbol_type (int tok, const location_type& l)
        : super_type (token_kind_type (tok), l)
#endif
      {}
#if 201103L <= YY_CPLUSPLUS
      symbol_type (int tok, Token v, location_type l)
        : super_type (token_kind_type (tok), std::move (v), std::move (l))
#else
      symbol_type (int tok, const Token& v, const location_type& l)
        : super_type (token_kind_type (tok), v, l)
#endif
      {}
    };

    /// Build a parser object.
    Parser (State& st_yyarg);
    virtual ~Parser ();

#if 201103L <= YY_CPLUSPLUS
    /// Non copyable.
    Parser (const Parser&) = delete;
    /// Non copyable.
    Parser& operator= (const Parser&) = delete;
#endif

    /// Parse.  An alias for parse ().
    /// \returns  0 iff parsing succeeded.
    int operator() ();

    /// Parse.
    /// \returns  0 iff parsing succeeded.
    virtual int parse ();

#if YYDEBUG
    /// The current debugging stream.
    std::ostream& debug_stream () const YY_ATTRIBUTE_PURE;
    /// Set the current debugging stream.
    void set_debug_stream (std::ostream &);

    /// Type for debugging levels.
    typedef int debug_level_type;
    /// The current debugging level.
    debug_level_type debug_level () const YY_ATTRIBUTE_PURE;
    /// Set the current debugging level.
    void set_debug_level (debug_level_type l);
#endif

    /// Report a syntax error.
    /// \param loc    where the syntax error is found.
    /// \param msg    a description of the syntax error.
    virtual void error (const location_type& loc, const std::string& msg);

    /// Report a syntax error.
    void error (const syntax_error& err);

    /// The user-facing name of the symbol whose (internal) number is
    /// YYSYMBOL.  No bounds checking.
    static const char *symbol_name (symbol_kind_type yysymbol);

    // Implementation of make_symbol for each token kind.
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_YYEOF (location_type l)
      {
        return symbol_type (token::YYEOF, std::move (l));
      }
#else
      static
      symbol_type
      make_YYEOF (const location_type& l)
      {
        return symbol_type (token::YYEOF, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_YYerror (location_type l)
      {
        return symbol_type (token::YYerror, std::move (l));
      }
#else
      static
      symbol_type
      make_YYerror (const location_type& l)
      {
        return symbol_type (token::YYerror, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_YYUNDEF (location_type l)
      {
        return symbol_type (token::YYUNDEF, std::move (l));
      }
#else
      static
      symbol_type
      make_YYUNDEF (const location_type& l)
      {
        return symbol_type (token::YYUNDEF, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_IDENT (Token v, location_type l)
      {
        return symbol_type (token::IDENT, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_IDENT (const Token& v, const location_type& l)
      {
        return symbol_type (token::IDENT, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_TYPEDEF_NAME (Token v, location_type l)
      {
        return symbol_type (token::TYPEDEF_NAME, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_TYPEDEF_NAME (const Token& v, const location_type& l)
      {
        return symbol_type (token::TYPEDEF_NAME, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_INT_LIT (Token v, location_type l)
      {
        return symbol_type (token::INT_LIT, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_INT_LIT (const Token& v, const location_type& l)
      {
        return symbol_type (token::INT_LIT, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_FLOAT_LIT (Token v, location_type l)
      {
        return symbol_type (token::FLOAT_LIT, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_FLOAT_LIT (const Token& v, const location_type& l)
      {
        return symbol_type (token::FLOAT_LIT, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_CHAR_LIT (Token v, location_type l)
      {
        return symbol_type (token::CHAR_LIT, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_CHAR_LIT (const Token& v, const location_type& l)
      {
        return symbol_type (token::CHAR_LIT, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_STRING_LIT (Token v, location_type l)
      {
        return symbol_type (token::STRING_LIT, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_STRING_LIT (const Token& v, const location_type& l)
      {
        return symbol_type (token::STRING_LIT, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_INT (Token v, location_type l)
      {
        return symbol_type (token::INT, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_INT (const Token& v, const location_type& l)
      {
        return symbol_type (token::INT, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_CHAR (Token v, location_type l)
      {
        return symbol_type (token::CHAR, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_CHAR (const Token& v, const location_type& l)
      {
        return symbol_type (token::CHAR, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_FLOAT (Token v, location_type l)
      {
        return symbol_type (token::FLOAT, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_FLOAT (const Token& v, const location_type& l)
      {
        return symbol_type (token::FLOAT, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_VOID (Token v, location_type l)
      {
        return symbol_type (token::VOID, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_VOID (const Token& v, const location_type& l)
      {
        return symbol_type (token::VOID, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LONG (Token v, location_type l)
      {
        return symbol_type (token::LONG, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_LONG (const Token& v, const location_type& l)
      {
        return symbol_type (token::LONG, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_SHORT (Token v, location_type l)
      {
        return symbol_type (token::SHORT, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_SHORT (const Token& v, const location_type& l)
      {
        return symbol_type (token::SHORT, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_UNSIGNED (Token v, location_type l)
      {
        return symbol_type (token::UNSIGNED, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_UNSIGNED (const Token& v, const location_type& l)
      {
        return symbol_type (token::UNSIGNED, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_SIGNED (Token v, location_type l)
      {
        return symbol_type (token::SIGNED, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_SIGNED (const Token& v, const location_type& l)
      {
        return symbol_type (token::SIGNED, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_DOUBLE (Token v, location_type l)
      {
        return symbol_type (token::DOUBLE, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_DOUBLE (const Token& v, const location_type& l)
      {
        return symbol_type (token::DOUBLE, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_CONST (Token v, location_type l)
      {
        return symbol_type (token::CONST, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_CONST (const Token& v, const location_type& l)
      {
        return symbol_type (token::CONST, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_STATIC (Token v, location_type l)
      {
        return symbol_type (token::STATIC, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_STATIC (const Token& v, const location_type& l)
      {
        return symbol_type (token::STATIC, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_EXTERN (Token v, location_type l)
      {
        return symbol_type (token::EXTERN, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_EXTERN (const Token& v, const location_type& l)
      {
        return symbol_type (token::EXTERN, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_IF (Token v, location_type l)
      {
        return symbol_type (token::IF, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_IF (const Token& v, const location_type& l)
      {
        return symbol_type (token::IF, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_ELSE (Token v, location_type l)
      {
        return symbol_type (token::ELSE, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_ELSE (const Token& v, const location_type& l)
      {
        return symbol_type (token::ELSE, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_WHILE (Token v, location_type l)
      {
        return symbol_type (token::WHILE, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_WHILE (const Token& v, const location_type& l)
      {
        return symbol_type (token::WHILE, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_FOR (Token v, location_type l)
      {
        return symbol_type (token::FOR, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_FOR (const Token& v, const location_type& l)
      {
        return symbol_type (token::FOR, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_DO (Token v, location_type l)
      {
        return symbol_type (token::DO, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_DO (const Token& v, const location_type& l)
      {
        return symbol_type (token::DO, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_RETURN (Token v, location_type l)
      {
        return symbol_type (token::RETURN, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_RETURN (const Token& v, const location_type& l)
      {
        return symbol_type (token::RETURN, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_BREAK (Token v, location_type l)
      {
        return symbol_type (token::BREAK, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_BREAK (const Token& v, const location_type& l)
      {
        return symbol_type (token::BREAK, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_CONTINUE (Token v, location_type l)
      {
        return symbol_type (token::CONTINUE, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_CONTINUE (const Token& v, const location_type& l)
      {
        return symbol_type (token::CONTINUE, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_STRUCT (Token v, location_type l)
      {
        return symbol_type (token::STRUCT, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_STRUCT (const Token& v, const location_type& l)
      {
        return symbol_type (token::STRUCT, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_TYPEDEF (Token v, location_type l)
      {
        return symbol_type (token::TYPEDEF, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_TYPEDEF (const Token& v, const location_type& l)
      {
        return symbol_type (token::TYPEDEF, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_ENUM (Token v, location_type l)
      {
        return symbol_type (token::ENUM, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_ENUM (const Token& v, const location_type& l)
      {
        return symbol_type (token::ENUM, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_SIZEOF (Token v, location_type l)
      {
        return symbol_type (token::SIZEOF, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_SIZEOF (const Token& v, const location_type& l)
      {
        return symbol_type (token::SIZEOF, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_SWITCH (Token v, location_type l)
      {
        return symbol_type (token::SWITCH, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_SWITCH (const Token& v, const location_type& l)
      {
        return symbol_type (token::SWITCH, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_CASE (Token v, location_type l)
      {
        return symbol_type (token::CASE, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_CASE (const Token& v, const location_type& l)
      {
        return symbol_type (token::CASE, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_DEFAULT (Token v, location_type l)
      {
        return symbol_type (token::DEFAULT, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_DEFAULT (const Token& v, const location_type& l)
      {
        return symbol_type (token::DEFAULT, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_GOTO (Token v, location_type l)
      {
        return symbol_type (token::GOTO, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_GOTO (const Token& v, const location_type& l)
      {
        return symbol_type (token::GOTO, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_PLUS (Token v, location_type l)
      {
        return symbol_type (token::PLUS, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_PLUS (const Token& v, const location_type& l)
      {
        return symbol_type (token::PLUS, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_MINUS (Token v, location_type l)
      {
        return symbol_type (token::MINUS, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_MINUS (const Token& v, const location_type& l)
      {
        return symbol_type (token::MINUS, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_STAR (Token v, location_type l)
      {
        return symbol_type (token::STAR, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_STAR (const Token& v, const location_type& l)
      {
        return symbol_type (token::STAR, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_SLASH (Token v, location_type l)
      {
        return symbol_type (token::SLASH, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_SLASH (const Token& v, const location_type& l)
      {
        return symbol_type (token::SLASH, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_PERCENT (Token v, location_type l)
      {
        return symbol_type (token::PERCENT, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_PERCENT (const Token& v, const location_type& l)
      {
        return symbol_type (token::PERCENT, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_ASSIGN (Token v, location_type l)
      {
        return symbol_type (token::ASSIGN, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_ASSIGN (const Token& v, const location_type& l)
      {
        return symbol_type (token::ASSIGN, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_PLUS_ASSIGN (Token v, location_type l)
      {
        return symbol_type (token::PLUS_ASSIGN, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_PLUS_ASSIGN (const Token& v, const location_type& l)
      {
        return symbol_type (token::PLUS_ASSIGN, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_MINUS_ASSIGN (Token v, location_type l)
      {
        return symbol_type (token::MINUS_ASSIGN, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_MINUS_ASSIGN (const Token& v, const location_type& l)
      {
        return symbol_type (token::MINUS_ASSIGN, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_STAR_ASSIGN (Token v, location_type l)
      {
        return symbol_type (token::STAR_ASSIGN, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_STAR_ASSIGN (const Token& v, const location_type& l)
      {
        return symbol_type (token::STAR_ASSIGN, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_SLASH_ASSIGN (Token v, location_type l)
      {
        return symbol_type (token::SLASH_ASSIGN, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_SLASH_ASSIGN (const Token& v, const location_type& l)
      {
        return symbol_type (token::SLASH_ASSIGN, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_PERCENT_ASSIGN (Token v, location_type l)
      {
        return symbol_type (token::PERCENT_ASSIGN, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_PERCENT_ASSIGN (const Token& v, const location_type& l)
      {
        return symbol_type (token::PERCENT_ASSIGN, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_AND_ASSIGN (Token v, location_type l)
      {
        return symbol_type (token::AND_ASSIGN, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_AND_ASSIGN (const Token& v, const location_type& l)
      {
        return symbol_type (token::AND_ASSIGN, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_OR_ASSIGN (Token v, location_type l)
      {
        return symbol_type (token::OR_ASSIGN, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_OR_ASSIGN (const Token& v, const location_type& l)
      {
        return symbol_type (token::OR_ASSIGN, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_XOR_ASSIGN (Token v, location_type l)
      {
        return symbol_type (token::XOR_ASSIGN, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_XOR_ASSIGN (const Token& v, const location_type& l)
      {
        return symbol_type (token::XOR_ASSIGN, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_SHL_ASSIGN (Token v, location_type l)
      {
        return symbol_type (token::SHL_ASSIGN, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_SHL_ASSIGN (const Token& v, const location_type& l)
      {
        return symbol_type (token::SHL_ASSIGN, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_SHR_ASSIGN (Token v, location_type l)
      {
        return symbol_type (token::SHR_ASSIGN, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_SHR_ASSIGN (const Token& v, const location_type& l)
      {
        return symbol_type (token::SHR_ASSIGN, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_EQ (Token v, location_type l)
      {
        return symbol_type (token::EQ, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_EQ (const Token& v, const location_type& l)
      {
        return symbol_type (token::EQ, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_NE (Token v, location_type l)
      {
        return symbol_type (token::NE, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_NE (const Token& v, const location_type& l)
      {
        return symbol_type (token::NE, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LT (Token v, location_type l)
      {
        return symbol_type (token::LT, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_LT (const Token& v, const location_type& l)
      {
        return symbol_type (token::LT, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LE (Token v, location_type l)
      {
        return symbol_type (token::LE, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_LE (const Token& v, const location_type& l)
      {
        return symbol_type (token::LE, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_GT (Token v, location_type l)
      {
        return symbol_type (token::GT, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_GT (const Token& v, const location_type& l)
      {
        return symbol_type (token::GT, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_GE (Token v, location_type l)
      {
        return symbol_type (token::GE, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_GE (const Token& v, const location_type& l)
      {
        return symbol_type (token::GE, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_ANDAND (Token v, location_type l)
      {
        return symbol_type (token::ANDAND, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_ANDAND (const Token& v, const location_type& l)
      {
        return symbol_type (token::ANDAND, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_OROR (Token v, location_type l)
      {
        return symbol_type (token::OROR, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_OROR (const Token& v, const location_type& l)
      {
        return symbol_type (token::OROR, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_NOT (Token v, location_type l)
      {
        return symbol_type (token::NOT, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_NOT (const Token& v, const location_type& l)
      {
        return symbol_type (token::NOT, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_BITAND (Token v, location_type l)
      {
        return symbol_type (token::BITAND, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_BITAND (const Token& v, const location_type& l)
      {
        return symbol_type (token::BITAND, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_BITOR (Token v, location_type l)
      {
        return symbol_type (token::BITOR, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_BITOR (const Token& v, const location_type& l)
      {
        return symbol_type (token::BITOR, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_BITXOR (Token v, location_type l)
      {
        return symbol_type (token::BITXOR, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_BITXOR (const Token& v, const location_type& l)
      {
        return symbol_type (token::BITXOR, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_BITNOT (Token v, location_type l)
      {
        return symbol_type (token::BITNOT, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_BITNOT (const Token& v, const location_type& l)
      {
        return symbol_type (token::BITNOT, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_SHL (Token v, location_type l)
      {
        return symbol_type (token::SHL, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_SHL (const Token& v, const location_type& l)
      {
        return symbol_type (token::SHL, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_SHR (Token v, location_type l)
      {
        return symbol_type (token::SHR, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_SHR (const Token& v, const location_type& l)
      {
        return symbol_type (token::SHR, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_INC (Token v, location_type l)
      {
        return symbol_type (token::INC, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_INC (const Token& v, const location_type& l)
      {
        return symbol_type (token::INC, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_DEC (Token v, location_type l)
      {
        return symbol_type (token::DEC, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_DEC (const Token& v, const location_type& l)
      {
        return symbol_type (token::DEC, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_DOT (Token v, location_type l)
      {
        return symbol_type (token::DOT, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_DOT (const Token& v, const location_type& l)
      {
        return symbol_type (token::DOT, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_ARROW (Token v, location_type l)
      {
        return symbol_type (token::ARROW, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_ARROW (const Token& v, const location_type& l)
      {
        return symbol_type (token::ARROW, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_QUESTION (Token v, location_type l)
      {
        return symbol_type (token::QUESTION, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_QUESTION (const Token& v, const location_type& l)
      {
        return symbol_type (token::QUESTION, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_COLON (Token v, location_type l)
      {
        return symbol_type (token::COLON, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_COLON (const Token& v, const location_type& l)
      {
        return symbol_type (token::COLON, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LPAREN (Token v, location_type l)
      {
        return symbol_type (token::LPAREN, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_LPAREN (const Token& v, const location_type& l)
      {
        return symbol_type (token::LPAREN, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_RPAREN (Token v, location_type l)
      {
        return symbol_type (token::RPAREN, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_RPAREN (const Token& v, const location_type& l)
      {
        return symbol_type (token::RPAREN, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LBRACE (Token v, location_type l)
      {
        return symbol_type (token::LBRACE, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_LBRACE (const Token& v, const location_type& l)
      {
        return symbol_type (token::LBRACE, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_RBRACE (Token v, location_type l)
      {
        return symbol_type (token::RBRACE, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_RBRACE (const Token& v, const location_type& l)
      {
        return symbol_type (token::RBRACE, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LBRACKET (Token v, location_type l)
      {
        return symbol_type (token::LBRACKET, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_LBRACKET (const Token& v, const location_type& l)
      {
        return symbol_type (token::LBRACKET, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_RBRACKET (Token v, location_type l)
      {
        return symbol_type (token::RBRACKET, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_RBRACKET (const Token& v, const location_type& l)
      {
        return symbol_type (token::RBRACKET, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_COMMA (Token v, location_type l)
      {
        return symbol_type (token::COMMA, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_COMMA (const Token& v, const location_type& l)
      {
        return symbol_type (token::COMMA, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_SEMI (Token v, location_type l)
      {
        return symbol_type (token::SEMI, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_SEMI (const Token& v, const location_type& l)
      {
        return symbol_type (token::SEMI, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LEXERROR (Token v, location_type l)
      {
        return symbol_type (token::LEXERROR, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_LEXERROR (const Token& v, const location_type& l)
      {
        return symbol_type (token::LEXERROR, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LOWER_THAN_ELSE (location_type l)
      {
        return symbol_type (token::LOWER_THAN_ELSE, std::move (l));
      }
#else
      static
      symbol_type
      make_LOWER_THAN_ELSE (const location_type& l)
      {
        return symbol_type (token::LOWER_THAN_ELSE, l);
      }
#endif


    class context
    {
    public:
      context (const Parser& yyparser, const symbol_type& yyla);
      const symbol_type& lookahead () const YY_NOEXCEPT { return yyla_; }
      symbol_kind_type token () const YY_NOEXCEPT { return yyla_.kind (); }
      const location_type& location () const YY_NOEXCEPT { return yyla_.location; }

      /// Put in YYARG at most YYARGN of the expected tokens, and return the
      /// number of tokens stored in YYARG.  If YYARG is null, return the
      /// number of expected tokens (guaranteed to be less than YYNTOKENS).
      int expected_tokens (symbol_kind_type yyarg[], int yyargn) const;

    private:
      const Parser& yyparser_;
      const symbol_type& yyla_;
    };

  private:
#if YY_CPLUSPLUS < 201103L
    /// Non copyable.
    Parser (const Parser&);
    /// Non copyable.
    Parser& operator= (const Parser&);
#endif


    /// Stored state numbers (used for stacks).
    typedef short state_type;

    /// Report a syntax error
    /// \param yyctx     the context in which the error occurred.
    void report_syntax_error (const context& yyctx) const;
    /// Compute post-reduction state.
    /// \param yystate   the current state
    /// \param yysym     the nonterminal to push on the stack
    static state_type yy_lr_goto_state_ (state_type yystate, int yysym);

    /// Whether the given \c yypact_ value indicates a defaulted state.
    /// \param yyvalue   the value to check
    static bool yy_pact_value_is_default_ (int yyvalue) YY_NOEXCEPT;

    /// Whether the given \c yytable_ value indicates a syntax error.
    /// \param yyvalue   the value to check
    static bool yy_table_value_is_error_ (int yyvalue) YY_NOEXCEPT;

    static const short yypact_ninf_;
    static const signed char yytable_ninf_;

    /// Convert a scanner token kind \a t to a symbol kind.
    /// In theory \a t should be a token_kind_type, but character literals
    /// are valid, yet not members of the token_kind_type enum.
    static symbol_kind_type yytranslate_ (int t) YY_NOEXCEPT;



    // Tables.
    // YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
    // STATE-NUM.
    static const short yypact_[];

    // YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
    // Performed when YYTABLE does not specify something else to do.  Zero
    // means the default is an error.
    static const unsigned char yydefact_[];

    // YYPGOTO[NTERM-NUM].
    static const short yypgoto_[];

    // YYDEFGOTO[NTERM-NUM].
    static const short yydefgoto_[];

    // YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
    // positive, shift that token.  If negative, reduce the rule whose
    // number is the opposite.  If YYTABLE_NINF, syntax error.
    static const short yytable_[];

    static const short yycheck_[];

    // YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
    // state STATE-NUM.
    static const unsigned char yystos_[];

    // YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.
    static const unsigned char yyr1_[];

    // YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.
    static const signed char yyr2_[];


#if YYDEBUG
    // YYRLINE[YYN] -- Source line where rule number YYN was defined.
    static const short yyrline_[];
    /// Report on the debug stream that the rule \a r is going to be reduced.
    virtual void yy_reduce_print_ (int r) const;
    /// Print the state stack on the debug stream.
    virtual void yy_stack_print_ () const;

    /// Debugging level.
    int yydebug_;
    /// Debug stream.
    std::ostream* yycdebug_;

    /// \brief Display a symbol kind, value and location.
    /// \param yyo    The output stream.
    /// \param yysym  The symbol.
    template <typename Base>
    void yy_print_ (std::ostream& yyo, const basic_symbol<Base>& yysym) const;
#endif

    /// \brief Reclaim the memory associated to a symbol.
    /// \param yymsg     Why this token is reclaimed.
    ///                  If null, print nothing.
    /// \param yysym     The symbol.
    template <typename Base>
    void yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const;

  private:
    /// Type access provider for state based symbols.
    struct by_state
    {
      /// Default constructor.
      by_state () YY_NOEXCEPT;

      /// The symbol kind as needed by the constructor.
      typedef state_type kind_type;

      /// Constructor.
      by_state (kind_type s) YY_NOEXCEPT;

      /// Copy constructor.
      by_state (const by_state& that) YY_NOEXCEPT;

      /// Record that this symbol is empty.
      void clear () YY_NOEXCEPT;

      /// Steal the symbol kind from \a that.
      void move (by_state& that);

      /// The symbol kind (corresponding to \a state).
      /// \a symbol_kind::S_YYEMPTY when empty.
      symbol_kind_type kind () const YY_NOEXCEPT;

      /// The state number used to denote an empty symbol.
      /// We use the initial state, as it does not have a value.
      enum { empty_state = 0 };

      /// The state.
      /// \a empty when empty.
      state_type state;
    };

    /// "Internal" symbol: element of the stack.
    struct stack_symbol_type : basic_symbol<by_state>
    {
      /// Superclass.
      typedef basic_symbol<by_state> super_type;
      /// Construct an empty symbol.
      stack_symbol_type ();
      /// Move or copy construction.
      stack_symbol_type (YY_RVREF (stack_symbol_type) that);
      /// Steal the contents from \a sym to build this.
      stack_symbol_type (state_type s, YY_MOVE_REF (symbol_type) sym);
#if YY_CPLUSPLUS < 201103L
      /// Assignment, needed by push_back by some old implementations.
      /// Moves the contents of that.
      stack_symbol_type& operator= (stack_symbol_type& that);

      /// Assignment, needed by push_back by other implementations.
      /// Needed by some other old implementations.
      stack_symbol_type& operator= (const stack_symbol_type& that);
#endif
    };

    /// A stack with random access from its top.
    template <typename T, typename S = std::vector<T> >
    class stack
    {
    public:
      // Hide our reversed order.
      typedef typename S::iterator iterator;
      typedef typename S::const_iterator const_iterator;
      typedef typename S::size_type size_type;
      typedef typename std::ptrdiff_t index_type;

      stack (size_type n = 200) YY_NOEXCEPT
        : seq_ (n)
      {}

#if 201103L <= YY_CPLUSPLUS
      /// Non copyable.
      stack (const stack&) = delete;
      /// Non copyable.
      stack& operator= (const stack&) = delete;
#endif

      /// Random access.
      ///
      /// Index 0 returns the topmost element.
      const T&
      operator[] (index_type i) const
      {
        return seq_[size_type (size () - 1 - i)];
      }

      /// Random access.
      ///
      /// Index 0 returns the topmost element.
      T&
      operator[] (index_type i)
      {
        return seq_[size_type (size () - 1 - i)];
      }

      /// Steal the contents of \a t.
      ///
      /// Close to move-semantics.
      void
      push (YY_MOVE_REF (T) t)
      {
        seq_.push_back (T ());
        operator[] (0).move (t);
      }

      /// Pop elements from the stack.
      void
      pop (std::ptrdiff_t n = 1) YY_NOEXCEPT
      {
        for (; 0 < n; --n)
          seq_.pop_back ();
      }

      /// Pop all elements from the stack.
      void
      clear () YY_NOEXCEPT
      {
        seq_.clear ();
      }

      /// Number of elements on the stack.
      index_type
      size () const YY_NOEXCEPT
      {
        return index_type (seq_.size ());
      }

      /// Iterator on top of the stack (going downwards).
      const_iterator
      begin () const YY_NOEXCEPT
      {
        return seq_.begin ();
      }

      /// Bottom of the stack.
      const_iterator
      end () const YY_NOEXCEPT
      {
        return seq_.end ();
      }

      /// Present a slice of the top of a stack.
      class slice
      {
      public:
        slice (const stack& stack, index_type range) YY_NOEXCEPT
          : stack_ (stack)
          , range_ (range)
        {}

        const T&
        operator[] (index_type i) const
        {
          return stack_[range_ - i];
        }

      private:
        const stack& stack_;
        index_type range_;
      };

    private:
#if YY_CPLUSPLUS < 201103L
      /// Non copyable.
      stack (const stack&);
      /// Non copyable.
      stack& operator= (const stack&);
#endif
      /// The wrapped container.
      S seq_;
    };


    /// Stack type.
    typedef stack<stack_symbol_type> stack_type;

    /// The stack.
    stack_type yystack_;

    /// Push a new state on the stack.
    /// \param m    a debug message to display
    ///             if null, no trace is output.
    /// \param sym  the symbol
    /// \warning the contents of \a s.value is stolen.
    void yypush_ (const char* m, YY_MOVE_REF (stack_symbol_type) sym);

    /// Push a new look ahead token on the state on the stack.
    /// \param m    a debug message to display
    ///             if null, no trace is output.
    /// \param s    the state
    /// \param sym  the symbol (for its value and location).
    /// \warning the contents of \a sym.value is stolen.
    void yypush_ (const char* m, state_type s, YY_MOVE_REF (symbol_type) sym);

    /// Pop \a n symbols from the stack.
    void yypop_ (int n = 1) YY_NOEXCEPT;

    /// Constants.
    enum
    {
      yylast_ = 840,     ///< Last index in yytable_.
      yynnts_ = 67,  ///< Number of nonterminal symbols.
      yyfinal_ = 2 ///< Termination state number.
    };


    // User arguments.
    State& st;

  };

  inline
  Parser::symbol_kind_type
  Parser::yytranslate_ (int t) YY_NOEXCEPT
  {
    // YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to
    // TOKEN-NUM as returned by yylex.
    static
    const signed char
    translate_table[] =
    {
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83
    };
    // Last valid token kind.
    const int code_max = 338;

    if (t <= 0)
      return symbol_kind::S_YYEOF;
    else if (t <= code_max)
      return static_cast <symbol_kind_type> (translate_table[t]);
    else
      return symbol_kind::S_YYUNDEF;
  }

  // basic_symbol.
  template <typename Base>
  Parser::basic_symbol<Base>::basic_symbol (const basic_symbol& that)
    : Base (that)
    , value ()
    , location (that.location)
  {
    switch (this->kind ())
    {
      case symbol_kind::S_assign_op: // assign_op
        value.copy< AssignOp > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_named: // named
        value.copy< DeclHead > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_param: // param
        value.copy< ParamInfo > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_dim: // dim
        value.copy< ParseContext::Dim > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_init_node: // init_node
        value.copy< ParseContext::InitNode > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_init_opt: // init_opt
        value.copy< ParseContext::Initializer > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_decl_specs: // decl_specs
      case symbol_kind::S_prefix_list: // prefix_list
      case symbol_kind::S_type_end: // type_end
      case symbol_kind::S_struct_spec: // struct_spec
      case symbol_kind::S_enum_spec: // enum_spec
        value.copy< ParseContext::TypeWords > (YY_MOVE (that.value));
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
        value.copy< Token > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_type_name: // type_name
      case symbol_kind::S_115_5: // @5
        value.copy< TypeSpec > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_decl_pushed: // decl_pushed
      case symbol_kind::S_prefix_item: // prefix_item
      case symbol_kind::S_94_2: // @2
      case symbol_kind::S_stars: // stars
      case symbol_kind::S_stars1: // stars1
        value.copy< int > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_compound: // compound
        value.copy< std::unique_ptr<CompoundStmt> > (YY_MOVE (that.value));
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
        value.copy< std::unique_ptr<Expr> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_statement: // statement
        value.copy< std::unique_ptr<Stmt> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_var_declarator: // var_declarator
        value.copy< std::unique_ptr<VarDecl> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_params: // params
      case symbol_kind::S_param_list: // param_list
        value.copy< std::vector<ParamInfo> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_dims: // dims
        value.copy< std::vector<ParseContext::Dim> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_init_list: // init_list
      case symbol_kind::S_init_items: // init_items
        value.copy< std::vector<ParseContext::InitNode> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_args: // args
        value.copy< std::vector<std::unique_ptr<Expr>> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_block_items: // block_items
      case symbol_kind::S_block_item: // block_item
      case symbol_kind::S_local_decl: // local_decl
        value.copy< std::vector<std::unique_ptr<Stmt>> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_var_declarators: // var_declarators
        value.copy< std::vector<std::unique_ptr<VarDecl>> > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

  }




  template <typename Base>
  Parser::symbol_kind_type
  Parser::basic_symbol<Base>::type_get () const YY_NOEXCEPT
  {
    return this->kind ();
  }


  template <typename Base>
  bool
  Parser::basic_symbol<Base>::empty () const YY_NOEXCEPT
  {
    return this->kind () == symbol_kind::S_YYEMPTY;
  }

  template <typename Base>
  void
  Parser::basic_symbol<Base>::move (basic_symbol& s)
  {
    super_type::move (s);
    switch (this->kind ())
    {
      case symbol_kind::S_assign_op: // assign_op
        value.move< AssignOp > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_named: // named
        value.move< DeclHead > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_param: // param
        value.move< ParamInfo > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_dim: // dim
        value.move< ParseContext::Dim > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_init_node: // init_node
        value.move< ParseContext::InitNode > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_init_opt: // init_opt
        value.move< ParseContext::Initializer > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_decl_specs: // decl_specs
      case symbol_kind::S_prefix_list: // prefix_list
      case symbol_kind::S_type_end: // type_end
      case symbol_kind::S_struct_spec: // struct_spec
      case symbol_kind::S_enum_spec: // enum_spec
        value.move< ParseContext::TypeWords > (YY_MOVE (s.value));
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
        value.move< Token > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_type_name: // type_name
      case symbol_kind::S_115_5: // @5
        value.move< TypeSpec > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_decl_pushed: // decl_pushed
      case symbol_kind::S_prefix_item: // prefix_item
      case symbol_kind::S_94_2: // @2
      case symbol_kind::S_stars: // stars
      case symbol_kind::S_stars1: // stars1
        value.move< int > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_compound: // compound
        value.move< std::unique_ptr<CompoundStmt> > (YY_MOVE (s.value));
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
        value.move< std::unique_ptr<Expr> > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_statement: // statement
        value.move< std::unique_ptr<Stmt> > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_var_declarator: // var_declarator
        value.move< std::unique_ptr<VarDecl> > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_params: // params
      case symbol_kind::S_param_list: // param_list
        value.move< std::vector<ParamInfo> > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_dims: // dims
        value.move< std::vector<ParseContext::Dim> > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_init_list: // init_list
      case symbol_kind::S_init_items: // init_items
        value.move< std::vector<ParseContext::InitNode> > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_args: // args
        value.move< std::vector<std::unique_ptr<Expr>> > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_block_items: // block_items
      case symbol_kind::S_block_item: // block_item
      case symbol_kind::S_local_decl: // local_decl
        value.move< std::vector<std::unique_ptr<Stmt>> > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_var_declarators: // var_declarators
        value.move< std::vector<std::unique_ptr<VarDecl>> > (YY_MOVE (s.value));
        break;

      default:
        break;
    }

    location = YY_MOVE (s.location);
  }

  // by_kind.
  inline
  Parser::by_kind::by_kind () YY_NOEXCEPT
    : kind_ (symbol_kind::S_YYEMPTY)
  {}

#if 201103L <= YY_CPLUSPLUS
  inline
  Parser::by_kind::by_kind (by_kind&& that) YY_NOEXCEPT
    : kind_ (that.kind_)
  {
    that.clear ();
  }
#endif

  inline
  Parser::by_kind::by_kind (const by_kind& that) YY_NOEXCEPT
    : kind_ (that.kind_)
  {}

  inline
  Parser::by_kind::by_kind (token_kind_type t) YY_NOEXCEPT
    : kind_ (yytranslate_ (t))
  {}



  inline
  void
  Parser::by_kind::clear () YY_NOEXCEPT
  {
    kind_ = symbol_kind::S_YYEMPTY;
  }

  inline
  void
  Parser::by_kind::move (by_kind& that)
  {
    kind_ = that.kind_;
    that.clear ();
  }

  inline
  Parser::symbol_kind_type
  Parser::by_kind::kind () const YY_NOEXCEPT
  {
    return kind_;
  }


  inline
  Parser::symbol_kind_type
  Parser::by_kind::type_get () const YY_NOEXCEPT
  {
    return this->kind ();
  }


#line 33 "src/parser/parser_bison.y"
} } // minic::bisonp
#line 3931 "src/parser/parser_bison.hpp"




#endif // !YY_YY_SRC_PARSER_PARSER_BISON_HPP_INCLUDED
