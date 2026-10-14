/* Two different sorts over the same data must agree element for element.
   same=1 and the largest element is 9, so 1*100 + 9 = 109 */
void bubble(int a[], int n) {
    int i; int j; int t;
    for (i = 0; i < n - 1; i = i + 1) {
        for (j = 0; j < n - 1 - i; j = j + 1) {
            if (a[j] > a[j + 1]) { t = a[j]; a[j] = a[j + 1]; a[j + 1] = t; }
        }
    }
}

void selection(int a[], int n) {
    int i; int j; int mi; int t;
    for (i = 0; i < n - 1; i = i + 1) {
        mi = i;
        for (j = i + 1; j < n; j = j + 1) {
            if (a[j] < a[mi]) { mi = j; }
        }
        t = a[i]; a[i] = a[mi]; a[mi] = t;
    }
}

int main() {
    int a[5]; int b[5];
    int i; int same;
    a[0] = 5; a[1] = 2; a[2] = 9; a[3] = 1; a[4] = 7;
    b[0] = 5; b[1] = 2; b[2] = 9; b[3] = 1; b[4] = 7;
    bubble(a, 5);
    selection(b, 5);
    same = 1;
    for (i = 0; i < 5; i = i + 1) {
        if (a[i] != b[i]) { same = 0; }
    }
    return same * 100 + a[4];
}
