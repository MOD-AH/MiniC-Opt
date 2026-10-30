/* 3x3 stored row-major in a flat array: fill with 0..8, transpose into
   b, then sum b's diagonal. A transpose leaves the diagonal alone, so
   the answer is a[0]+a[4]+a[8] = 0+4+8 = 12 */
int main() {
    int a[9]; int b[9];
    int i; int j; int s;
    for (i = 0; i < 3; i = i + 1) {
        for (j = 0; j < 3; j = j + 1) {
            a[i * 3 + j] = i * 3 + j;
        }
    }
    for (i = 0; i < 3; i = i + 1) {
        for (j = 0; j < 3; j = j + 1) {
            b[j * 3 + i] = a[i * 3 + j];
        }
    }
    s = 0;
    for (i = 0; i < 3; i = i + 1) { s = s + b[i * 3 + i]; }
    return s;
}
