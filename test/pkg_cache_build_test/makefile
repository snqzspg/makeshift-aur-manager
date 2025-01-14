CC=gcc
CFLAGS=-Wall -Wpedantic -g -I ../../src
LD=gcc
LDFLAGS=-I ../../src

.PHONY=clean

a.out: build_test.c pkg_cache.o
	$(LD) $(LDFLAGS) -o $@ $^

pkg_cache.o: ../../src/aur/pkg_cache.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	-rm a.out pkg_cache.o
	-if [ -d __pkg_cache__ ]; then rm -rfvd __pkg_cache__; fi
