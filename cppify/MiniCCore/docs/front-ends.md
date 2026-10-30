# Front ends: hand-written and generated (roadmap Phase 4)

Cppify has two lexers and two parsers. The driver chooses with `--lexer=` and `--parser=`.

| Stage | Hand-written | Generated |
|---|---|---|
| Lexer | `src/lexer/lexer.cpp`, peek/advance code | `src/lexer/lexer_re2c.re` → `lexer_re2c.cpp`, a direct-coded DFA from re2c 4.6. **Default.** |
| Parser | `src/parser/parser.cpp`, recursive descent with a Pratt-style expression ladder. **Default.** | `src/parser/parser_bison.y` → `parser_bison.{cpp,hpp}`, IELR(1) tables from Bison 3.8 |

## Why the generated files are committed

re2c and Bison generate ordinary C++. Committing that output means a build needs only `c++`; neither tool has to be installed by the CI runner, `build-app.sh` or anyone cloning the repo. (macOS's built-in Bison is 2.3, which cannot read this grammar.) The specs stay the source of truth: when the tools are installed, `run_all` regenerates both outputs and fails if the committed copy differs.

Regenerate after editing a spec, from `MiniCCore/`:

```
re2c -W --no-generation-date -o src/lexer/lexer_re2c.cpp src/lexer/lexer_re2c.re
bison -o src/parser/parser_bison.cpp --header=src/parser/parser_bison.hpp src/parser/parser_bison.y
```

## What is shared, and why

Two parts of the front end are meaning rather than syntax. Each is written once and called by both implementations:

- **`Preprocessor`.** The macro table, the `#if` stack and the `#if` condition language.
- **`ParseContext`.** The declaration semantics:
  - struct layout,
  - the typedef and enum tables,
  - constant folding of array sizes and case labels,
  - brace-initializer flattening,
  - array-length inference.

So comparing the two lexers compares only scanning, and comparing the two parsers compares only syntax.

## How the parsers handle typedef names

The generated parser has to know which identifiers name types. C is not context-free: `T * x;` declares `x` or multiplies, depending on what `T` is.

- **The "lexer hack".** The token feed returns `TYPEDEF_NAME` for an identifier the actions have recorded as a typedef.
- **After a type word it does not.** So `unsigned T` keeps `T` an ordinary name, as the hand-written parser does.
- **IELR(1), not LALR(1).** It avoids the conflicts LALR's state merging would cause, such as `x :` as a label versus `a ? x : y`.
- **No conflicts left.** The grammar builds with `%expect 0`; the dangling `else` is resolved by precedence.

## Equivalence: what was measured

| Check | Result |
|---|---|
| Lexers: `--dump-tokens` on every test file and benchmark program (tokens, positions, diagnostics) | identical on all 180 (175 tests + 5 benchmarks) |
| Lexers: `scripts/lexfuzz.py`, malformed fragments (unterminated literals, stray `\`, `#` mid-line, odd number suffixes, CR, non-ASCII) | 10,000 inputs, 0 differ |
| Parsers: `--dump-ast` and `--check` on every file without a syntax error | identical |
| Parsers: `--dump-ast` on 600 random programs from `scripts/fuzz.py`, under each lexer | 1,200 / 1,200 identical |
| Parsers: syntax-error and lexical-error tests | first diagnostic at the same line and column in all of them; later diagnostics (wording, recovery) differ in 5 |
| End to end: clang oracle and fuzzer run the hand-written-lexer + Bison-parser combination | 0 mismatches |

The comparison found bugs in both front ends. All are now fixed, and each has a regression test where one applies.

**Hand-written front end (all three were already there):**

- **A trailing `\` at the end of the file overran the source.** Inside a character or string literal, a final `\` made the lexer read one byte past the end of the source, so the token's lexeme ended in a NUL.
- **An indented directive in a false `#if` region was lost.** In such a region an indented `#endif` was skipped, so the region never closed (`tests/programs/preproc_indented.c`).
- **`for (int i = 0; ; i++)` was rejected.** The parser took a different path when the condition was empty, then read `i++` as the condition. clang runs it (`tests/programs/for_decl_empty_cond.c`).

**re2c spec (my mistakes while writing it):**

- **Signed `char` as the input type.** Bytes 0x80–0xFF compared negative and fell out of character classes such as `[^\x00]`.
- **Lost state during error recovery.** Bison's recovery unwinds without running actions, so a failed declaration left its type on a stack. It is now truncated by index.

## Speed

`--trace-json` stage timings on a generated file of 16,002 lines (2,000 functions), median of 7 runs:

| Stage | Hand-written | Generated |
|---|---:|---:|
| Lex | 6.0 ms | **4.2 ms** (re2c, about 30% faster) |
| Parse | **7.0 ms** | 29 ms (Bison, about 4× slower) |

The Bison parser is likely slower because every symbol on its stack carries a full token, lexeme string included, through Bison's variant value type. It has not been profiled. In absolute terms both parse a 16,000-line file in well under a tenth of a second.

## The decision (roadmap 4.3)

The roadmap's rule: a generated front end becomes the default if and only if it is equivalent.

- **re2c lexer: default.** It is equivalent on every check above, and faster.
- **Bison parser: not the default.** Its ASTs are identical, but its error recovery produces different follow-on diagnostics in 5 of the syntax-error tests. (In one, `syn04`, it gives 2 diagnostics where the hand-written parser cascades to 4.) It is also slower. It stays available as `--parser=bison`, is tested on every run, and runs as its own mode in the clang oracle and the fuzzer.

## Known divergences on invalid input

These were found by reading the two parsers rather than by a failing test. None affects a valid program.

- **`sizeof` and casts with `static`/`extern`.** In `(static int)x` or `sizeof (static int)`, the hand-written parser does not treat `static`/`extern` as starting a type name; the Bison grammar accepts them.
- **A trailing operator after a string initializer.** `char s[] = "ab" + 1` is a syntax error in the hand-written parser and a type error in the Bison one.
- **Error wording.** Bison words its messages from the expected-token set. It says "unexpected 'x'" where no single phrase fits.
