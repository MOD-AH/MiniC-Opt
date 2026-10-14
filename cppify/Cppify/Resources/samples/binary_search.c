int bsearch(int a[], int n, int key) {
    int lo = 0;
    int hi = n - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        if (a[mid] == key) {
            return mid;
        }
        if (a[mid] < key) {
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    return -1;
}

int main() {
    int a[7];
    a[0] = 1; a[1] = 3; a[2] = 5; a[3] = 7;
    a[4] = 9; a[5] = 11; a[6] = 13;
    return bsearch(a, 7, 11);
}
