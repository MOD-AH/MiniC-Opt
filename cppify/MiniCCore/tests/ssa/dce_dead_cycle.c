// `dead` is only ever read to compute its own next value, so liveness says
// it is live and P5 keeps all of it. Mark-sweep DCE starts from what has an
// effect (the return) and never reaches `dead`, so its updates go.
int main() {
    int i, dead = 0, s = 0;
    for (i = 0; i < 10; i = i + 1) {
        dead = dead + i * 3;
        s = s + i;
    }
    return s;
}
