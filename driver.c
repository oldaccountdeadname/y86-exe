#include <err.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "driver.h"

#define CMD_DESC(name, desc) \
	"\t"name"\t"desc"\n"

static void repl(struct cpu *c);
static void repl_help(void);

int
exec(struct cpu c, const char *p)
{
	/* This maps required memory, then begins execution with the
	 * given CPU implementation. */
	int err;
	FILE *f;
	struct memory *m;

	err = 0;

	f = fopen(p, "r");
	if (!f) {
		fprintf(stderr, "\033[1;31mCould not open %s;\033[0m %s.\n",
			p, strerror(errno));
		err = 1;
		goto ret;
	}

	m = mloadf(f);
	if (!m) {
		err = 1;
		goto cleanup;
	}

	c.mload(m);
	repl(&c);

	mdest(m);

cleanup:
	if (fclose(f) != 0) {
		fprintf(stderr, "\033[1;31mCould not close %s;\033[0m %s.\n",
			p, strerror(errno));
		return 1;
	}

ret:
	return err;
}


static void
repl(struct cpu *c)
{
	enum cpu_excep excep;
	char *cmd = NULL;
	size_t len = 0;
	ssize_t r;
	int repl = 1;

	do {
		if (repl) {
			printf(">>> ");
			r = getline(&cmd, &len, stdin);
			if (r < 0 || strcmp(cmd, "quit\n") == 0)
				break;
			else if (strcmp(cmd, "step\n") == 0)
				excep = c->step();
			else if (strcmp(cmd, "cont\n") == 0)
			        repl = 0;
			else
				repl_help();
		}
		
		if (!repl)
			excep = c->step();
	} while (excep == EX_AOK);

	if (cmd)
		free(cmd);
}

static void
repl_help(void)
{
        printf("Available commands:\n"
	       CMD_DESC("cont", "execute all remaining instructions.")
	       CMD_DESC("help", "display this help message.")
	       CMD_DESC("quit", "quit this REPL.")
	       CMD_DESC("step", "execute one instruction."));
}
