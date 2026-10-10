/* h(2)=3, g(3)=6, f(6)=16 */
int h(int x) { return x + 1; }
int g(int x) { return x * 2; }
int f(int x) { return x + 10; }

int main() {
    return f(g(h(2)));
}
