int main() {
    int a[10];
    int i;
    for (i = 0; i < 10; i = i + 1) {
        a[i] = i + 1;
    }
    for (i = 1; i < 10; i = i + 1) {
        a[i] = a[i] + a[i - 1];
    }
    return a[9];
}
