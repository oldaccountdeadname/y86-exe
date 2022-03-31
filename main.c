#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "memory.h"
#include "driver.h"

#include "impls/seq.h"

#define HELP_MSG \
"    %s [ --help | -h] | <exe>\n" \
"\n"									\
"%s emulates a y86 CPU. Pass the y86 executable as the first and only argument.\n"

static void read_args(int, char **);
static int launch(void);

/* This structure is initialized by read_args. Do not call launch without first
 * sufficiently initializing this structure with read_args. */
static struct {
	int help;
	char *pname;
	char *fname;
} args;

static struct cpu seq = {
	.mborrow = seq_mborrow,
	.mload = seq_mload,
	.step = seq_step,
};

int
main(int argc, char **argv)
{
	read_args(argc, argv);
	return launch();
}

static void
read_args(int argc, char **argv)
{
	char *x;
	args.pname = argv[0];
	args.fname = x = argv[1];

	if (argc != 2)
		args.help = -1;
        else if (strcmp(x, "--help") == 0 || strcmp(x, "-h") == 0)
		args.help = 1;
}

static int
launch(void)
{
	if (args.help) {
		printf(HELP_MSG, args.pname, args.pname);
		return args.help == -1;
	}

	return exec(seq, args.fname);
}
