/* Three exit points in one function. 1*100 + (-1)*10 + 0 = 90 */
int sign_of(int x) {
    if (x > 0) { return 1; }
    if (x < 0) { return -1; }
    return 0;
}

int main() {
    return sign_of(5) * 100 + sign_of(-3) * 10 + sign_of(0);
}
