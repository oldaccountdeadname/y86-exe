#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../memory.h"
#include "../driver.h"
#include "../shared.h"

#include "seq.h"

#define FLAG(f) (state.flags & f)


typedef void (*stage_t)(void);

static void fetch(void);
static void decode(void);
static void execute(void);
static void memory(void);
static void writeback(void);
static void pcupdate(void);
static void changeFetch(void);
static void changeDecode(void);
static void changeExecute(void);
static void changeMemory(void);
static void changePCUpdt(void);
static void printRegs(void);


/* Check condition codes to see if they match what's given in cmpcc. */
static int cmpcc(int);

/* Update SF and ZF. */
static void upopflg(int);

/* check if adding src, src, and getting dst overflowed. */
static int ovfl(int, int, int);

static struct {
	struct memory *mem;
	uint64_t registers[16];
	uint64_t pc;

	enum flag flags;

	/* See p. 384, 389 (section 4.3) of Computer Systems. */
	uint64_t p;

	struct {
		uint64_t m;
	} memory;

	struct {
		uint64_t e;
		short cond;
	} execute;

	struct {
		uint64_t a;
		uint64_t b;
	} decode;

	struct {
		uint64_t a; /* regA */
		uint64_t b; /* regB */
		uint64_t c; /* immediate */
		uint64_t p; /* next pc */
		enum opcode icode; /* operation */
		int ifun; /* function specifier. */
	} fetch;

	enum cpu_excep ex;
} state = {
	NULL,
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	0,

	0,

	0,

	{ 0 },

	{ 0 },

	{ 0, 0 },

	{ 0, 0, 0, 0, 0, 0 },

	EX_AOK,
};

static stage_t stages[] = {
	fetch, decode, execute, memory, writeback, pcupdate
};

static unsigned int nstages = sizeof(stages) / sizeof(*stages);

void
seq_mload(struct memory *m)
{
	state.mem = m;
	state.registers[0x04] = mstack_start(m);
}

const struct memory *seq_mborrow(void)
{
	return state.mem;
}

enum cpu_excep
seq_step(void)
{
	for (unsigned int i = 0; i < nstages; i++) {
		if (state.ex != EX_AOK)
			return state.ex;
		(stages[i])();
	}
	changeFetch();
	changeDecode();
	changeExecute();
	changeMemory();
	changePCUpdt();
	printRegs();
	return state.ex;
}

static void
fetch(void)
{
	int inv;
	struct m_ins ins = mget_ins(state.mem, state.pc, &inv);

	if (inv) {
		state.ex = EX_ADR;
		return;
	}

	state.fetch.icode = ins.bytes[0] & 0xf0;
	state.fetch.ifun  = ins.bytes[0] & 0x0f;
	state.fetch.p = state.pc + 10;

	switch (state.fetch.icode) {
	case OP_JXX:
	case OP_CLL:
		memcpy(&state.fetch.c, &ins.bytes[1], 8);
		break;

	case OP_IRM:
		if ((ins.bytes[1] & 0xf0) != 0xf0) {
			state.ex = EX_INS;
			break;
		}
		
		state.fetch.b = ins.bytes[1] & 0x0f;
		memcpy(&state.fetch.c, &ins.bytes[2], 8);
		break;

	case OP_POP:
	case OP_PSH:
		state.fetch.a = (ins.bytes[1] & 0xf0) >> 4;
		if ((ins.bytes[1] & 0x0f) != 0x0f)
			state.ex = EX_INS;
		break;

	case OP_MRM:
	case OP_RMM:
		state.fetch.a = (ins.bytes[1] & 0xf0) >> 4;
		state.fetch.b =  ins.bytes[1] & 0x0f;
		memcpy(&state.fetch.c, &ins.bytes[2], 8);
		break;

	case OP_OPQ:
	case OP_RRM:
		state.fetch.a = (ins.bytes[1] & 0xf0) >> 4;
		state.fetch.b =  ins.bytes[1] & 0x0f;
		break;

	case OP_HLT:
	case OP_NOP:
	case OP_RET:
		break;
	}
}

