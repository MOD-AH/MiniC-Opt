/* Highest in-bounds index of the array must be writable and readable. */
int main() {
    int a[10];
    a[9] = 5;
    return a[9];
}
