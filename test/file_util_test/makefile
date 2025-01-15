CC=gcc
CFLAGS=-Wall -Wpedantic -g -I ../../src
LD=gcc
LDFLAGS=-I ../../src

.PHONY=clean

a.out: read_test.c file_utils.o
	$(LD) $(LDFLAGS) -o $@ $^

file_utils.o: ../../src/file_utils.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	-rm a.out file_utils.o
