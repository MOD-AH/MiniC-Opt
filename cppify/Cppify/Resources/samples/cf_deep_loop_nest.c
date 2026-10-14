/* Four levels of loop nesting: 2 * 3 * 2 * 2 = 24 */
int main() {
    int i; int j; int k; int l; int s;
    s = 0;
    for (i = 0; i < 2; i = i + 1) {
        for (j = 0; j < 3; j = j + 1) {
            for (k = 0; k < 2; k = k + 1) {
                for (l = 0; l < 2; l = l + 1) {
                    s = s + 1;
                }
            }
        }
    }
    return s;
}
