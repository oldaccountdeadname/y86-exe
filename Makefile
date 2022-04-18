CC ?= gcc
CFLAGS += -std=c99 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -g

y86-exe: driver.o main.o memory.o impls/seq.o
	$(CC) $(CFLAGS) driver.c main.o memory.o impls/seq.o -o y86-exe
main.o: main.c driver.h impls/seq.h memory.h
driver.o: driver.c driver.h memory.h
memory.o: memory.c memory.h
impls/seq.o: impls/seq.c impls/seq.h memory.h driver.h shared.h

clean:
	rm -f y86-exe
	rm -f main.o
	rm -f driver.o
