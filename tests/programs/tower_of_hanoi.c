/* Moves for 5 discs is 2^5 - 1 = 31. The count lives in a global, so
   this also exercises a recursive function with a global side effect. */
int moves = 0;

void hanoi(int n, int from, int to, int via) {
    if (n == 0) { return; }
    hanoi(n - 1, from, via, to);
    moves = moves + 1;
    hanoi(n - 1, via, to, from);
}

int main() {
    hanoi(5, 1, 3, 2);
    return moves;
}
