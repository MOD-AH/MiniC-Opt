/* P6 unreachable-code elimination: the condition is a literal constant,
   so once P2 constant-propagates it into the branch's own test, P6 is
   the only pass licensed to sever the dead edge and delete the block. */
int main() {
    int s;
    s = 1;
    if (0) {
        s = 999;
    }
    return s;
}
