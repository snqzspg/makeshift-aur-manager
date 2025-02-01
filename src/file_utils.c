#define _GNU_SOURCE

#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "file_utils.h"
#include "logger/logger.h"

streamed_content_t STR_CT_NULL_RET = {.content = NULL, .len = 0};

#define FRAG_SIZE 16384
streamed_content_t stream_fd_content_alloc(int fd) {
	char* content = (char*) malloc(FRAG_SIZE * sizeof(char));
	size_t content_size = FRAG_SIZE;

	if (content == NULL) {
		error_perror("[stream_fd_content_alloc]");
		return STR_CT_NULL_RET;
	}

	size_t total_file_size = 0;
	char fragment[FRAG_SIZE];
	int n_read;

	do {
		n_read = read(fd, fragment, FRAG_SIZE);
		if (n_read < 0) {
			(void) error_printf("[stream_fd_content_alloc] \033[1;31mReading fd failed\033[0m! - %s\n", strerror(errno));
			free(content);
			return STR_CT_NULL_RET;
		}

		if (n_read == 0) {
			break;
		}

		if (content_size < total_file_size + n_read + 1) {
			while (content_size < total_file_size + n_read + 1) {
				content_size <<= 1;
			}
			content = (char *) realloc(content, content_size * sizeof(char));
			if (content == NULL) {
				error_perror("[stream_fd_content_alloc]");
				return STR_CT_NULL_RET;
			}
		}

		for (int i = 0; i < n_read; i++) {
			content[total_file_size + i] = fragment[i];
		}

		total_file_size += n_read;

		content[total_file_size] = '\0';
	} while (n_read != 0);

	content_size = total_file_size;
	if (content_size == 0) {
		free(content);
		return STR_CT_NULL_RET;
	}
	content_size++;
	content = (char *) realloc(content, content_size * sizeof(char));
	if (content == NULL) {
		error_perror("[stream_fd_content_alloc]");
		return STR_CT_NULL_RET;
	}

	return (streamed_content_t) {
		.content = content,
		.len     = content_size - 1
	};
}

void stream_fd_content_dealloc(streamed_content_t* s) {
	free(s -> content);
	s -> content = NULL;
	s -> len     = 0;
}

int load_file_contents(char* __restrict__ dest, size_t limit, const char* path) {
	int fd = open(path, O_RDONLY);

	if (fd < 0) {
		(void) error_printf(" \033[1;31mOpening %s failed\033[0m! - %s\n", path, strerror(errno));
		return -1;
	}

	size_t total_file_size = 0;

	char fragment[FRAG_SIZE];

	int n_read;

	if (dest != NULL && total_file_size < limit) {
		(void) memset(dest, 0, limit * sizeof(char));
	}

	do {
		n_read = read(fd, fragment, FRAG_SIZE);
		if (n_read < 0) {
			(void) error_printf(" \033[1;31mReading %s failed\033[0m! - %s\n", path, strerror(errno));
			return -1;
		}

		total_file_size += n_read;

		if (dest != NULL && total_file_size < limit) {
			(void) strncat(dest, fragment, limit);
		}
	} while (n_read != 0);

	if (close(fd) < 0) {
		(void) error_printf(" \033[1;31mClosing %s failed\033[0m! - %s\n", path, strerror(errno));
	}

	return (int) total_file_size;
}

int file_exists(const char* fname) {
    struct stat sb;

    int r = stat(fname, &sb);
    if (r >= 0) {
        return 1;
    }
    if (errno == ENOENT) {
        return 0;
    }
    (void) warning_perror("[file_exists] Error occured");
    return 0;
}

#define analyze_size 8192
/**
 * Checking if a file is text or binary according to the method
 * specified by GNU diff.
 * 
 * @param fd A unix file descriptor of the opened file.access
 * @return 0 for binary, 1 for text file.
 */
int is_text_file(int fd) {
	char to_analyze[analyze_size];
	
	int n_chars_in = read(fd, to_analyze, analyze_size);
	if (n_chars_in < 0) {
		error_perror("[checking_is_text_or_binary]");
		return 0;
	}
	if (n_chars_in == 0)
		return 0;
	
	for (int i = 0; i < n_chars_in; i++) {
		if (to_analyze[i] == '\0') {
			return 0;
		}
	}
	return 1;
}

struct linux_dirent {
	long           d_ino;
	off_t          d_off;
	unsigned short d_reclen;
	char           d_name[];
};

#define RDIR_FRAG_SIZE 4096

/**
 * A very Linux specific method to get all files for a directory.
 * It returns the files and folders in order of inodes.
 * 
 * @param dest Nullable, the output array of filenames to write to.
 * @param len_outs Nullable, an array of size_t where the lengths of the folder names are being written into.
 * @param is_dir_outs Nullable, an array of boolean values to be written to, which is true if the corresponding folder name is a directory.
 * @param dest_limit The size of the arrays. All arrays must have the same size. This is so that len_outs[0] is the length of dest[0], and is_dir_outs[0] tells if dest[0] is a directory.
 * @param folder The path to the folder to list the files.
 * @param recursive List all files inside the subfolders.
 * 
 * @return The number of entries inside this folder.
 */
size_t write_dir_filenames(char** dest, size_t* len_outs, char* is_dir_outs, size_t dest_limit, const char* folder, int recursive) {
	int fd = open(folder, O_RDONLY | O_DIRECTORY);

	if (fd == -1) {
		(void) error_printf(" Opening %s failed!: %s\n", folder, strerror(errno));
		return 0;
	}

	char fragment[RDIR_FRAG_SIZE];
	size_t n_entries = 0;

	for (long n_read = syscall(SYS_getdents, fd, fragment, RDIR_FRAG_SIZE); n_read != 0; n_read = syscall(SYS_getdents, fd, fragment, RDIR_FRAG_SIZE)) {
		if (n_read == -1) {
			error_perror("[getdents]");
			return 0;
		}

		for (int bpos = 0; bpos < n_read;) {
			struct linux_dirent* d = (struct linux_dirent*) (fragment + bpos);
			int d_type = *(fragment + bpos + d -> d_reclen - 1);
			int is_dir = d_type == DT_DIR;

			bpos += d -> d_reclen;

			size_t fpath_len = snprintf(NULL, 0, "%s/%s", folder, d -> d_name);
			char fpath[fpath_len + 1];
			(void) snprintf(fpath, fpath_len + 1, "%s/%s", folder, d -> d_name);

			if (recursive && (strcmp(d -> d_name, ".") == 0 || strcmp(d -> d_name, "..") == 0)) {
				continue;
			}

			if (recursive && is_dir) {
				char**  dest1        = dest        == NULL || dest_limit < n_entries ? NULL : dest        + n_entries;
				size_t* len_outs1    = len_outs    == NULL || dest_limit < n_entries ? NULL : len_outs    + n_entries;
				char*   is_dir_outs1 = is_dir_outs == NULL || dest_limit < n_entries ? NULL : is_dir_outs + n_entries;
				n_entries += write_dir_filenames(dest1, len_outs1, is_dir_outs1, dest_limit <= n_entries? 0 : dest_limit - n_entries, fpath, 1);
				continue;
			}

			if (d_type != DT_REG) {
				continue;
			}

			if (n_entries < dest_limit) {
				if (is_dir_outs != NULL) {
					is_dir_outs[n_entries] = is_dir;
				}

				if (len_outs != NULL) {
					len_outs[n_entries] = fpath_len;
				}

				if (dest != NULL) {
					(void) strncpy(dest[n_entries], fpath, fpath_len + 1);
				}
			}
			n_entries++;
		}
	}

	(void) close(fd);

	return n_entries;
}
