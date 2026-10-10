// Unit tests for the hand-written DFA scanner: token classification,
// maximal-munch operator splitting, literal forms, comment handling, and
// the line/column bookkeeping every later diagnostic depends on.
#include "test_helpers.h"

using namespace minic;

namespace {

Tok firstTok(const std::string& src) {
    std::vector<Token> ts = th::lexBody(src);
    return ts.empty() ? Tok::END_OF_FILE : ts[0].type;
}

std::string firstLexeme(const std::string& src) {
    std::vector<Token> ts = th::lexBody(src);
    return ts.empty() ? std::string() : ts[0].lexeme;
}

}  // namespace

// ---- identifiers and keywords ----------------------------------------

TEST(lexer_identifier_is_ident) { CHECK_EQ(firstTok("abc"), Tok::IDENT); }

TEST(lexer_identifier_keeps_lexeme) { CHECK_EQ(firstLexeme("counter"), std::string("counter")); }

TEST(lexer_identifier_may_start_with_underscore) { CHECK_EQ(firstTok("_x"), Tok::IDENT); }

TEST(lexer_identifier_may_contain_digits) { CHECK_EQ(firstLexeme("a1b2"), std::string("a1b2")); }

TEST(lexer_keyword_int) { CHECK_EQ(firstTok("int"), Tok::KW_INT); }
TEST(lexer_keyword_char) { CHECK_EQ(firstTok("char"), Tok::KW_CHAR); }
TEST(lexer_keyword_float) { CHECK_EQ(firstTok("float"), Tok::KW_FLOAT); }
TEST(lexer_keyword_void) { CHECK_EQ(firstTok("void"), Tok::KW_VOID); }
TEST(lexer_keyword_if) { CHECK_EQ(firstTok("if"), Tok::KW_IF); }
TEST(lexer_keyword_else) { CHECK_EQ(firstTok("else"), Tok::KW_ELSE); }
TEST(lexer_keyword_while) { CHECK_EQ(firstTok("while"), Tok::KW_WHILE); }
TEST(lexer_keyword_for) { CHECK_EQ(firstTok("for"), Tok::KW_FOR); }
TEST(lexer_keyword_do) { CHECK_EQ(firstTok("do"), Tok::KW_DO); }
TEST(lexer_keyword_return) { CHECK_EQ(firstTok("return"), Tok::KW_RETURN); }
TEST(lexer_keyword_break) { CHECK_EQ(firstTok("break"), Tok::KW_BREAK); }
TEST(lexer_keyword_continue) { CHECK_EQ(firstTok("continue"), Tok::KW_CONTINUE); }

TEST(lexer_keyword_prefix_is_still_an_identifier) {
    // "integer" starts with "int" but must not lex as the keyword.
    CHECK_EQ(firstTok("integer"), Tok::IDENT);
}

TEST(lexer_keyword_is_case_sensitive) { CHECK_EQ(firstTok("INT"), Tok::IDENT); }

// ---- literals ---------------------------------------------------------

TEST(lexer_integer_literal) { CHECK_EQ(firstTok("123"), Tok::INT_LIT); }

TEST(lexer_integer_literal_lexeme) { CHECK_EQ(firstLexeme("4096"), std::string("4096")); }

TEST(lexer_zero_is_an_integer_literal) { CHECK_EQ(firstTok("0"), Tok::INT_LIT); }

TEST(lexer_float_literal) { CHECK_EQ(firstTok("1.5"), Tok::FLOAT_LIT); }

TEST(lexer_float_literal_lexeme) { CHECK_EQ(firstLexeme("3.14159"), std::string("3.14159")); }

TEST(lexer_char_literal) { CHECK_EQ(firstTok("'a'"), Tok::CHAR_LIT); }

TEST(lexer_char_literal_escape) { CHECK_EQ(firstTok("'\\n'"), Tok::CHAR_LIT); }

