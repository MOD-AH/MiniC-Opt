/* Inner loop never runs because the outer index keeps it empty. */
int main() {
    int i; int j; int s;
    s = 0;
    for (i = 0; i < 5; i = i + 1) {
        for (j = 0; j < 0; j = j + 1) { s = s + 1; }
    }
    return s;
}
