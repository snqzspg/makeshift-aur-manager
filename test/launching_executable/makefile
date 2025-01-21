CC=gcc
CFLAGS=-Wall -Wpedantic -g -I ../../src
LD=gcc
LDFLAGS=-I ../../src

.PHONY=clean

a.out: vercmp.c subprocess_unix.o
	$(LD) $(LDFLAGS) -o $@ $^

subprocess_unix.o: ../../src/subprocess_unix.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	-rm a.out subprocess_unix.o
	-if [ -d __pkg_cache__ ]; then rm -rfvd __pkg_cache__; fi
