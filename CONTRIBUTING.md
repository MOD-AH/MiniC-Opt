# Contributing to MiniC-Opt

## Branching

- `main` is protected. All work happens on a feature branch.
- Branch naming: `<member>/<module>-<short-description>`, e.g. `m3/ir-backpatching`.
- Every pull request needs one reviewer approval and a green CI run.

## Commits

Commit small and often, under your own GitHub account — the contributor graph is
part of our Review evidence. Use a scope prefix:

```
lexer: handle escape sequences in char literals
ir:    backpatch short-circuit boolean jumps
opt:   implement P5 dead-code elimination
docs:  add FIRST/FOLLOW table to grammar.ebnf
```

## Frozen interfaces

`include/ir.h`, `include/cfg.h` and `include/pass.h` are **frozen from Week 4**.
Changing them requires approval from the two members whose modules consume them.
This is the mechanism that lets four people write passes in parallel against a
stable IR.

## Adding an optimization pass

1. Implement the `Pass` interface: `name()`, `description()`, `run(CFG&) -> bool`.
2. State the pass in the header comment as **precondition ⇒ rewrite**, naming the
   analysis that supplies the precondition. No analysis, no rewrite.
3. Write at least three micro-tests in `tests/programs/` where *only* your pass
   can fire, and commit the golden IR.
4. Register the pass with the pass manager only once its tests are green.

## Definition of done

A module is done when: it builds warning-free under both g++ and clang++, its
unit tests pass, its behaviour is documented in the README table, and — from
Review 2 onward — the differential-execution gate is still green.
