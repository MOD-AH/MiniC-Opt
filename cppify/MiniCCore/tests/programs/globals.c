/* Globals with initializers, read from a function other than the one
   that declared them. 10 + 32 = 42 */
int g1 = 10;
int g2 = 32;

int add_globals() {
    return g1 + g2;
}

int main() {
    return add_globals();
}
