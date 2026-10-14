/* Eight-way else-if chain, each arm taken exactly once.
   1+2+...+8 = 36 */
int classify(int x) {
    if (x < 10) { return 1; }
    else if (x < 20) { return 2; }
    else if (x < 30) { return 3; }
    else if (x < 40) { return 4; }
    else if (x < 50) { return 5; }
    else if (x < 60) { return 6; }
    else if (x < 70) { return 7; }
    else { return 8; }
}

int main() {
    int i; int s;
    s = 0;
    for (i = 0; i < 80; i = i + 10) {
        s = s + classify(i);
    }
    return s;
}
