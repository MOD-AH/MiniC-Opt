int main() {
    int a[6];
    int i;
    a[0] = 6; a[1] = 3; a[2] = 8; a[3] = 1; a[4] = 5; a[5] = 2;

    for (i = 1; i < 6; i = i + 1) {
        int key = a[i];
        int j = i - 1;
        while (j >= 0 && a[j] > key) {
            a[j + 1] = a[j];
            j = j - 1;
        }
        a[j + 1] = key;
    }
    return a[5];
}
