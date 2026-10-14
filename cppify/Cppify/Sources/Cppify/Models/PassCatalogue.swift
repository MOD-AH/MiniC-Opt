import Foundation

/// Standing description of each optimization pass: what it does, what
/// analysis licenses it, and — the part a textbook usually omits — what it
/// deliberately refuses to do.
///
/// The per-edit `reason` strings come from the compiler itself at run time.
/// What lives here is only the material that is true for every program, so
/// the two never contradict each other.
struct Pass: Identifiable, Hashable {
    let id: String            // matches --opt= and the trace's "pass" field
    let number: String        // P1…P9, the numbering used in the report
    let title: String
    let what: String
    let licensedBy: String
    let refuses: String?

    static let all: [Pass] = [
        Pass(id: "fold", number: "P1", title: "Constant Folding",
             what: "Evaluates an operation whose operands are all compile-time constants and replaces it with the result, so the work happens once during compilation instead of every time the program runs.",
             licensedBy: "None — this is a purely syntactic rewrite. It needs no analysis because a constant cannot change.",
             refuses: "The arithmetic deliberately mirrors the interpreter's own applyBinary, including integer division truncating toward zero. If folding used different rules from running, an optimized program could print a different answer from an unoptimized one."),

        Pass(id: "constprop", number: "P2", title: "Constant Propagation",
             what: "Where a variable is known to hold a constant, replaces reads of that variable with the constant itself. This is what exposes further folding.",
             licensedBy: "Reaching definitions. A use may be replaced only when exactly one definition reaches it and that definition assigns a constant.",
             refuses: "Nothing extra. A constant cannot be invalidated by some other variable being reassigned, so unlike copy propagation this pass needs no locality restriction."),

        Pass(id: "copyprop", number: "P3", title: "Copy Propagation",
             what: "After an assignment x = y, replaces later reads of x with y, so the copy itself becomes dead and can be removed.",
             licensedBy: "Reaching definitions, plus one condition reaching definitions does not give: the SOURCE must not have been reassigned between the copy and the use.",
             refuses: "Facts that cross a basic-block boundary. Reaching definitions proves x's own definition is un-killed but says nothing about y being overwritten. gcd.c exposed exactly this: the loop reassigns the source between the copy and its use, and the naive version silently used the new value, returning 0 instead of 21."),

        Pass(id: "cse", number: "P4", title: "Common Subexpression Elimination",
             what: "When the same expression is computed twice and nothing it depends on has changed, reuses the first result instead of recomputing it.",
             licensedBy: "Available expressions, restricted to a single forward walk within one block.",
             refuses: "Cross-block reuse. Availability tells you the operands are unchanged, but not WHICH variable still holds the value — so the pass tracks holders itself and drops any fact whose operand or holder is redefined."),

        Pass(id: "dce", number: "P5", title: "Dead Code Elimination",
             what: "Deletes an instruction whose result is never read again, walking each block backwards from its live-out set so chains of dead assignments collapse in one go.",
             licensedBy: "Live variables, computed across blocks by the iterative solver.",
             refuses: "Four things, even though they write a name. CALL may have an observable side effect such as printing. DIV and MOD can trap on zero. LOAD_INDEX can trap out of bounds. And a store to a GLOBAL is never dead, because liveness here is intra-procedural and cannot see the read another function performs later — deleting one silently emptied the body of a function whose only job was updating a counter."),

        Pass(id: "unreachable", number: "P6", title: "Unreachable Code Elimination",
             what: "Folds a branch whose condition has become constant into an unconditional jump or removes it outright, then deletes any block that is no longer reachable from the function entry.",
             licensedBy: "CFG reachability, plus a branch condition that constant propagation has already turned into a literal.",
             refuses: "Guessing. A block is removed only when it is genuinely unreachable from entry after the edge is severed, recomputed to a local fixed point because severing one edge can strand further blocks."),

        Pass(id: "strength", number: "P7", title: "Strength Reduction",
             what: "Replaces a multiplication by a loop induction variable with an addition, by maintaining a mirror variable that advances by a fixed step each iteration.",
             licensedBy: "A basic induction variable: one written exactly once in the loop, by i = i ± C for a constant C.",
             refuses: "Loops without a usable preheader. The mirror's update is inserted immediately after the induction variable's own update, which is what makes the invariant hold at every point in the loop without needing a before/after case split."),

        Pass(id: "licm", number: "P8", title: "Loop-Invariant Code Motion",
             what: "Moves a computation whose value cannot change across iterations out of the loop and into its preheader, so it runs once instead of every time round.",
             licensedBy: "Dominators, back edges and natural loops.",
             refuses: "Anything that might trap, and anything that does not run on every iteration. The defining block must dominate the loop tail — hoisting a computation that only ran behind an inner `if` would change behaviour for iterations that used to skip it, and hoisting a division out of a loop that runs zero times would introduce a crash that never existed."),

        Pass(id: "peephole", number: "P9", title: "Peephole Optimization",
             what: "Pattern-matches small windows of control transfers: collapses a jump that lands on another jump, and removes a jump to the block that physically follows it.",
             licensedBy: "None — direct pattern matching on the CFG, kept inside the differential-execution safety net rather than run on a separately emitted listing.",
             refuses: "Reordering successors. Redirecting an edge replaces the target in place at its existing index, because an IF_FALSE block's successor order encodes which edge is the branch and which is the fall-through, and branch folding depends on it."),
    ]

    static func named(_ id: String) -> Pass? { all.first { $0.id == id } }
}
