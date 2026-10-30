// Inside the loop a[2] is a load from a local array of 4 at a constant,
// in-bounds index, and nothing in the loop stores to `a` or calls anything.
// licm-ssa hoists the load; P8 never moves a load. k / 2 is a division by a
// non-zero constant, which cannot trap, so it is hoisted too (P8 refuses all
// divisions).
int main() {
    int a[4];
    int i, s = 0, k;
    a[0] = 5; a[1] = 6; a[2] = 7; a[3] = 8;
    k = a[3];
    for (i = 0; i < 10; i = i + 1) {
        s = s + a[2] + k / 2;
    }
    return s;
}
