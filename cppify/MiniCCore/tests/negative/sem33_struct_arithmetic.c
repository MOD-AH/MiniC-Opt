struct P { int x; };
int main() {
    struct P a, b;
    a.x = 1; b.x = 2;
    return a + b;
}
