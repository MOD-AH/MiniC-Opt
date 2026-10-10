/* sorted: 5,10,13,14,29,37 -> 5*100 + 37 = 537 */
int main() {
    int a[6];
    int i; int j; int mi; int t;
    a[0] = 29; a[1] = 10; a[2] = 14; a[3] = 37; a[4] = 13; a[5] = 5;
    for (i = 0; i < 5; i = i + 1) {
        mi = i;
        for (j = i + 1; j < 6; j = j + 1) {
            if (a[j] < a[mi]) { mi = j; }
        }
        t = a[i]; a[i] = a[mi]; a[mi] = t;
    }
    return a[0] * 100 + a[5];
}
