/* Pointers through functions: a function returning a pointer, a boxed
   parameter whose address is taken, in-place reversal with two cursors,
   pointer-driven loops and compound assignment through a dereference.
     maxp returns the FIRST maximum (strict >): v[1] = 9, offset 1
     inc makes it 10; bump(10) prints 15
     rev(v, v+5) turns {4,10,2,9,1,7} into {7,1,9,2,10,4}
     sum = 33; g: 4, *=3 -> 12, ++ -> 13; *m is now v[1] = 1 */
int *maxp(int *a, int n) {
    int *best = a;
    int i;
    for (i = 1; i < n; i++)
        if (a[i] > *best) best = &a[i];
    return best;
}
void inc(int *p) { (*p)++; }
void bump(int n) {
    int *q = &n;
    *q += 5;
    printf("%d\n", n);
}
void rev(int *lo, int *hi) {
    while (lo < hi) {
        int t = *lo;
        *lo = *hi;
        *hi = t;
        lo++;
        hi--;
    }
}
int main() {
    int v[6] = {4, 9, 2, 9, 1, 7};
    int *m = maxp(v, 6);
    printf("%d %d\n", *m, m - v);
    inc(m);
    printf("%d\n", v[1]);
    bump(10);
    rev(v, v + 5);
    int i;
    for (i = 0; i < 6; i++) printf("%d ", v[i]);
    printf("\n");
    int *e = v + 6;
    int *p;
    int cnt = 0;
    for (p = v; p != e; p++) cnt += *p;
    printf("%d\n", cnt);
    int g = 0;
    int *gp = &g;
    *gp = 4;
    *gp *= 3;
    g++;
    printf("%d\n", g);
    return *m;
}
