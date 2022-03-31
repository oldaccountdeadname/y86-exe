/* <stdio.h> and <stdint.h> must be included before this header. */

struct memory;

/* Load the contents of a file into newly-mapped memory. File contents are at
 * address 0, and everything after is initialized to 0. Memory size is an
 * unspecified constant. */
struct memory *mloadf(FILE *);

/* Deallocate memory. Callers must not reuse a memory pointer after calling this
 * function on it.*/
void mdest(struct memory *);

/* Get the address of the start of the stack. */
uint64_t mstack_start(const struct memory *);
