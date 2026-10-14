/* A void function must not return a value. The compiler reports this more
   specifically than a generic return-type mismatch: "'return' with a value
   in a function returning void". */
void f() {
    return 5;
}

int main() {
    f();
    return 0;
}
