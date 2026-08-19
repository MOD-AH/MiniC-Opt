int search(int a[], int n, int key) {
    int i = 0;
    while (i < n) {
        if (a[i] == key) {
            return i;
        }
        i = i + 1;
    }
    return -1;
}

int main() {
    int a[5];
    a[0] = 10; a[1] = 20; a[2] = 30; a[3] = 40; a[4] = 50;
    return search(a, 5, 40);
}
