/* memory.h must be included before this header file. */

enum cpu_excep {
	EX_AOK = 0, EX_HLT, EX_ADR, EX_INS,
};

struct cpu {
	void (*mload)(struct memory *);
	const struct memory *(*mborrow)(void);
	enum cpu_excep (*step)(void);
};

int exec(struct cpu, const char *);
