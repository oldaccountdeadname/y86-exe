#include <assert.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../memory.h"
#include "../driver.h"
#include "../shared.h"

#include "seq.h"

#define FLAG(f) (state.flags & f)

#define STAGE(s) "\n[\033[1m"s" STAGE\033[0m]\n\n"

typedef void (*stage_t)(void);

static void fetch(void);
static void decode(void);
static void execute(void);
static void memory(void);
static void writeback(void);
static void pcupdate(void);

static void disp_fetch(void);
static void disp_decode(void);
static void disp_exec(void);
static void disp_mem(void);
static void disp_pc_updt(void);
static void disp_writeback(void);
static void disp_excep(void);


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

	struct {
		int zf:1;
		int sf:1;
		int of:1;
	} flags;

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

	{ 0, 0, 0 },

	0,

	{ 0 },

	{ 0 },

	{ 0, 0 },

	{ 0, 0, 0, 0, 0, 0 },

	EX_AOK,
};

static const stage_t stages[] = {
	fetch, disp_fetch, decode, disp_decode, execute, disp_exec,
	memory, disp_mem, writeback, disp_writeback, pcupdate, disp_pc_updt,
};

static const unsigned int nstages = sizeof(stages) / sizeof(*stages);

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
	for (unsigned int i = 0; i < nstages; i += 2) {
		if (state.ex != EX_AOK)
			return state.ex;
		(stages[i])();
		(stages[i + 1])();
	}
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
			state.flags.of = ovfl(state.decode.b, state.decode.a,
					      state.execute.e);
			break;
		case 0x1:
			state.execute.e = state.decode.b - state.decode.a;
			state.flags.of = ovfl(-state.decode.b, state.decode.a,
					      state.execute.e);
			break;
		case 0x2:
			state.execute.e = state.decode.b & state.decode.a;
			state.flags.of = 0;
			break;
		case 0x3:
			state.execute.e = state.decode.b ^ state.decode.a;
			state.flags.of = 0;
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
		state.registers[state.fetch.b] = state.fetch.c;
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

static void
disp_fetch()
{
	printf(STAGE("FETCH"));
	printf("rA:\t%"PRIu64"\trB:\t%"PRIu64"\tvalC:\t%"PRIu64
	       "\tvalP:\t%"PRIu64"\nicode:\t%u\tifun:\t%d\n", state.fetch.a, state.fetch.b, state.fetch.c, state.fetch.p, state.fetch.icode, state.fetch.ifun);
	printf("symbolic:\t%s\n", syms[state.fetch.icode + state.fetch.ifun]);
	disp_excep();
}

static void
disp_decode()
{
	printf(STAGE("DECODE"));
	printf("valA:\t%"PRIu64"\tvalB:\t%"PRIu64"\n", state.decode.a, state.decode.b);
	disp_excep();
}

static void
disp_exec()
{
	printf(STAGE("EXECUTE"));
	printf("valE:\t%"PRIu64"\tCnd: %hd\n", state.execute.e, state.execute.cond);
	disp_excep();
}

static void
disp_mem()
{
	printf(STAGE("MEMORY"));
	printf("valM:\t%"PRIu64"\t\n", state.memory.m);
	disp_excep();
}

static void
disp_pc_updt()
{
	printf(STAGE("EXECUTE"));
	printf("newPC:\t%"PRIu64"\n", state.pc);
	disp_excep();
}

static int
cmpcc(int op)
{
	// See fig. 3.15. Note that != is used as a substitute for Xor. */
	switch (op) {
	case 0: return 1;

	case 1: /* le */
		return (state.flags.sf != state.flags.of) || state.flags.zf;
	case 2: /* l */
		return state.flags.sf != state.flags.of;
	case 3: /* e */
		return state.flags.zf;
	case 4: /* ne */
		return !state.flags.zf;
	case 5: /* ge */
		return !(state.flags.sf ^ state.flags.of);
	case 6: /* g */
		return !(state.flags.sf != state.flags.of) && ~state.flags.zf;
	default:
		// unreachable.
		assert(0);
	}
}

static void
upopflg(int x)
{
	state.flags.zf = x == 0;
	state.flags.sf = x < 0;
}

static int
ovfl(int a, int b, int r)
{
	return (a < 0 && b < 0 && r >= 0) ||
		(a > 0 && b > 0 && r <= 0);
}


static
void disp_writeback()
{
	printf(STAGE("WRITEBACK"));
	printf("RAX\tRBX\tRCX\tRDX\n");
	printf("%"PRIu64"\t%"PRIu64"\t%"PRIu64"\t%"PRIu64"\n",
	       state.registers[0], state.registers[1], state.registers[2],
	       state.registers[3]);

	printf("RSP\tRBP\tRSI\tRDI\n");
	printf("%"PRIu64"\t%"PRIu64"\t%"PRIu64"\t%"PRIu64"\n",
	       state.registers[4], state.registers[5], state.registers[6],
	       state.registers[7]);

	printf("R8\tR9\tR10\tR11\n");
	printf("%"PRIu64"\t%"PRIu64"\t%"PRIu64"\t%"PRIu64"\n",
	       state.registers[8], state.registers[9], state.registers[10],
	       state.registers[11]);

	printf("R12\tR13\tR14\n");
	printf("%"PRIu64"\t%"PRIu64"\t%"PRIu64"\n",
	       state.registers[12], state.registers[13], state.registers[14]);
}

static void
disp_excep(void)
{
	printf("exception:\t%s\n", EXCEPTION(state.ex));
}
