/* lcm(12,18) = 12 / gcd(12,18) * 18 = 2 * 18 = 36 */
int gcd_i(int a, int b) {
    int t;
    while (b != 0) { t = b; b = a % b; a = t; }
    return a;
}

int main() {
    int a; int b;
    a = 12; b = 18;
    return (a / gcd_i(a, b)) * b;
}
