/* Largest representable int, divided down so the printed value is
   small but only correct if the literal survived intact. */
int main() {
    int a;
    a = 2147483647;
    return a / 1000000;
}
