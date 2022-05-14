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
