/* a..d form a chain of assignments nothing ever reads; DCE should
   collapse the whole chain in one backward walk. */
int main() {
    int a; int b; int c; int d; int live;
    a = 1;
    b = a + 1;
    c = b + 1;
    d = c + 1;
    live = 7;
    return live;
}
