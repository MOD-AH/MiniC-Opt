struct P { int x; };
int main() {
    struct P p;
    p.x = 1;
    return p->x;
}
