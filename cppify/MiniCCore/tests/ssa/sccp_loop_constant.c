// x is 7 on every path that can run, but two definitions of x reach the
// loop (x = 7 and x = x + 1), so P2 cannot propagate it. SCCP assumes the
// branch is not taken until proven otherwise, finds x == 7 keeps it that way,
// and folds the test, the dead update and every use of x.
int main() {
    int x = 7, i, s = 0;
    for (i = 0; i < 10; i = i + 1) {
        if (x != 7) {
            x = x + 1;
        }
        s = s + x;
    }
    return s;
}
