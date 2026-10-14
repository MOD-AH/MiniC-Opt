/* a is fully initialized: 1+2+3+4+5 = 15.
   b supplies only two of four initializers; the remainder must be
   zero-filled, so 7+8+0+0 = 15 as well.  Total 30. */
int main() {
    int a[5] = {1, 2, 3, 4, 5};
    int b[4] = {7, 8};
    int i; int s;
    s = 0;
    for (i = 0; i < 5; i = i + 1) { s = s + a[i]; }
    for (i = 0; i < 4; i = i + 1) { s = s + b[i]; }
    return s;
}
