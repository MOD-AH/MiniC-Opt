struct A { int x; };
struct B { int x; };
int main() {
    struct A a;
    struct B b;
    a.x = 1;
    b = a;
    return b.x;
}
