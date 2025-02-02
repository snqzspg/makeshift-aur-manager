#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "arg_parse/arg_commands.h"
#include "file_utils.h"
#include "hashtable.h"
#include "logger/logger.h"
#include "pacman.h"
#include "unistd_helper.h"

#include "aur/pkg_install_stages.h"

char*  pacman_output      = NULL;
size_t pacman_output_size = 0;

pacman_names_vers_t PACMAN_PKGS_NULL_RET = {
    .n_items        = 0,
    .pkg_names_vers = NULL
};

static char* record_pacman_output(int in_fd, size_t* __restrict__ n_chars_recorded_out) {
	if (pacman_output == NULL) {
		pacman_output = (char *) malloc(1024);
		if (pacman_output == NULL) {
			(void) error_printf("[%s:%d]: %s\n", __FILE__, __LINE__ - 1, strerror(errno));
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
			(void) error_printf("[%s:%d]: %s\n", __FILE__, __LINE__, strerror(errno));
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
size_t process_pacman_output(pacman_name_ver_t* __restrict__ pacman_objs_out, size_t pacman_objs_size, char* pacman_output, size_t pacman_output_len) {
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

	run_syscall_print_err_w_act(pipe(pacman_pipe_fds), return PACMAN_PKGS_NULL_RET;, error_printf, __FILE__, __LINE__);

	int pacman_child_proc = fork();

	if (pacman_child_proc == 0) {
		run_syscall_print_err_w_act(close(pacman_pipe_fds[0]), ;, warning_printf, __FILE__, __LINE__);
		run_syscall_print_err_w_act(dup2(pacman_pipe_fds[1], STDOUT_FILENO), close(pacman_pipe_fds[1]); _exit(EXIT_FAILURE); return PACMAN_PKGS_NULL_RET;, error_printf, __FILE__, __LINE__);
		run_syscall_print_err_w_act(close(pacman_pipe_fds[1]), ;, warning_printf, __FILE__, __LINE__);

		run_syscall_print_err_w_act(execl("/usr/bin/pacman", "pacman", "-Qm", (char *) NULL),
			_exit(EXIT_FAILURE);
			return PACMAN_PKGS_NULL_RET;
		, error_printf, __FILE__, __LINE__);

		_exit(EXIT_FAILURE);
		return PACMAN_PKGS_NULL_RET;
	} else if (pacman_child_proc == -1) {
		(void) error_printf("[%s:%d]: %s\n", __FILE__, __LINE__ - 1, strerror(errno));
		return PACMAN_PKGS_NULL_RET;
	} else {
		run_syscall_print_err_w_act(close(pacman_pipe_fds[1]), ;, warning_printf, __FILE__, __LINE__);

		int pacman_exit_stat;
		run_syscall_print_err_w_act(waitpid(pacman_child_proc, &pacman_exit_stat, 0), (void) close(pacman_pipe_fds[0]); return PACMAN_PKGS_NULL_RET;, error_printf, __FILE__, __LINE__);

		int pacman_ret_code = WEXITSTATUS(pacman_exit_stat);

		if (!WIFEXITED(pacman_exit_stat)) {
			(void) warning_printf(" pacman exited with code %d.\n", pacman_ret_code);
		}

		size_t pacman_out_len = 0;
		char* pacman_out = record_pacman_output(pacman_pipe_fds[0], &pacman_out_len);

		if (pacman_out == NULL) {
			return PACMAN_PKGS_NULL_RET;
		}

		ret_list.n_items = process_pacman_output(NULL, 0, pacman_out, pacman_out_len);

		if (ret_list.pkg_names_vers == NULL) {
			ret_list.pkg_names_vers = malloc(ret_list.n_items * sizeof(pacman_name_ver_t));
		} else {
			ret_list.pkg_names_vers = realloc(ret_list.pkg_names_vers, ret_list.n_items * sizeof(pacman_name_ver_t));
		}

		ret_list.n_items = process_pacman_output(ret_list.pkg_names_vers, ret_list.n_items, pacman_out, pacman_out_len);

		// ret_list -> pkg_names_vers = realloc(ret_list -> pkg_names_vers, ret_list -> n_items * sizeof(pacman_name_ver_t));

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

int perform_pacman_checkupdates() {
	int out_fds[2];

	int pipe_failed = 0;

	run_syscall_print_w_act(pipe(out_fds), pipe_failed = 1;, "NOTE", __FILE__, __LINE__);

	int check_upd_child = fork();

	if (check_upd_child == 0) {
		// (void) fprintf(stderr, "--- \033[1;32mExecuting /usr/bin/checkupdates\033[0m ---\n");
		if (!pipe_failed) {
			run_syscall_print_err_w_act(close(out_fds[0]), ;, warning_printf, __FILE__, __LINE__);
			run_syscall_print_err_w_act(dup2(out_fds[1], STDOUT_FILENO), close(out_fds[1]); pipe_failed = 1;, warning_printf, __FILE__, __LINE__);
			run_syscall_print_err_w_act(close(out_fds[1]), ;, warning_printf, __FILE__, __LINE__);
		}

		if (execl("/usr/bin/checkupdates", "checkupdates", NULL) < 0) {
			(void) note_printf(" /usr/bin/checkupdates \033[1;31mexecution failed!\033[0m\n");
			(void) note_printf(" %s\n", strerror(errno));
			return errno;
		}
		return -1;
	} else if (check_upd_child == -1) {
		if (!pipe_failed) {
			(void) close(out_fds[0]);
			(void) close(out_fds[1]);
		}
		(void) error_printf("[%s:%d]: %s\n", __FILE__, __LINE__ - 1, strerror(errno));
		return -1;
	} else {
		if (!pipe_failed) {
			run_syscall_print_err_w_act(close(out_fds[1]), ;, warning_printf, __FILE__, __LINE__);
		}

		int checkupd_exit_stat;
		run_syscall_print_err_w_act(waitpid(check_upd_child, &checkupd_exit_stat, 0), close(out_fds[0]); return -1;, warning_printf, __FILE__, __LINE__);

		if (!pipe_failed && WEXITSTATUS(checkupd_exit_stat) == 0) {
			streamed_content_t updates_output = stream_fd_content_alloc(out_fds[0]);

			(void) write(STDOUT_FILENO,
				"\n--- \033[1;34mPacman Updates\033[0m ---\n"
				"\033[1;34mRun '\033[1;32msudo pacman -Syu\033[1;34m' (preferred) or use '\033[1;32m"
			, 106);
			(void) write(STDOUT_FILENO, exec_arg0, strlen(exec_arg0));
			(void) write(STDOUT_FILENO,
				" pacman-upgrade\033[1;34m' using this application to update.\033[0m\n\n"
			, 63);
			(void) write(STDOUT_FILENO, updates_output.content, updates_output.len);
			(void) write(STDOUT_FILENO, "\n", 1);

			stream_fd_content_dealloc(&updates_output);
		}

		return WEXITSTATUS(checkupd_exit_stat);
	}
}

int perform_pacman_upgrade(int argc, char** argv) {
	int n_pars = argc + 4;
	char* exec_pars[n_pars];
	exec_pars[0] = "sudo";
	exec_pars[1] = "pacman";
	exec_pars[2] = "-Syu";
	for (int i = 0, i1 = 3; i < argc; i++, i1++) {
		exec_pars[i1] = argv[i];
	}
	exec_pars[n_pars - 1] = NULL;

	int check_upd_child = fork();

	if (check_upd_child == 0) {
		(void) fprintf(stderr, "--- \033[32mExecuting /usr/bin/sudo ");
		for (int i = 1; i < n_pars - 1; i++) {
			(void) fprintf(stderr, "%s ", exec_pars[i]);
		}
		(void) fprintf(stderr, "\033[39;49m---\n");

		if (execv("/usr/bin/sudo", exec_pars) < 0) {
			(void) note_printf(" /usr/bin/sudo \033[31mexecution failed!\033[39;49m\n");
			(void) note_printf(" %s\n", strerror(errno));
			return errno;
		}
		return -1;
	} else if (check_upd_child == -1) {
		(void) error_printf("[%s:%d]: %s\n", __FILE__, __LINE__ - 1, strerror(errno));
		return -1;
	} else {
		int pacman_exit_stat;
		run_syscall_print_err_w_act(waitpid(check_upd_child, &pacman_exit_stat, 0), return errno;, error_printf, __FILE__, __LINE__);

		return WEXITSTATUS(pacman_exit_stat);
	}
}

// int compare_versions(const char* ver1, const char* ver2) {
// 	int vercmp_fds[2];

// 	run_syscall_print_err_w_act(pipe(vercmp_fds), return 0;, warning_printf, __FILE__, __LINE__);

// 	int pacman_child = fork();

// 	if (pacman_child == -1) {
// 		(void) error_printf("[%s:%d]: %s\n", __FILE__, __LINE__ - 1, strerror(errno));
// 		return 0;
// 	} else if (pacman_child == 0) {
// 		run_syscall_print_err_w_act(close(vercmp_fds[0]), ;, warning_printf, __FILE__, __LINE__);
// 		run_syscall_print_err_w_act(dup2(vercmp_fds[1], STDOUT_FILENO), (void) close(vercmp_fds[1]); _exit(EXIT_FAILURE); return 0;, error_printf, __FILE__, __LINE__);
// 		run_syscall_print_err_w_act(close(vercmp_fds[1]), ;, warning_printf, __FILE__, __LINE__);

// 		run_syscall_print_err_w_act(execl("/usr/bin/vercmp", "vercmp", ver1, ver2, NULL), ;, error_printf, __FILE__, __LINE__);
// 		_exit(EXIT_FAILURE);
// 		return 0;
// 	} else {
// 		run_syscall_print_err_w_act(close(vercmp_fds[1]), ;, warning_printf, __FILE__, __LINE__);

// 		int exit_status;
// 		run_syscall_print_err_w_act(waitpid(pacman_child, &exit_status, 0), (void) close(vercmp_fds[0]); return 0;, error_printf, __FILE__, __LINE__);

// 		int ret_code = WEXITSTATUS(exit_status);

// 		if (!WIFEXITED(exit_status)) {
// 			(void) warning_printf(" pacman exited with code %d.\n", ret_code);
// 		}

// 		char vercmp_out[21];
// 		int n_txferred = read(vercmp_fds[0], vercmp_out, 21);

// 		if (n_txferred == 0) {
// 			(void) warning_printf(" vercmp did not print anything");
// 			return 0;
// 		}

// 		if (n_txferred < 0) {
// 			(void) warning_printf("[%s:%d]: %s\n", __FILE__, __LINE__ - 8, strerror(errno));
// 			return 0;
// 		}

// 		if (n_txferred < 21) {
// 			vercmp_out[n_txferred] = '\0';
// 		} else {
// 			vercmp_out[20] = '\0';
// 		}

// 		return atoi(vercmp_out);
// 	}
// }
