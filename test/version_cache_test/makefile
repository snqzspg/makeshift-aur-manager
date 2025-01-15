CC=gcc
CFLAGS=-Wall -Wpedantic -g -I ../../src
LD=gcc
LDFLAGS=-Wall -Wpedantic -g -I ../../src

.PHONY=clean

a.out: version_cache.c file_utils.o pacman.o pkg_cache.o pkgver_cache.o
	$(LD) $(LDFLAGS) -o $@ $^

file_utils.o: ../../src/file_utils.c
	$(CC) $(CFLAGS) -c -o $@ $<

pacman.o: ../../src/pacman.c
	$(CC) $(CFLAGS) -c -o $@ $<

pkg_cache.o: ../../src/aur/pkg_cache.c
	$(CC) $(CFLAGS) -c -o $@ $<

pkgver_cache.o: ../../src/aur/pkgver_cache.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	-rm a.out file_utils.o pacman.o pkg_cache.o pkgver_cache.o
	-if [ -d __pkg_cache__ ]; then rm -rfvd __pkg_cache__; fi
