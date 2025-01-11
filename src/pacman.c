#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
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

/**
 * pacman_output is only mutated if pacman_objs_out is not NULL.
 */
static size_t process_pacman_output(pacman_name_ver_t* __restrict__ pacman_objs_out, size_t pacman_objs_size, char* pacman_output, size_t pacman_output_len) {
	char delim[] = "\n";

	size_t n_tokens = 0;

	for (size_t i = 0; i < pacman_output_len; i++) {
		if (pacman_output[i] == '\n') {
			n_tokens++;
		}

		if (pacman_output[i] == ' ') {
		}
	}
	n_tokens++;

	if (pacman_objs_out == NULL) {
		return n_tokens;
	}

	size_t n_pkgs = 0;
	char* token = strtok(pacman_output, delim);
	for (size_t i = 0; token; i++, token = strtok(NULL, delim)) {
		pacman_objs_out[i].valid = 0;
		char* next_space = strchr(token, ' ');
		if (next_space == NULL) {
			continue;
		}
		*next_space = '\0';
		pacman_objs_out[i].name    = token;
		pacman_objs_out[i].version = next_space + 1;
		pacman_objs_out[i].valid   = 1;
		n_pkgs++;
	}

	return n_pkgs;
}

pacman_names_vers_t ret_list = {
	.pkg_names_vers = NULL,
	.n_items        = 0
};

pacman_names_vers_t get_installed_non_pacman() {
	int pacman_pipe_fds[2];

	if (pipe(pacman_pipe_fds) < 0) {
		(void) fprintf(stderr, "[ERROR][%s:%d]: %s\n", __FILE__, __LINE__ - 1, strerror(errno));
		return PACMAN_PKGS_NULL_RET;
	}

	int pacman_child_proc = fork();

	if (pacman_child_proc == 0) {
		if (close(pacman_pipe_fds[0]) < 0) {
			(void) fprintf(stderr, "[ERROR][%s:%d]: %s\n", __FILE__, __LINE__ - 1, strerror(errno));
			return PACMAN_PKGS_NULL_RET;
		}

		if (dup2(pacman_pipe_fds[1], STDOUT_FILENO) < 0) {
			(void) fprintf(stderr, "[ERROR][%s:%d]: %s\n", __FILE__, __LINE__ - 1, strerror(errno));
			return PACMAN_PKGS_NULL_RET;
		}

		if (close(pacman_pipe_fds[1]) < 0) {
			(void) fprintf(stderr, "[ERROR][%s:%d]: %s\n", __FILE__, __LINE__ - 1, strerror(errno));
			return PACMAN_PKGS_NULL_RET;
		}
		
		if (execl("/usr/bin/pacman", "pacman", "-Qm", (char *) NULL) < 0) {
			(void) fprintf(stderr, "[ERROR][%s:%d]: %s\n", __FILE__, __LINE__ - 1, strerror(errno));
			return PACMAN_PKGS_NULL_RET;
		}
		return PACMAN_PKGS_NULL_RET;
	} else if (pacman_child_proc == -1) {
		(void) fprintf(stderr, "[ERROR][%s:%d]: %s\n", __FILE__, __LINE__ - 1, strerror(errno));
		return PACMAN_PKGS_NULL_RET;
	} else {
		if (close(pacman_pipe_fds[1]) < 0) {
			(void) fprintf(stderr, "[ERROR][%s:%d]: %s\n", __FILE__, __LINE__ - 1, strerror(errno));
			return PACMAN_PKGS_NULL_RET;
		}

		int pacman_exit_stat;
		if (waitpid(pacman_child_proc, &pacman_exit_stat, 0) < 0) {
			(void) fprintf(stderr, "[ERROR][%s:%d]: %s\n", __FILE__, __LINE__ - 1, strerror(errno));
			return PACMAN_PKGS_NULL_RET;
		}

		int pacman_ret_code = WEXITSTATUS(pacman_exit_stat);

		if (!WIFEXITED(pacman_exit_stat)) {
			(void) fprintf(stderr, "[WARNING] pacman exited with code %d.\n", pacman_ret_code);
		}

		size_t pacman_out_len = 0;
		char* pacman_out = record_pacman_output(pacman_pipe_fds[0], &pacman_out_len);

		if (pacman_out == NULL) {
			return PACMAN_PKGS_NULL_RET;
		}

		// if (ret_list == NULL) {
		// 	ret_list = malloc(sizeof(pacman_names_vers_t));
		// 	if (ret_list == NULL) {
		// 		(void) fprintf(stderr, "[ERROR][%s:%d]: %s\n", __FILE__, __LINE__ - 1, strerror(errno));
		// 		return PACMAN_PKGS_NULL_RET;
		// 	}
		// 	ret_list -> n_items        = 0;
		// 	ret_list -> pkg_names_vers = NULL;
		// }

		ret_list.n_items = process_pacman_output(NULL, 0, pacman_out, pacman_out_len);

		if (ret_list.pkg_names_vers == NULL) {
			ret_list.pkg_names_vers = malloc(ret_list.n_items * sizeof(pacman_name_ver_t));
		} else {
			ret_list.pkg_names_vers = realloc(ret_list.pkg_names_vers, ret_list.n_items * sizeof(pacman_name_ver_t));
		}

		ret_list.n_items = process_pacman_output(ret_list.pkg_names_vers, ret_list.n_items, pacman_out, pacman_out_len);

		// ret_list -> pkg_names_vers = realloc(ret_list -> pkg_names_vers, ret_list -> n_items * sizeof(pacman_name_ver_t));

		// for (size_t i = 0; i < ret_list.n_items; i++) {
		// 	if (ret_list.pkg_names_vers[i].valid) {
				// (void) printf("{.name = \"%s\", .version = \"%s\"}\n", pkgs[i].name, pkgs[i].version);
			// }
		// }

		return ret_list;
	}
}

void clean_up_pacman_output(void) {
	if (ret_list.pkg_names_vers != NULL) {
		free(ret_list.pkg_names_vers);
	}

	if (pacman_output != NULL) {
		free(pacman_output);
		pacman_output      = NULL;
		pacman_output_size = 0;
	}
}
