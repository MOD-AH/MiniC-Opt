/* Recursive merge sort with a scratch array passed alongside.
   sorted 1,2,3,4,5,6 packed as decimal digits = 123456 */
void msort(int a[], int tmp[], int lo, int hi) {
    int mid; int i; int j; int k;
    if (lo >= hi) { return; }
    mid = (lo + hi) / 2;
    msort(a, tmp, lo, mid);
    msort(a, tmp, mid + 1, hi);
    i = lo; j = mid + 1; k = lo;
    while (i <= mid && j <= hi) {
        if (a[i] <= a[j]) { tmp[k] = a[i]; i = i + 1; }
        else { tmp[k] = a[j]; j = j + 1; }
        k = k + 1;
    }
    while (i <= mid) { tmp[k] = a[i]; i = i + 1; k = k + 1; }
    while (j <= hi) { tmp[k] = a[j]; j = j + 1; k = k + 1; }
    for (i = lo; i <= hi; i = i + 1) { a[i] = tmp[i]; }
}

int main() {
    int a[6]; int t[6];
    int i; int s;
    a[0] = 6; a[1] = 1; a[2] = 5; a[3] = 2; a[4] = 4; a[5] = 3;
    msort(a, t, 0, 5);
    s = 0;
    for (i = 0; i < 6; i = i + 1) { s = s * 10 + a[i]; }
    return s;
}
