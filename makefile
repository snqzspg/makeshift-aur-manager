CC=gcc
CFLAGS=-Wall -Wpedantic -I lib/jsmn -g
LD=gcc
LDFLAGS=-lcurl -lz 

exec_out=bin/aur_man
src_files=src/arg_parse/arg_commands.c src/aur/pkg_cache.c src/aur/pkg_install_stages.c src/aur/pkg_update_report.c src/aur/pkgver_cache.c src/logger/logger.c src/aur_pkg_parse.c src/aur.c src/file_utils.c src/hashtable.c src/pacman.c src/subprocess_unix.c src/zlib_wrapper.c src/main.c
obj_files=$(patsubst src/%.c, obj/%.o, $(src_files))

completion_srcs=obj/completion/bash_completion.c
completion_objs=$(patsubst %.c, %.o, $(completion_srcs))

.PHONY=run clean clean_completion

$(exec_out): $(obj_files) bin $(completion_objs)
	$(LD) $(LDFLAGS) -o $@ $(obj_files) $(completion_objs)

bin:
	mkdir -p bin

obj:
	mkdir -p obj

obj/completion/bash_completion.c: completion/bash-completion.bash
	mkdir -p obj/completion
	gzip -c $< | python3 dev/gen_completion_c_source.py bash_completion_script_compressed bash_completion > $@

obj/completion/%.o: obj/completion/%.c
	$(CC) $(CFLAGS) -Isrc/completion -c -o $@ $<

obj/%.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

run: $(exec_out)
	./$(exec_out)

clean: clean_completion
	-rm $(exec_out)
	-rm -d $(dir $(exec_out))
	-rm $(obj_files)
	-rm -d $(dir $(obj_files))

clean_completion:
	-rm obj/completion/bash_completion.o
	-rm obj/completion/bash_completion.c
	-rm -d obj/completion
