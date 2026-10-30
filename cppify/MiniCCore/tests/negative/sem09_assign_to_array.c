/* Whole-array assignment is not a thing; elements are assignable. */
int main() {
    int a[3];
    int b[3];
    a = b;
    return 0;
}
