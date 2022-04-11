#include <err.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "driver.h"

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
	while (c.step() == EX_AOK);

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
