/* Euclid, recursive this time (gcd.c is the iterative version).
   gcd(1071, 462) = 21 */
int gcd_r(int a, int b) {
    if (b == 0) { return a; }
    return gcd_r(b, a % b);
}

int main() {
    return gcd_r(1071, 462);
}
