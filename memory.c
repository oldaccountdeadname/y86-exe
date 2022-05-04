#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "memory.h"

// Let's allocate 4 MB of memory. Because virtual memory, this is all allocated
// lazily :)
#define MEMSIZE ((4096 * 1024))

struct memory {
	struct {
		char *x;
		int fd;
	} alloc;

	uint64_t prog_size;
};

struct memory *
mloadf(FILE *f)
{
	void *map;
	int zf;
	long len;

	fseek(f, 0, SEEK_END);
	len = ftell(f);
	fseek(f, 0, SEEK_SET);

	if (len > MEMSIZE) {
		fprintf(stderr, "\033[1;31mGiven executable is too large.\033[0m\n");
		return NULL;
	}

	zf = open("/dev/zero", O_RDONLY);
	if (zf < 0) {
		fprintf(stderr, "\033[1;31m/dev/zero somehow inaccessible.\033[0m\n");
		return NULL;
	}

	map = mmap(NULL, MEMSIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, zf, 0);
	if (map == MAP_FAILED) {
		fprintf(stderr, "\033[1;31mCouldn't map memory for emulation.\033[0m\n");
		fprintf(stderr, "\Err #%d\n", errno);
		return NULL;
	}

	if (fread(map, 1, len, f) == 0) {
		fprintf(stderr,	"\033[1;31mCouldn't read file.\033[0m\n");
		munmap(map, MEMSIZE);
		return NULL;
	}

	struct memory *m = malloc(sizeof(struct memory));
	if (!m) {
		munmap(map, MEMSIZE);
		return NULL;
	}

	m->alloc.x = map;
	m->alloc.fd = zf;
	m->prog_size = len;

	return m;
}

void
mdest(struct memory *m)
{
	munmap(m->alloc.x, MEMSIZE);
	close(m->alloc.fd);
	free(m);
}

int
mset(struct memory *m, uint64_t addr, uint64_t val)
{
	if (addr > m->prog_size)
		return 0;

	m->alloc.x[addr] = val;
	return 1;
}

uint64_t
mget(const struct memory *m, uint64_t addr, int *oor)
{
	if (addr + 8 > m->prog_size) {
		*oor = 1;
		return 0;
	} else {
		*oor = 0;
		return m->alloc.x[addr];
	}
}

struct m_ins
mget_ins(const struct memory *m, uint64_t addr, int *inv)
{
	struct m_ins ins = {
		.bytes = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	};

	if (addr + 10 > m->prog_size) {
		*inv = 1;
		return ins;
	} else {
		*inv = 0;
		memcpy(&ins.bytes, m->alloc.x + addr, 10);
		return ins;
	}
}

uint64_t
mstack_start(const struct memory *m)
{
	return m->prog_size;
}
