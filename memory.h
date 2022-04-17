/* <stdio.h> and <stdint.h> must be included before this header. */;

struct memory;

/* Store a 10-byte instruction. This is a glorified typedef. */
struct m_ins {
	char bytes[10];
};

/* Load the contents of a file into newly-mapped memory. File contents are at
 * address 0, and everything after is initialized to 0. Memory size is an
 * unspecified constant. */
struct memory *mloadf(FILE *);

/* Deallocate memory. Callers must not reuse a memory pointer after calling this
 * function on it.*/
void mdest(struct memory *);

/* Set the memory address indexed by the first argument to the value
 * given in the second argument. Returns zero if the address is out of
 * range, non-zero otherwise. */
int mset(struct memory *, uint64_t, uint64_t);

/* Get the memory address indicated by the second argument. If out of
 * range, the value at the given integer pointer is set to 1,
 * otherwise to 0. If out of range, the return value is undefined. */
uint64_t mget(const struct memory *, uint64_t, int *);
struct m_ins mget_ins(const struct memory *, uint64_t, int *);

/* Get the address of the start of the stack. */
uint64_t mstack_start(const struct memory *);
