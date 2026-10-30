/* `int a[];` gives the compiler nothing to infer a length from. */
int main() {
    int a[];
    return 0;
}
