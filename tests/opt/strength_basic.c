/* P7 strength reduction, isolated: i is a genuine loop-varying induction
   variable (never a compile-time constant across iterations, so P1/P2
   cannot fold i*8 away) -- only strength reduction has a reason to touch
   this multiply. */
int main() {
    int i, s;
    i = 0;
    s = 0;
    while (i < 5) {
        s = s + i * 8;
        i = i + 1;
    }
    return s;
}