static void changeFetch()
{
	printf("~~~~~~~~~~FETCH STAGE~~~~~~~~~~");
	printf("rA: %llu\t rB: %llu\t valC: %llu\t valP: %llu\t icode: %u\t ifun: %d\t", state.fetch.a, state.fetch.b, state.fetch.c, state.fetch.p, state.fetch.icode, state.fetch.ifun);
}

static void
decode(void)
{
	switch (state.fetch.icode) {
	case OP_POP:
		if (state.fetch.a >= 0x0f)
			goto ex_ins;

		state.decode.a = state.decode.b = state.registers[0x04];
		break;

	case OP_PSH:
		if (state.fetch.a >= 0x0f)
			goto ex_ins;

		state.decode.a = state.registers[state.fetch.a];
		state.decode.b = state.registers[0x04]; // %rsp
		break;

	case OP_OPQ:
		if (state.fetch.a >= 0x0f || state.fetch.b >= 0x0f)
			goto ex_ins;

		state.decode.a = state.registers[state.fetch.a];
		state.decode.b = state.registers[state.fetch.b];
		break;

	case OP_MRM:
		if (state.fetch.b >= 0x0f)
			goto ex_ins;

		state.decode.b = state.registers[state.fetch.b];
		break;

	case OP_RMM:
		if (state.fetch.b >= 0x0f)
			goto ex_ins;

		state.decode.b = state.registers[state.fetch.b];
		// FALLTHROUGH
	case OP_RRM:
		if (state.fetch.a >= 0x0f)
			goto ex_ins;

		state.decode.a = state.registers[state.fetch.a];
		break;

	case OP_RET:
		// read %rsp to valA
		state.decode.a = state.registers[0x04];
		break;

	case OP_CLL:
	case OP_HLT:
	case OP_IRM:
	case OP_JXX:
	case OP_NOP:
		break;
	}

	return;

ex_ins:
	state.ex = EX_INS;
}

static void changeDecode()
{
	printf("~~~~~~~~~~DECODE STAGE~~~~~~~~~~");
	printf("valA: %llu\t valB: %llu", state.decode.a, state.decode.b);
}

static void
execute(void)
{
	switch (state.fetch.icode) {
	case OP_CLL:
		// Read the return address to state.execute.e.
		state.execute.e = state.decode.a + 8;
		break;

	case OP_HLT:
		state.ex = EX_HLT;
		break;

	case OP_JXX:
	case OP_RRM:
		state.execute.cond = cmpcc(state.fetch.ifun);
		break;

	case OP_MRM:
		state.execute.e = state.decode.b + state.fetch.c;
		break;

	case OP_OPQ:
		switch (state.fetch.ifun) {
		case 0x0:
			state.execute.e = state.decode.b + state.decode.a;
			state.flags = (state.flags & ~OF) |
				!!(ovfl(state.decode.a, state.decode.b,
					state.execute.e)) << OF_OFFSET;
			break;
		case 0x1:
			state.execute.e = state.decode.b - state.decode.a;
			state.flags = (state.flags & ~OF) |
				!!(ovfl(state.decode.a, state.decode.b,
					state.execute.e)) << OF_OFFSET;
			break;
		case 0x2:
			state.execute.e = state.decode.b & state.decode.a;
			state.flags &= ~OF;
			break;
		case 0x3:
			state.execute.e = state.decode.b ^ state.decode.a;
			state.flags &= ~OF;
			break;
		default:
			state.ex = EX_INS;
			break;
		}

		upopflg(state.execute.e);

		break;

	case OP_POP:
		state.execute.e = state.decode.b + 8;
		break;

	case OP_PSH:
		state.execute.e = state.decode.b - 8;
		break;

	case OP_RET:
		// Read the address of the future PC to mem.
		state.execute.e = state.decode.a - 8;
		break;

	case OP_RMM:
		state.execute.e = state.decode.b + state.fetch.c;
		break;

	case OP_IRM:
	case OP_NOP:
		break;
	}
}

static void changeExecute()
{
	printf("~~~~~~~~~~EXECUTE STAGE~~~~~~~~~~");
	printf("valE: %llu\t Cnd: %hd", state.execute.e, state.execute.cond);
}

