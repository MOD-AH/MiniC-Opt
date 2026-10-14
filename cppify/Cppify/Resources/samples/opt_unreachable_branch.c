/* flag is a known constant, so constant propagation turns the branch
   condition into a constant and the then-arm becomes unreachable. */
int main() {
    int flag; int r;
    flag = 0;
    r = 0;
    if (flag) { r = 1; } else { r = 2; }
    return r;
}
