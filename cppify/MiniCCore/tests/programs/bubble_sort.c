int main() {
    int a[8];
    int i;
    int j;
    a[0] = 5; a[1] = 2; a[2] = 9; a[3] = 1;
    a[4] = 7; a[5] = 3; a[6] = 8; a[7] = 4;

    for (i = 0; i < 7; i = i + 1) {
        for (j = 0; j < 7 - i; j = j + 1) {
            if (a[j] > a[j + 1]) {
                int t = a[j];
                a[j] = a[j + 1];
                a[j + 1] = t;
            }
        }
    }
    return a[0] + a[7];
}