static void
memory(void)
{
	int inv;

	switch (state.fetch.icode) {
	case OP_CLL:
	case OP_PSH:
		if (!mset(state.mem, state.execute.e, state.fetch.p))
			goto eadr;
		break;

	case OP_MRM:
		state.memory.m = mget(state.mem, state.execute.e, &inv);
		if (inv)
			goto eadr;
		break;

	case OP_POP:
	case OP_RET:
		state.memory.m = mget(state.mem, state.decode.a, &inv);
		if (inv)
			goto eadr;
		break;

	case OP_RMM:
		if (!mset(state.mem, state.execute.e, state.decode.a))
			goto eadr;
		break;

	case OP_HLT:
	case OP_IRM:
	case OP_JXX:
	case OP_NOP:
	case OP_OPQ:
	case OP_RRM:
		break;
	}

	return;

eadr:
	state.ex = EX_ADR;
}

static void changeMemory()
{
	printf("~~~~~~~~~~MEMORY STAGE~~~~~~~~~~");
	printf("valM: %llu\t", state.memory.m);
}

static void
writeback(void)
{
	switch (state.fetch.icode) {
	case OP_CLL:
	case OP_PSH:
	case OP_RET:
		state.registers[0x04] = state.execute.e;
		break;

	case OP_IRM:
		state.registers[state.fetch.a] = state.fetch.c;
		break;

	case OP_MRM:
		state.registers[state.fetch.a] = state.memory.m;
		break;

	case OP_OPQ:
		state.registers[state.fetch.b] = state.execute.e;
		break;

	case OP_POP:
		state.registers[0x04] = state.execute.e;
		state.registers[state.decode.a] = state.memory.m;
		break;

	case OP_RRM:
		state.registers[state.fetch.b] = state.execute.cond
			? state.decode.a
			: state.decode.b;
		break;

	case OP_HLT:
	case OP_JXX:
	case OP_NOP:
	case OP_RMM:
		break;
	}
}

static void
pcupdate(void)
{
	state.pc = state.fetch.p;
}

static void changePCUpdt()
{
	printf("~~~~~~~~~~EXECUTE STAGE~~~~~~~~~~");
	printf("newPC: %llu\t", state.pc);
}

static int
cmpcc(int op)
{
	// See fig. 3.15. Note that != is used as a substitute for Xor. */
	switch (op) {
	case 0: return 1;

	case 1: /* le */
		return (FLAG(SF) != FLAG(OF)) || FLAG(ZF);
	case 2: /* l */
		return FLAG(SF) != FLAG(OF);
	case 3: /* e */
		return FLAG(ZF);
	case 4: /* ne */
		return !FLAG(ZF);
	case 5: /* ge */
		return !(FLAG(SF) ^ FLAG(OF));
	case 6: /* g */
		return !(FLAG(SF) != FLAG(OF)) && ~FLAG(ZF);
	default:
		// unreachable.
		assert(0);
	}
}

static void
upopflg(int x)
{
	state.flags = (state.flags & ~ZF) | (!!(x == 0) << ZF_OFFSET);
	state.flags = (state.flags & ~SF) | (!!(x  < 0) << SF_OFFSET);
}

static int
ovfl(int a, int b, int r)
{
	return (a < 0 && b < 0 && r >= 0) ||
		(a > 0 && b > 0 && r <= 0);
}


static void printRegs()
{
	printf("RAX\tRBX\tRCX\tRDX\tRSP\tRBP\tRSI\tRDI\tR8\tR9\tR10\tR11\tR12\tR13\tR14\tR15");
	printf("%llu\t%llu\t%llu\t%llu\t%llu\t%llu\t%llu\t%llu\t%llu\t%llu\t%llu\t%llu\t%llu\t%llu\t%llu\t%llu\t", state.registers[0], state.registers[1], state.registers[2], state.registers[3], state.registers[4], state.registers[5], state.registers[6], state.registers[7], state.registers[8], state.registers[9], state.registers[10], state.registers[11], state.registers[12], state.registers[13], state.registers[14], state.registers[15]);
}
