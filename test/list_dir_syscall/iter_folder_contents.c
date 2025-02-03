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

struct __write_dir_filenames_args {
	size_t* p_names_stash_count;
	char**  folder_names_out;
	size_t  folder_names_limit;
	char*   names_stash;
	size_t  names_stash_limit;
	size_t  folder_names_count;
};

static int __accumulate_total_strlens(const char* fname, const struct linux_dirent* d, char d_type, void* data_passed) {
	size_t fname_len = strlen(fname);
	struct __write_dir_filenames_args* args = (struct __write_dir_filenames_args*) data_passed;
	if (args -> folder_names_out != NULL && args -> names_stash != NULL && args -> folder_names_count < args -> folder_names_limit) {
		args -> folder_names_out[args -> folder_names_count] = args -> names_stash + *(args -> p_names_stash_count);
		if (*(args -> p_names_stash_count) + fname_len < args -> names_stash_limit) {
			(void) memcpy(args -> folder_names_out[args -> folder_names_count], fname, (fname_len + 1) * sizeof(char));
		}
	}
	args -> folder_names_count++;
	*(args -> p_names_stash_count) += fname_len + 1;
	return 0;
}

size_t write_dir_filenames(size_t* total_alloc_len, char** folder_names_out, size_t folder_names_limit, char* names_stash, size_t names_stash_limit, const char* folder, int recursive) {
	*total_alloc_len = 0;
	struct __write_dir_filenames_args args = {
		.p_names_stash_count = total_alloc_len,
		.folder_names_out    = folder_names_out,
		.folder_names_limit  = folder_names_limit,
		.names_stash         = names_stash,
		.names_stash_limit   = names_stash_limit,
		.folder_names_count  = 0
	};

	size_t n_entries = iter_folder_contents(folder, recursive, __accumulate_total_strlens, &args);

	// if (dest != NULL) {
	// 	qsort(dest, dest_limit < n_entries ? dest_limit : n_entries, sizeof(char*), alph_cmp);
	// }

	return n_entries;
}

int print_full_name(const char* full_name, const struct linux_dirent* d, char d_type, void* data_to_modify) {
	(void) printf("%s%s%s\n", d_type == DT_DIR ? "\033[1;34m" : "", full_name, d_type == DT_DIR ? "\033[0m" : "");
	return 0;
}

int main(int argc, char** argv) {
	char* fdr = argc > 1 ? argv[1] : NULL;

	// size_t n_entries = iter_folder_contents(fdr, 1, NULL, NULL);
	// (void) fprintf(stderr, "[\033[1;33mDEBUG\033[0m] %zu\n", n_entries);

	// iter_folder_contents(fdr, 1, print_full_name, NULL);

	size_t total_alloc_len;
	size_t n_entries = write_dir_filenames(&total_alloc_len, NULL, 0, NULL, 0, fdr, 0);
	(void) fprintf(stderr, "[\033[1;33mDEBUG\033[0m] %zu\n", n_entries);

	char stash[total_alloc_len];
	char* entries[n_entries];

	write_dir_filenames(&total_alloc_len, entries, n_entries, stash, total_alloc_len, fdr, 0);

	for (size_t i = 0; i < n_entries; i++) {
		(void) printf("%s\n", entries[i]);
	}

	return 0;
}
