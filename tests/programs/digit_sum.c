/* 9+8+7+6+5+4 = 39 */
int dsum(int n) {
    int s;
    s = 0;
    while (n > 0) { s = s + n % 10; n = n / 10; }
    return s;
}

int main() {
    return dsum(987654);
}
