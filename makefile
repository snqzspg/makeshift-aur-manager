CC=gcc
CFLAGS=-Wall -Wpedantic -I lib/jsmn -g
LD=gcc
LDFLAGS=-lcurl 

exec_out=bin/aur_man
src_files=src/aur/pkg_cache.c src/aur/pkgver_cache.c src/aur_pkg_parse.c src/aur.c src/file_utils.c src/hashtable.c src/pacman.c src/subprocess_unix.c src/main.c
obj_files=$(patsubst src/%.c, obj/%.o, $(src_files))

.PHONY=run clean

$(exec_out): $(obj_files) bin
	$(LD) $(LDFLAGS) -o $@ $(obj_files)

bin:
	mkdir -p bin

obj:
	mkdir -p obj

obj/%.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

run: $(exec_out)
	./$(exec_out)

clean:
	-rm $(exec_out)
	-rm -d $(dir $(exec_out))
	-rm $(obj_files)
	-rm -d $(dir $(obj_files))
