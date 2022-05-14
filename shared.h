/* Define a macro to index the exception array in a memory-safe
 * manner. */
#define EXCEPTION(n) exps[n > 3 ? 4 : n]

enum opcode {
	OP_HLT = 0x00,
	OP_NOP = 0x10,
	OP_RRM = 0x20, /* rrmovq, cmovXX */
	OP_IRM = 0x30, /* irmovq */
	OP_RMM = 0x40, /* rmmovq */
	OP_MRM = 0x50, /* mrmovq */
	OP_OPQ = 0x60, /* OPq */
	OP_JXX = 0x70, /* jXX */
	OP_CLL = 0x80, /* call */
	OP_RET = 0x90, /* ret */
	OP_PSH = 0xA0, /* pushq */
	OP_POP = 0xB0, /* popq */
};

static const char *exps[] = {
	"\033[32mAOK\033[0m", "HLT", "ADR", "INS",
	"INVALID EXCEPTION CODE ENCOUNTERED",
};

static const char *syms[] = {
	[0x00] = "hlt",
	[0x10] = "nop",

	[0x20] = "rrmovq",
	[0x30] = "irmovq",
	[0x40] = "rmmovq",
	[0x50] = "mrmovq",

	[0x60] = "addq",
	[0x61] = "subq",
	[0x62] = "andq",
	[0x63] = "xorq",

	[0x70] = "jmp",
	[0x71] = "jle",
	[0x72] = "jl",
	[0x73] = "je",
	[0x74] = "jne",
	[0x75] = "jge",
	[0x76] = "jg",

	[0x21] = "cmovle",
	[0x22] = "cmovl",
	[0x23] = "cmove",
	[0x24] = "cmovne",
	[0x25] = "cmovge",
	[0x26] = "cmovg",

	[0x80] = "call",
	[0x90] = "ret",
	[0xa0] = "pushq",
	[0xb0] = "popq",
};
