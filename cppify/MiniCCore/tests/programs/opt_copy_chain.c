/* A five-link copy chain; copy propagation should collapse it to one. */
int main() {
    int a; int b; int c; int d; int e; int f;
    a = 9;
    b = a; c = b; d = c; e = d; f = e;
    return f;
}
