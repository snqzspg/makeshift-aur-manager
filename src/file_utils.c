#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "file_utils.h"

streamed_content_t STR_CT_NULL_RET = {.content = NULL, .len = 0};

#define FRAG_SIZE 16384
streamed_content_t stream_fd_content_alloc(int fd) {
	char* content = (char*) malloc(FRAG_SIZE * sizeof(char));
	size_t content_size = FRAG_SIZE;

	if (content == NULL) {
		perror("[ERROR][stream_fd_content_alloc]");
		return STR_CT_NULL_RET;
	}

	size_t total_file_size = 0;
	char fragment[FRAG_SIZE];
	int n_read;

	do {
		n_read = read(fd, fragment, FRAG_SIZE);
		if (n_read < 0) {
			(void) fprintf(stderr, "[ERROR][stream_fd_content_alloc] \033[1;31mReading fd failed\033[0m! - %s\n", strerror(errno));
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
				perror("[ERROR][stream_fd_content_alloc]");
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
	content = (char *) realloc(content, content_size * sizeof(char));
	if (content == NULL) {
		perror("[ERROR][stream_fd_content_alloc]");
		return STR_CT_NULL_RET;
	}

	return (streamed_content_t) {
		.content = content,
		.len     = content_size
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
		(void) fprintf(stderr, "[ERROR] \033[1;31mOpening %s failed\033[0m! - %s\n", path, strerror(errno));
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
			(void) fprintf(stderr, "[ERROR] \033[1;31mReading %s failed\033[0m! - %s\n", path, strerror(errno));
			return -1;
		}

		total_file_size += n_read;

		if (dest != NULL && total_file_size < limit) {
			(void) strncat(dest, fragment, limit);
		}
	} while (n_read != 0);

	if (close(fd) < 0) {
		(void) fprintf(stderr, "[ERROR] \033[1;31mClosing %s failed\033[0m! - %s\n", path, strerror(errno));
	}

	return (int) total_file_size;
}
