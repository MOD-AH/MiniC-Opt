/* The same binary search, in MiniC.
   Differences forced by the language, not by choice:
     - no #include: there are no headers
     - no printf/scanf: print_int and read_int are built in
     - no int arr[n] (variable-length array): size must be a constant
     - no &arr[i]: read_int() returns the value instead */
int binarySearch(int arr[], int size, int target) {
    int low;  int high;  int mid;
    low = 0;
    high = size - 1;
    while (low <= high) {
        mid = low + (high - low) / 2;
        if (arr[mid] == target) { return mid; }
        if (arr[mid] < target) { low = mid + 1; } else { high = mid - 1; }
    }
    return -1;
}

int main() {
    int arr[64];
    int n;  int target;  int i;  int result;
    n = read_int();
    for (i = 0; i < n; i = i + 1) { arr[i] = read_int(); }
    target = read_int();
    result = binarySearch(arr, n, target);
    print_int(result);
    return 0;
}
