/* 3^7 = 2187 */
int power(int base, int exp) {
    int r; int i;
    r = 1;
    for (i = 0; i < exp; i = i + 1) { r = r * base; }
    return r;
}

int main() {
    return power(3, 7);
}
