/* MiniC has no switch; this is the else-if chain that stands in for one.
   31+28+31+30+31 = 151 */
int day_len(int d) {
    if (d == 1) { return 31; }
    else if (d == 2) { return 28; }
    else if (d == 3) { return 31; }
    else if (d == 4) { return 30; }
    else if (d == 5) { return 31; }
    else { return 0; }
}

int main() {
    int i; int s;
    s = 0;
    for (i = 1; i <= 5; i = i + 1) { s = s + day_len(i); }
    return s;
}
