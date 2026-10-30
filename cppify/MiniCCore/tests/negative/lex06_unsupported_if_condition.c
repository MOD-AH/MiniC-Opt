/* An #if condition the subset cannot evaluate is an error, never silently
   treated as true or false. */
#if X > 2
int main(void) { return 0; }
#endif
