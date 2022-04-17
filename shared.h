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

#define ZF_OFFSET 0
#define SF_OFFSET 1
#define OF_OFFSET 2

enum flag {
	ZF = 1 << ZF_OFFSET,
	SF = 1 << SF_OFFSET,
	OF = 1 << OF_OFFSET,
};