TEST(lexer_integer_then_dot_is_not_swallowed_by_float) {
    // "1.5" is one FLOAT_LIT, so the stream length distinguishes it from
    // an integer followed by something else.
    CHECK_EQ(th::lexBody("1.5").size(), size_t(1));
}

// ---- operators, including maximal munch -------------------------------

TEST(lexer_plus) { CHECK_EQ(firstTok("+"), Tok::OP_PLUS); }
TEST(lexer_minus) { CHECK_EQ(firstTok("-"), Tok::OP_MINUS); }
TEST(lexer_star) { CHECK_EQ(firstTok("*"), Tok::OP_STAR); }
TEST(lexer_slash) { CHECK_EQ(firstTok("/"), Tok::OP_SLASH); }
TEST(lexer_percent) { CHECK_EQ(firstTok("%"), Tok::OP_PERCENT); }
TEST(lexer_assign) { CHECK_EQ(firstTok("="), Tok::OP_ASSIGN); }

TEST(lexer_eq_beats_assign) { CHECK_EQ(firstTok("=="), Tok::OP_EQ); }
TEST(lexer_ne) { CHECK_EQ(firstTok("!="), Tok::OP_NE); }
TEST(lexer_le_beats_lt) { CHECK_EQ(firstTok("<="), Tok::OP_LE); }
TEST(lexer_ge_beats_gt) { CHECK_EQ(firstTok(">="), Tok::OP_GE); }
TEST(lexer_lt) { CHECK_EQ(firstTok("<"), Tok::OP_LT); }
TEST(lexer_gt) { CHECK_EQ(firstTok(">"), Tok::OP_GT); }
TEST(lexer_not) { CHECK_EQ(firstTok("!"), Tok::OP_NOT); }
TEST(lexer_and) { CHECK_EQ(firstTok("&&"), Tok::OP_AND); }
TEST(lexer_or) { CHECK_EQ(firstTok("||"), Tok::OP_OR); }
TEST(lexer_inc_beats_plus) { CHECK_EQ(firstTok("++"), Tok::OP_INC); }
TEST(lexer_dec_beats_minus) { CHECK_EQ(firstTok("--"), Tok::OP_DEC); }
TEST(lexer_plus_assign) { CHECK_EQ(firstTok("+="), Tok::OP_PLUS_ASSIGN); }
TEST(lexer_minus_assign) { CHECK_EQ(firstTok("-="), Tok::OP_MINUS_ASSIGN); }
TEST(lexer_star_assign) { CHECK_EQ(firstTok("*="), Tok::OP_STAR_ASSIGN); }
TEST(lexer_slash_assign) { CHECK_EQ(firstTok("/="), Tok::OP_SLASH_ASSIGN); }

TEST(lexer_plus_plus_is_one_token_not_two) {
    CHECK_EQ(th::lexBody("++").size(), size_t(1));
}

TEST(lexer_plus_space_plus_is_two_tokens) {
    CHECK_EQ(th::lexBody("+ +").size(), size_t(2));
}

// ---- punctuation ------------------------------------------------------

TEST(lexer_lparen) { CHECK_EQ(firstTok("("), Tok::LPAREN); }
TEST(lexer_rparen) { CHECK_EQ(firstTok(")"), Tok::RPAREN); }
TEST(lexer_lbrace) { CHECK_EQ(firstTok("{"), Tok::LBRACE); }
TEST(lexer_rbrace) { CHECK_EQ(firstTok("}"), Tok::RBRACE); }
TEST(lexer_lbracket) { CHECK_EQ(firstTok("["), Tok::LBRACKET); }
TEST(lexer_rbracket) { CHECK_EQ(firstTok("]"), Tok::RBRACKET); }
TEST(lexer_comma) { CHECK_EQ(firstTok(","), Tok::COMMA); }
TEST(lexer_semi) { CHECK_EQ(firstTok(";"), Tok::SEMI); }

// ---- whitespace, comments, EOF ----------------------------------------

TEST(lexer_empty_source_is_just_eof) {
    std::vector<Token> ts = th::lexOk("");
    REQUIRE_EQ(ts.size(), size_t(1));
    CHECK_EQ(ts[0].type, Tok::END_OF_FILE);
}

