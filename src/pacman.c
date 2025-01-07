#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include "pacman.h"

char*  pacman_output      = NULL;
size_t pacman_output_size = 0;

static char* record_pacman_output(int in_fd, size_t* __restrict__ n_chars_recorded_out) {
	if (pacman_output == NULL) {
		pacman_output = (char *) malloc(1024);
		if (pacman_output == NULL) {
			(void) fprintf(stderr, "[ERROR][%s:%d]: %s\n", __FILE__, __LINE__ - 1, strerror(errno));
			return NULL;
		}
	}

	size_t n_chars_read = 0;

	char fragment[1024];

	int chars_read;
	
	while (1) {
		chars_read = read(in_fd, fragment, 1024);

		if (chars_read == 0) {
			break;
		}

		if (chars_read < 0) {
			(void) fprintf(stderr, "[ERROR][%s:%d]: %s\n", __FILE__, __LINE__, strerror(errno));
			break;
		}

		if (pacman_output_size - n_chars_read < 1024) {
			if (pacman_output_size < 1024) {
				pacman_output_size = 2048;
			} else {
				pacman_output_size *= 2;
			}

			pacman_output = (char *) realloc(pacman_output, pacman_output_size);
		}

		(void) memcpy(pacman_output + n_chars_read, fragment, chars_read);

		n_chars_read += chars_read;
	}

	pacman_output[n_chars_read] = '\0';
	*n_chars_recorded_out = n_chars_read;
	return pacman_output;
}

pacman_names_vers_t get_installed_non_pacman() {
	int pacman_pipe_fds[2];

	if (pipe(pacman_pipe_fds) < 0) {
		(void) fprintf(stderr, "[ERROR][%s:%d]: %s\n", __FILE__, __LINE__ - 1, strerror(errno));
		return NULL_RET;
	}

	int pacman_child_proc = fork();

	if (pacman_child_proc == 0) {
		if (close(pacman_pipe_fds[0]) < 0) {
			(void) fprintf(stderr, "[ERROR][%s:%d]: %s\n", __FILE__, __LINE__ - 1, strerror(errno));
			return NULL_RET;
		}

		if (dup2(pacman_pipe_fds[1], STDOUT_FILENO) < 0) {
			(void) fprintf(stderr, "[ERROR][%s:%d]: %s\n", __FILE__, __LINE__ - 1, strerror(errno));
			return NULL_RET;
		}

		if (close(pacman_pipe_fds[1]) < 0) {
			(void) fprintf(stderr, "[ERROR][%s:%d]: %s\n", __FILE__, __LINE__ - 1, strerror(errno));
			return NULL_RET;
		}
		
		if (execl("/usr/bin/pacman", "pacman", "-Qm", (char *) NULL) < 0) {
			(void) fprintf(stderr, "[ERROR][%s:%d]: %s\n", __FILE__, __LINE__ - 1, strerror(errno));
			return NULL_RET;
		}
	} else if (pacman_child_proc == -1) {
		(void) fprintf(stderr, "[ERROR][%s:%d]: %s\n", __FILE__, __LINE__ - 1, strerror(errno));
		return NULL_RET;
	} else {
		if (close(pacman_pipe_fds[1]) < 0) {
			(void) fprintf(stderr, "[ERROR][%s:%d]: %s\n", __FILE__, __LINE__ - 1, strerror(errno));
			return NULL_RET;
		}

		int pacman_exit_stat;
		if (waitpid(pacman_child_proc, &pacman_exit_stat, 0) < 0) {
			(void) fprintf(stderr, "[ERROR][%s:%d]: %s\n", __FILE__, __LINE__ - 1, strerror(errno));
			return NULL_RET;
		}

		int pacman_ret_code = WEXITSTATUS(pacman_exit_stat);

		if (!WIFEXITED(pacman_exit_stat)) {
			(void) fprintf(stderr, "[WARNING] pacman exited with code %d.\n", pacman_ret_code);
		}

		size_t pacman_out_len = 0;
		char* pacman_out = record_pacman_output(pacman_pipe_fds[0], &pacman_out_len);

		if (pacman_out == NULL) {
			return NULL_RET;
		}

		(void) printf("%s\n", pacman_out);

		free(pacman_out);
	}
}
