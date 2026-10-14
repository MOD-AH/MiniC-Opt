/* ++ needs something assignable; (x + 1) is a value, not a location. */
int main() {
    int x;
    x = 1;
    (x + 1)++;
    return x;
}