TEST(lexer_whitespace_only_is_just_eof) {
    CHECK_EQ(th::lexBody("  \t\n  ").size(), size_t(0));
}

TEST(lexer_line_comment_is_skipped) {
    CHECK_EQ(th::lexBody("// all of this\nx").size(), size_t(1));
}

TEST(lexer_block_comment_is_skipped) {
    CHECK_EQ(th::lexBody("/* gone */ x").size(), size_t(1));
}

TEST(lexer_block_comment_spanning_lines_is_skipped) {
    CHECK_EQ(th::lexBody("/* a\n b\n c */ x").size(), size_t(1));
}

TEST(lexer_always_terminates_with_eof) {
    std::vector<Token> ts = th::lexOk("int x;");
    REQUIRE(!ts.empty());
    CHECK_EQ(ts.back().type, Tok::END_OF_FILE);
}

// ---- line and column bookkeeping --------------------------------------

TEST(lexer_first_token_is_line_one_col_one) {
    std::vector<Token> ts = th::lexBody("abc");
    REQUIRE(!ts.empty());
    CHECK_EQ(ts[0].line, 1);
    CHECK_EQ(ts[0].col, 1);
}

TEST(lexer_column_advances_within_a_line) {
    std::vector<Token> ts = th::lexBody("a bb");
    REQUIRE_EQ(ts.size(), size_t(2));
    CHECK_EQ(ts[1].col, 3);
}

TEST(lexer_newline_resets_column_and_bumps_line) {
    std::vector<Token> ts = th::lexBody("a\nb");
    REQUIRE_EQ(ts.size(), size_t(2));
    CHECK_EQ(ts[1].line, 2);
    CHECK_EQ(ts[1].col, 1);
}

TEST(lexer_line_counts_across_a_block_comment) {
    std::vector<Token> ts = th::lexBody("/*\n\n*/ x");
    REQUIRE_EQ(ts.size(), size_t(1));
    CHECK_EQ(ts[0].line, 3);
}

// ---- diagnostics ------------------------------------------------------

TEST(lexer_clean_source_reports_no_errors) {
    CHECK(th::lexErrors("int main() { return 0; }").empty());
}

TEST(lexer_rejects_unexpected_character) {
    CHECK(th::hasError(th::lexErrors("int x = @;"), "unexpected character"));
}

TEST(lexer_rejects_unterminated_block_comment) {
    CHECK(th::hasError(th::lexErrors("/* never closed"), "unterminated block comment"));
}

TEST(lexer_rejects_malformed_number) {
    CHECK(th::hasError(th::lexErrors("int x = 12abc;"), "malformed numeric literal"));
}

TEST(lexer_rejects_unterminated_char_literal) {
    CHECK(th::hasError(th::lexErrors("char c = 'a;"), "unterminated character literal"));
}

TEST(lexer_rejects_bitwise_and) {
    CHECK(th::hasError(th::lexErrors("int x = 1 & 2;"), "bitwise operators"));
}

TEST(lexer_rejects_bitwise_or) {
    CHECK(th::hasError(th::lexErrors("int x = 1 | 2;"), "bitwise operators"));
}

TEST(lexer_does_not_stop_at_the_first_error) {
    // Two bad characters must both be reported; the scanner recovers.
    CHECK(th::lexErrors("int x = @; int y = @;").size() >= size_t(2));
}

TEST(lexer_error_carries_a_position) {
    std::vector<Diagnostic> errs = th::lexErrors("int x = @;");
    REQUIRE(!errs.empty());
    CHECK_EQ(errs[0].line, 1);
    CHECK(errs[0].col > 0);
}

// ---- token naming -----------------------------------------------------

TEST(lexer_tokName_is_non_empty_for_every_token_produced) {
    for (const Token& t : th::lexOk("int f(int a) { return a + 1; }")) {
        CHECK(std::string(tokName(t.type)).size() > 0);
    }
}
