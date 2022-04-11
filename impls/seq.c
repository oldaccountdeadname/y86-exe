#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "../memory.h"
#include "../driver.h"

#include "seq.h"

static struct {
	struct memory *mem;
	uint64_t registers[16];
	uint64_t pc;
	
	/* See p. 389 (section 4.3) of Computer Systems. */
	uint64_t a, b, e, p;

	enum cpu_excep ex;
} state = {
	NULL,
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	0,

	0, 0, 0, 0,

	EX_AOK,
};


void
seq_mload(struct memory *m)
{
	state.mem = m;
}

const struct memory *seq_mborrow(void)
{
	return state.mem;
}

enum cpu_excep
seq_step(void)
{
	// TODO
	return state.ex;
}
