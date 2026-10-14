# Worked optimization example

This example was worked **by hand before implementation began**. It is the
specification the passes are written against, and the demonstration the team
will run live at Review 3.

## Source (MiniC)

```c
int main() {
    int i, n, s, t, a, b;
    a = 4;  b = 5;  n = 100;  s = 0;
    i = 0;
    while (i < n) {
        t = a * b;        /* loop invariant        */
        s = s + t;
        s = s + a * b;    /* common sub-expression */
        i = i + 1 * 8;    /* constant expression   */
    }
    return s;
}
```

## Unoptimized three-address code

```
L0:   a  = 4
      b  = 5
      n  = 100
      s  = 0
      i  = 0
L1:   t1 = i < n            ; loop header
      ifFalse t1 goto L3
L2:   t  = a * b            ; loop invariant
      s  = s + t
      t2 = a * b            ; redundant recomputation
      s  = s + t2
      t3 = 1 * 8            ; constant expression
      i  = i + t3
      goto L1
L3:   ret s

   static instructions in loop body : 7
   static instructions total        : 15
   dynamic instructions executed    : 125   (13 iterations: i = 0, 8, ... 96)
```

## After the optimization pipeline

```
L0:   s  = 0
      i  = 0
      t  = 20               ; P2 propagated a = 4 and b = 5,
                            ;   then P1 folded 4 * 5
L1:   t1 = i < 100          ; P2 propagated n = 100
      ifFalse t1 goto L3
L2:   s  = s + t            ; P8 hoisted a * b out of the loop
      s  = s + t            ; P4 CSE: the second a * b reused t
      i  = i + 8            ; P1 folded 1 * 8
      goto L1
L3:   ret s

   a = 4, b = 5 and n = 100 were deleted by P5 (dead-code elimination)
   once every use of them had been propagated away.

   static instructions in loop body : 4     (was 7,   -43%)
   static instructions total        : 10    (was 15,  -33%)
   dynamic instructions executed    : 84    (was 125, -33%)
```

## Why this example is the one we teach from

**No single pass produces this result.**

1. **P2** constant propagation replaces `a`, `b`, `n` with their constant values
2. → which makes **P1** constant folding able to evaluate `4 * 5` and `1 * 8`
3. → which makes the loop-invariant expression visible to **P8**, hoisting it
4. → which leaves the second `a * b` as a plain redundancy for **P4** to reuse
5. → which leaves `t2`, `t3`, and then `a`, `b`, `n` dead for **P5** to delete

Five passes, each enabling the next. This is exactly why the pass manager runs
the pipeline **to a fixed point** — repeatedly, until a full sweep changes
nothing — instead of running each pass once.
