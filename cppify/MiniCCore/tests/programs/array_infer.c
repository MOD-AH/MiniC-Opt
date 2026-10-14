/* An array whose length comes from its initializer, and char arrays
   initialised from a string literal.
     sum of {10,20,30,40,50} = 150
     "cppify" is 6 characters, so name[0..2] = c p p and name[6] is the
       terminating 0
     buf is "ab" padded to 10: {97, 98, 0, 0, ...}
     return 150 / 10 = 15 */
int main() {
    int nums[] = {10, 20, 30, 40, 50};
    char name[] = "cppify";
    char buf[10] = "ab";
    int sum = 0;
    for (int i = 0; i < 5; i++) sum += nums[i];
    printf("sum = %d\n", sum);
    printf("%c%c%c\n", name[0], name[1], name[2]);
    printf("%d %d %d\n", buf[0], buf[1], buf[2]);
    printf("%d\n", name[6]);
    return sum / 10;
}
