/* The same x*y is computed twice per iteration; CSE should reuse it and
   LICM may hoist it. 6 iterations * (15 + 15) = 180 */
int main() {
    int i; int s; int x; int y;
    x = 3; y = 5; s = 0;
    for (i = 0; i < 6; i = i + 1) {
        s = s + x * y;
        s = s + x * y;
    }
    return s;
}
