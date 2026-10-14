/* void functions communicating only through a global, including one
   with a bare `return;`.  Three bumps -> 3. */
int counter = 0;

void bump() {
    counter = counter + 1;
}

void noop() {
    return;
}

int main() {
    bump();
    noop();
    bump();
    bump();
    return counter;
}
