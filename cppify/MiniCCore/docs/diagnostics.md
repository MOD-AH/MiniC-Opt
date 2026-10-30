# Diagnostic codes (roadmap 5.4)

Every diagnostic `--trace-json` reports carries a stable code, so the app (and anyone reading a log) can group, filter and look up errors without depending on the wording.

- **Assignment.** Codes are assigned in `src/trace/diag_codes.cpp` from the message text, by the first matching rule.
- **Stability.** Rules are only ever appended, never renumbered.
- **Fallback.** A message no rule recognises gets its stage's `-000` code. `run_all` fails if any negative test produces one.

## Primary and secondary

Each diagnostic is also marked primary or secondary.

- **Primary:** every lexical and semantic error, and the first syntax error.
- **Secondary:** later syntax errors, and any syntax error after a lexical one. These may be consequences of an earlier error rather than new problems.

Semantic analysis runs only on a program that parsed cleanly, so its errors are independent.

`unsupported: true` marks C that Cppify deliberately does not implement. In the app it turns the build state into **Unsupported feature** rather than **Compilation failed**.

This list is generated from the rule table:

| Code | Category | Unsupported | Matches a message containing |
|---|---|---|---|
| `CPPIFY-PP-001` | Unsupported #if condition | yes | `#if condition`, `#elif condition` |
| `CPPIFY-PP-002` | Unmatched #endif, #else or #elif |  | `without a matching #if` |
| `CPPIFY-PP-003` | Unterminated #if |  | `unterminated #if` |
| `CPPIFY-PP-004` | Function-like macro | yes | `function-like macro` |
| `CPPIFY-PP-005` | Directive after #else |  | `after #else` |
| `CPPIFY-LEX-001` | Unexpected character |  | `unexpected character` |
| `CPPIFY-LEX-002` | Unterminated comment |  | `unterminated block comment` |
| `CPPIFY-LEX-003` | Malformed number |  | `malformed numeric literal` |
| `CPPIFY-LEX-004` | Unterminated character literal |  | `unterminated character` |
| `CPPIFY-LEX-005` | Unterminated string |  | `unterminated string` |
| `CPPIFY-PARSE-001` | Missing ';' |  | `expected ';'` |
| `CPPIFY-PARSE-002` | Expected an expression |  | `expected an expression` |
| `CPPIFY-PARSE-003` | Expected a type |  | `expected a type` |
| `CPPIFY-PARSE-004` | Expected a declaration |  | `expected a declaration` |
| `CPPIFY-PARSE-005` | Unexpected token |  | `expected ` |
| `CPPIFY-PARSE-006` | Unexpected token |  | `unexpected ` |
| `CPPIFY-PARSE-007` | Not assignable |  | `left-hand side of assignment` |
| `CPPIFY-PARSE-008` | Expression too deeply nested |  | `nests too deeply` |
| `CPPIFY-PARSE-009` | Array without a size |  | `has no size` |
| `CPPIFY-PARSE-010` | Bad array size |  | `array size`, `array dimension` |
| `CPPIFY-PARSE-011` | Not a constant |  | `constant integer expression` |
| `CPPIFY-PARSE-012` | Bad initializer |  | `initializer` |
| `CPPIFY-PARSE-013` | Bad struct definition |  | `struct`, `field` |
| `CPPIFY-PARSE-014` | Call of a non-function |  | `not a function name` |
| `CPPIFY-PARSE-015` | Statement outside a case |  | `before the first 'case'` |
| `CPPIFY-PARSE-016` | Missing parameter name |  | `parameter name` |
| `CPPIFY-PARSE-017` | union (not supported) | yes | `'union'` |
| `CPPIFY-PARSE-018` | Function pointer (not supported) | yes | `function pointers` |
| `CPPIFY-SEMA-001` | Undeclared identifier |  | `undeclared identifier` |
| `CPPIFY-SEMA-002` | Undeclared function |  | `undeclared function` |
| `CPPIFY-SEMA-003` | Wrong number of arguments |  | `expects ` |
| `CPPIFY-SEMA-004` | Type mismatch |  | `type mismatch`, `wrong type`, `does not match`, `incompatible` |
| `CPPIFY-SEMA-005` | Missing return |  | `missing return` |
| `CPPIFY-SEMA-006` | Return value mismatch |  | `'return` |
| `CPPIFY-SEMA-007` | Redeclaration |  | `redeclaration`, `defined twice`, `shadows a built-in` |
| `CPPIFY-SEMA-008` | Division by zero |  | `division by zero` |
| `CPPIFY-SEMA-009` | Wrong operand type |  | `must be integral`, `must be numeric`, `must be a scalar`, `integral expression`, `invalid operand`, `invalid operator`, `cannot cast` |
| `CPPIFY-SEMA-010` | Pointer misuse |  | `pointer`, `subscript` |
| `CPPIFY-SEMA-011` | Struct misuse |  | `struct`, `field`, `'.'`, `'->'` |
| `CPPIFY-SEMA-012` | Bad initializer |  | `initializer`, `too many initializers` |
| `CPPIFY-SEMA-013` | Undefined label |  | `label '` |
| `CPPIFY-SEMA-014` | Bad switch |  | `duplicate case`, `duplicate 'default'`, `switch` |
| `CPPIFY-SEMA-015` | Not assignable |  | `assign to an array`, `increment/decrement`, `operand of '&'` |
| `CPPIFY-SEMA-016` | Void misuse |  | `void` |
| `CPPIFY-SEMA-017` | Library call misuse |  | `address`, `format` |
| `CPPIFY-SEMA-018` | Function pointer (not supported) | yes | `function pointers` |
| `CPPIFY-SEMA-019` | File I/O (not supported) | yes | `undeclared function 'fopen'`, `undeclared function 'fclose'`, `undeclared function 'fread'`, `undeclared function 'fwrite'` |

Fallbacks: `CPPIFY-LEX-000`, `CPPIFY-PARSE-000`, `CPPIFY-SEMA-000`.
