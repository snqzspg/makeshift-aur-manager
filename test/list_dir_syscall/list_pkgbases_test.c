#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
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

/**
 * Iterate through the entries inside a given folder, performing a function for each entry.
 * 
 * @param folder The path to the folder to look into
 * @param recursive If set to a non-zero value, it will iterate through all files within subfolders
 * @param fx_to_exec_content The function to apply for each entry. The parameters are provided as follows:
 *                           (const char* full_name, const struct linux_dirent* d, char d_type, void* data_passed)
 *                           Return a non-zero value to prematuraly terminate the iteration.
 * @param data_to_pass Pointer to a mutable area that can be accessed within the function for each entry.
 */
size_t iter_folder_contents(const char* folder, int recursive, int (*fx_to_exec_content)(const char*, const struct linux_dirent*, char, void*), void* data_to_pass) {
	int         folder_is_blank   = folder == NULL || *folder == '\0';
	const char* folder_to_syscall = folder_is_blank ? "." : folder;

	int fd = open(folder_to_syscall, O_RDONLY | O_DIRECTORY);

	if (fd == -1) {
		(void) fprintf(stderr, "[\033[1;31mERROR\033[0m] Opening %s failed! - %s\n", folder_to_syscall, strerror(errno));
		return 0;
	}

	char fragment[RDIR_FRAG_SIZE];
	size_t n_entries = 0;

	for (long n_read = syscall(SYS_getdents, fd, fragment, RDIR_FRAG_SIZE); n_read != 0; n_read = syscall(SYS_getdents, fd, fragment, RDIR_FRAG_SIZE)) {
		if (n_read == -1) {
			perror("[\033[1;31mERROR\033[0m][getdents]");
			(void) close(fd);
			return 0;
		}

		for (int bpos = 0; bpos < n_read;) {
			struct linux_dirent* d = (struct linux_dirent*) (fragment + bpos);
			char d_type = *(fragment + bpos + d -> d_reclen - 1);
			int  is_dir = d_type == DT_DIR;

			bpos += d -> d_reclen;

			size_t fpath_len = snprintf(NULL, 0, "%s/%s", folder, d -> d_name);
			char fpath_buffer[fpath_len + 1];
			(void) snprintf(fpath_buffer, fpath_len + 1, "%s/%s", folder, d -> d_name);

			char* fpath = folder_is_blank ? d -> d_name : fpath_buffer;

			if (recursive && (strcmp(d -> d_name, ".") == 0 || strcmp(d -> d_name, "..") == 0 || strcmp(d -> d_name, ".git") == 0)) {
				continue;
			}

			if (recursive && is_dir) {
				n_entries += iter_folder_contents(fpath, 1, fx_to_exec_content, data_to_pass);
				continue;
			}

			if (recursive && d_type != DT_REG) {
				continue;
			}

			int r = fx_to_exec_content == NULL ? 0 : fx_to_exec_content(fpath, d, d_type, data_to_pass);
			if (r != 0) {
				(void) close(fd);
				return n_entries;
			}

			n_entries++;
		}
	}

	(void) close(fd);
	return n_entries;
}

char* pkg_cache_folder = "__pkg_cache__";
struct __list_pkgbases_args {
	char** pkgbases;
	size_t pkgbases_limit;
	char*  entries_stash;
	size_t entries_stash_limit;
	size_t pkgbases_counter;
	size_t entries_stash_counter;
};

static int __write_pkgbase_names(const char* fname, const struct linux_dirent* d, char d_type, void* data_passed) {
	const char*  pkgbase_name = d -> d_name;
	size_t pkgbase_name_len = strlen(pkgbase_name);

	if (d_type != DT_DIR || strcmp(pkgbase_name, ".") == 0 || strcmp(pkgbase_name, "..") == 0 || strcmp(pkgbase_name, "__completions__") == 0) {
		return 0;
	}

	struct __list_pkgbases_args* args = (struct __list_pkgbases_args*) data_passed;
	char** pkgbases            = args -> pkgbases;
	size_t pkgbases_limit      = args -> pkgbases_limit;
	char*  entries_stash       = args -> entries_stash;
	size_t entries_stash_limit = args -> entries_stash_limit;
	size_t i = args -> pkgbases_counter;
	size_t j = args -> entries_stash_counter;

	if (pkgbases != NULL && entries_stash != NULL && i < pkgbases_limit) {
		pkgbases[i] = entries_stash + j;
		if (j + pkgbase_name_len < entries_stash_limit) {
			(void) memcpy(pkgbases[i], pkgbase_name, (pkgbase_name_len + 1) * sizeof(char));
		}
	}
	i++;
	j += pkgbase_name_len + 1;

	args -> pkgbases_counter      = i;
	args -> entries_stash_counter = j;
	return 0;
}

char does_pkg_cache_exist() {
	struct stat sb;
	return stat(pkg_cache_folder, &sb) == 0 && S_ISDIR(sb.st_mode);
}

size_t list_pkgbases(size_t* __restrict__ total_str_alloc, char** pkgbases, size_t pkgbases_limit, char* entries_stash, size_t entries_stash_limit) {
	if (!does_pkg_cache_exist()) {
		return 0;
	}

	struct __list_pkgbases_args args = {
		.pkgbases              = pkgbases,
		.pkgbases_limit        = pkgbases_limit,
		.entries_stash         = entries_stash,
		.entries_stash_limit   = entries_stash_limit,
		.pkgbases_counter      = 0,
		.entries_stash_counter = 0
	};

	size_t n_dir_entries = iter_folder_contents(pkg_cache_folder, 0, __write_pkgbase_names, &args);

	if (total_str_alloc != NULL)
		*total_str_alloc = args.entries_stash_counter;

	return args.pkgbases_counter;
}

int main(void) {
	size_t total_str_alloc;
	size_t n_pkgs = list_pkgbases(&total_str_alloc, NULL, 0, NULL, 0);

	char  stash[total_str_alloc];
	char* pkgbases[n_pkgs];

	(void) list_pkgbases(NULL, pkgbases, n_pkgs, stash, total_str_alloc);

	for (size_t i = 0; i < n_pkgs; i++) {
		(void) printf("%s\n", pkgbases[i]);
	}

	return 0;
}
