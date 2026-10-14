/* Bitwise operators act on bit patterns; a float has none here. This used
   to type-check as a float operation and silently compute garbage. */
int main() {
    float f;
    f = 1.5;
    return f & 2;
}
