/* 12321 is a palindrome, 12345 is not -> 1*10 + 0 = 10 */
int is_pal(int n) {
    int r; int o;
    o = n; r = 0;
    while (n > 0) { r = r * 10 + n % 10; n = n / 10; }
    if (r == o) { return 1; }
    return 0;
}

int main() {
    return is_pal(12321) * 10 + is_pal(12345);
}
