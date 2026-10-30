/* 1..7 reversed, packed as decimal digits = 7654321 */
int main() {
    int a[7];
    int i; int t; int s;
    for (i = 0; i < 7; i = i + 1) { a[i] = i + 1; }
    for (i = 0; i < 3; i = i + 1) {
        t = a[i]; a[i] = a[6 - i]; a[6 - i] = t;
    }
    s = 0;
    for (i = 0; i < 7; i = i + 1) { s = s * 10 + a[i]; }
    return s;
}
