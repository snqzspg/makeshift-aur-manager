#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

struct linux_dirent {
	long           d_ino;
	off_t          d_off;
	unsigned short d_reclen;
	char           d_name[];
};

int alph_cmp(const void* a, const void* b) {
	return strcmp(*((const char**) a), *((const char**) b));
}

#define RDIR_FRAG_SIZE 256

size_t write_dir_filenames(char** dest, size_t* len_outs, char* is_dir_outs, size_t dest_limit, const char* folder) {
	int fd = open(folder, O_RDONLY | O_DIRECTORY);

	if (fd == -1) {
		(void) fprintf(stderr, "[ERROR] \033[1;31mOpening %s failed\033[0m! - %s\n", folder, strerror(errno));
		return 0;
	}

	char fragment[RDIR_FRAG_SIZE];
	size_t n_entries = 0;

	for (long n_read = syscall(SYS_getdents, fd, fragment, RDIR_FRAG_SIZE); n_read != 0; n_read = syscall(SYS_getdents, fd, fragment, RDIR_FRAG_SIZE)) {
		if (n_read == -1) {
			perror("[ERROR][getdents]");
			return 0;
		}

		for (int bpos = 0; bpos < n_read;) {
			struct linux_dirent* d = (struct linux_dirent*) (fragment + bpos);
			if (n_entries < dest_limit) {
				if (is_dir_outs != NULL) {
					is_dir_outs[n_entries] = *(fragment + bpos + d -> d_reclen - 1) == DT_DIR;
				}

				if (len_outs != NULL) {
					len_outs[n_entries] = strlen(d -> d_name);
				}

				if (dest != NULL) {
					(void) strncpy(dest[n_entries], d -> d_name, strlen(d -> d_name) + 1);
				}
			}
			bpos += d -> d_reclen;
			n_entries++;
		}
	}

	(void) close(fd);

	if (dest != NULL) {
		qsort(dest, dest_limit < n_entries ? dest_limit : n_entries, sizeof(char*), alph_cmp);
	}

	return n_entries;
}

int main(int argc, char** argv) {
	char* fdr = argc > 1 ? argv[1] : ".";

	size_t n_entries = write_dir_filenames(NULL, NULL, NULL, 0, fdr);
	(void) printf("%zu\n", n_entries);

	char is_dir_bools[n_entries];
	size_t fn_lens[n_entries];

	(void) write_dir_filenames(NULL, fn_lens, is_dir_bools, n_entries, fdr);

	size_t total_len_alloc = 0;
	for (size_t i = 0; i < n_entries; i++) {
		total_len_alloc += fn_lens[i] + 1;
	}

	char dirname_stash[total_len_alloc];
	char *dirnames[n_entries];

	for (size_t i = 0, j = 0; i < n_entries; i++) {
		dirnames[i] = dirname_stash + j;
		j += fn_lens[i] + 1;
	}

	(void) write_dir_filenames(dirnames, fn_lens, is_dir_bools, n_entries, fdr);

	for (size_t i = 0; i < n_entries; i++) {
		(void) printf("%s%s%s\n", is_dir_bools[i] ? "\033[1;34m" : "", dirnames[i], is_dir_bools[i] ? "\033[0m" : "");
	}

	return 0;
}
