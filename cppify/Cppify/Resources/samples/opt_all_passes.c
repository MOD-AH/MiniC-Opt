/* Deliberately gives every pass something to do: constants to fold and
   propagate, a dead store, an invariant multiply, a redundant repeat of
   it, an induction multiply, and a constant branch.
   per iteration 20 + 20 + 2i; i=0..4 -> 200 + 20 = 220; flag adds 1. */
int main() {
    int i; int s; int a; int b; int t; int dead; int flag;
    a = 4; b = 5; flag = 1; s = 0;
    dead = 99;
    for (i = 0; i < 5; i = i + 1) {
        t = a * b;
        s = s + t;
        s = s + a * b;
        s = s + i * 2;
    }
    if (flag) { s = s + 1; } else { s = s - 1; }
    return s;
}
