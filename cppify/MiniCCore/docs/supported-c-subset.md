# Cppify C subset, v1

What Cppify compiles, how it behaves, and where it differs from a real C compiler. Every behaviour row names the test that pins it.

The reference is clang on macOS arm64 (LP64: `char` is signed, `long` is 64 bits). `scripts/clang_oracle.py` runs every positive test under both compilers and requires identical stdout and exit status from Cppify unoptimized, with the classic preset and with the optimized preset. Last run: 120 programs, 0 mismatches, 0 excluded. `scripts/fuzz.py` does the same for random programs. In its last large run, 740 programs under 7 configurations gave 0 mismatches.

## Types

| Type | Width | Behaviour | Test |
|---|---|---|---|
| `char` / `signed char` / `unsigned char` | 8 | wraps to range on every store and conversion | `int_widths.c`, `char_arith.c` |
| `short` / `unsigned short` | 16 | as above | `int_widths.c` |
| `int` / `unsigned` | 32 | arithmetic wraps mod 2^32 | `int_widths.c` |
| `long`, `long long` (+ `unsigned`) | 64 | unsigned compare/divide/shift on `unsigned long` | `int_widths.c` |
| `float`, `double` | 4, 8 | both are a C `double` at run time; `sizeof` and struct layout use the C sizes | `float_arith.c`, `c_gaps_closed.c` |
| pointers, arrays (incl. 2-D), `struct`, `typedef`, `enum` | — | pointer = (array, offset); a struct is a flat run of slots | `pointers_*.c`, `sample_matrix_2d.c`, `structs_misc.c` |

The integer promotions and usual arithmetic conversions (C11 6.3.1) are applied: `-1 < 0u` is 0 and `unsigned char` 200 + 100 is 300 (`int_widths.c`). Literals take their C type from value and suffix (`u`, `l`, `ll`).

## Statements, declarations, preprocessor

Supported:

- **Control flow:** `if`/`else`, `while`, `do`, `for`, `switch`, `break`, `continue`, `goto` and labels, `return`. Tests: `cf_*.c`, `do_while.c`.
- **Operators:** `?:`, the comma operator, `sizeof`, every compound assignment, `++`/`--`. Tests: `compound_assign.c`, `incr_decr.c`.
- **Variables:** globals (`globals.c`), and `static` locals, which are initialized once and keep their value (`static_locals.c`).
- **Preprocessor:**
  - `#define NAME text` for object-like macros.
  - `#undef`.
  - `#ifdef`, `#ifndef`, `#if`/`#elif` on an integer or `defined(NAME)`, `#else`, `#endif`. Test: `preproc_cond.c`.
  - `#include` lines are accepted and ignored. The library below is always in scope.

## Library

Library calls run inside the interpreter; nothing is linked.

| Group | Functions |
|---|---|
| I/O | `printf`, `scanf`, `sscanf`, `sprintf`, `snprintf`, `puts`, `putchar`, `getchar`, `gets`, `fgets`, `fputs`, `fputc`, `putc`, `fflush` |
| Strings and memory | `strlen`, `strcpy`, `strncpy`, `strcat`, `strncat`, `strcmp`, `strncmp`, `strchr`, `strrchr`, `strstr`, `strspn`, `strcspn`, `strdup`, `memset`, `memcpy`, `memmove`, `memcmp` |
| Conversion | `atoi`, `atol`, `atof` |
| Math | `abs`, `labs`, `fabs`, `sqrt`, `cbrt`, `pow`, the trig and hyperbolic functions, `exp`, the `log` family, `floor`, `ceil`, `round`, `trunc`, `fmod`, `fmin`, `fmax`, `hypot` |
| Memory | `malloc`, `calloc`, `realloc`, `free` |
| Process | `exit`, `assert`, `rand`, `srand`, `time`, `clock` |
| Characters | `<ctype.h>` classification and case conversion |

Format strings are type-checked against their arguments. Tests: `stdlib_strings_math.c`, `stdin_scanf_fgets.c`, `sample_strings_demo.c`.

## Rejected with an error (never miscompiled)

| Feature | Diagnostic |
|---|---|
| `union` | expected a declaration |
| function pointers | expected an identifier |
| defining a variadic function (`...`) | expected a type |
| function-like macros | function-like macro is not supported |
| an `#if` condition other than an integer or `defined(X)` | unsupported #if condition (`lex06_unsupported_if_condition.c`) |
| `#if` without `#endif`, or a stray `#endif` | unterminated #if (`lex07_unterminated_if.c`) |
| file I/O (`fopen`, `FILE`) | call to undeclared function |
| `\x` escapes in character literals | unterminated character literal |

After a syntax error, compilation stops before semantic analysis. Each fault gives one diagnostic, not a cascade (`syn_rec*.c`).

## Where Cppify knowingly differs from clang

**Undefined behaviour.** The C standard leaves these undefined; Cppify picks one fixed answer:

- **Signed overflow** wraps (two's complement), which is also what clang -O0 produces.
- **Division by zero, a shift count outside `[0, width)`, an out-of-bounds or null access** stop the run with a runtime error instead of continuing.
- **An uninitialized local** reads as 0.

These are tested deterministically by `tests/boundary`.

**Defined behaviour that differs.** These are known gaps (roadmap items, not features):

| Case | Cppify | clang |
|---|---|---|
| `sizeof(1.5f)` | 8 (the lexer drops the `f`, so the constant is a double) | 4 |
| float arithmetic | computed in double precision (`0.1f` prints as `0.1000000000` with `%.10f`) | float precision (`0.1000000015`) |
| `\x` escapes | rejected in character literals; in strings `\x41` reads as `x41` | hexadecimal escapes |
| memory | counted in slots (one per scalar); `sizeof` still reports bytes | bytes |

Closed in roadmap 5.5, pinned by `tests/programs/c_gaps_closed.c`:

- `sizeof(double)` is 8 (it was 4), and struct layout follows.
- `sizeof` has type `size_t` (it was `int`), so `-1 < sizeof(int)` is 0.
- Bytes above 127 in strings are negative `char` values (they were 128–255).

No native code is produced. Programs run on the IR interpreter, twice: unoptimized and optimized.
