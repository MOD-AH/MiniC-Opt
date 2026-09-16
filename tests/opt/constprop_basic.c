/* P2 micro-test 1 — matches the report's own Table 13 example exactly:
   x = 3; y = x + 2 -> (after constprop, then fold) y = 5. */
int main() {
    int x;
    int y;
    x = 3;
    y = x + 2;
    return y;
}
