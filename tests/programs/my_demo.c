int sumOfSquares(int n) {
    int s;
    int i;
    s = 0;
    i = 1;
    while (i <= n) {
        s = s + i * i;
        i = i + 1;
    }
    return s;
}

int main() {
    return sumOfSquares(5);
}
