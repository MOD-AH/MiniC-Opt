/* Lomuto partition, recursive, mutating through an array parameter.
   sorted 1,2,3,5,7,8,9 packed as decimal digits = 1235789 */
void qsort_rec(int a[], int lo, int hi) {
    int i; int j; int p; int t;
    if (lo >= hi) { return; }
    p = a[hi];
    i = lo - 1;
    for (j = lo; j < hi; j = j + 1) {
        if (a[j] <= p) {
            i = i + 1;
            t = a[i]; a[i] = a[j]; a[j] = t;
        }
    }
    i = i + 1;
    t = a[i]; a[i] = a[hi]; a[hi] = t;
    qsort_rec(a, lo, i - 1);
    qsort_rec(a, i + 1, hi);
}

int main() {
    int a[7];
    int i; int s;
    a[0] = 9; a[1] = 3; a[2] = 7; a[3] = 1; a[4] = 8; a[5] = 2; a[6] = 5;
    qsort_rec(a, 0, 6);
    s = 0;
    for (i = 0; i < 7; i = i + 1) { s = s * 10 + a[i]; }
    return s;
}
