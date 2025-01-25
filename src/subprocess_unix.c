#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "logger/logger.h"
#include "unistd_helper.h"

/**
 * Runs a subprocess by using UNIX syscalls.
 * 
 * @param pwd The directory to change to before running subprocess
 * @param exec_path The absolute path to the executable
 * @param args An array of arguments to be supplied, including argv0
 *             The last item should be NULL
 * @param stdout_fd An output which the stdout pipe fd will be placed.
 *                  Read from this to get the stdout string.
 * @param stdin_fd_in Supply an alternative file descripter for the subprocess to read as stdin.
 *                    Should supply STDIN_FILENO for default stdin.
 *                    Custom supplied stdin_fd will get closed in the main process.
 * @param fds_to_close FDs to close for the child process.
 *                     The last item should be 0 or negative number.
 * @param quiet Set this non-zero to silence execution information
 * @param wait_exit Set this to non-zero for the function to wait for subprocess to complete.
 *                  A zero value means the function will return with the subprocess still running.
 * 
 * @return An integer: return code of subprocess if wait_exit is true, pid of child if wait_exit
 *         is false. On error it will give -1. To clean Zombie processes, call UNIX wait for the
 *         child pid.
 */
int run_subprocess_v(const char* pwd, const char* exec_path, char* const args[], int* __restrict__ stdout_fd, int stdin_fd_in, int* fds_to_close, char quiet, char wait_exit) {
	int pipefds[2];
	if (stdout_fd != NULL) {
		run_syscall_print_w_act(pipe(pipefds), return -EXIT_FAILURE;, "ERROR", __FILE__, __LINE__);
	}

	int subprocess = fork();

	if (subprocess < 0) {
		if (stdout_fd != NULL) {
			(void) close(pipefds[0]);
			(void) close(pipefds[1]);
		}
		(void) error_printf("[%s:%d]: %s\n", __FILE__, __LINE__ - 3, strerror(errno));
		return -EXIT_FAILURE;
	} else if (subprocess == 0) {
		if (stdin_fd_in != STDIN_FILENO && stdin_fd_in >= 0) {
			run_syscall_print_err_w_act(dup2(stdin_fd_in, STDIN_FILENO), 
				if (stdout_fd != NULL) {
					(void) close(pipefds[0]);
					(void) close(pipefds[1]);
				}
				_exit(-EXIT_FAILURE);
				return -EXIT_FAILURE;
			, error_printf, __FILE__, __LINE__);
			run_syscall_print_err_w_act(close(stdin_fd_in), ;, warning_printf, __FILE__, __LINE__);
		}
		if (fds_to_close != NULL) {
			for (int* i = fds_to_close; *i <= 0; i++) {
				run_syscall_print_err_w_act(close(*i), ;, warning_printf, __FILE__, __LINE__);
			}
		}
		if (stdout_fd != NULL) {
			run_syscall_print_err_w_act(close(pipefds[0]), close(pipefds[1]); _exit(-EXIT_FAILURE); return -EXIT_FAILURE;, error_printf, __FILE__, __LINE__);
			run_syscall_print_err_w_act(dup2(pipefds[1], STDOUT_FILENO), close(pipefds[1]); _exit(-EXIT_FAILURE); return -EXIT_FAILURE;, error_printf, __FILE__, __LINE__);
			run_syscall_print_err_w_act(close(pipefds[1]), ;, warning_printf, __FILE__, __LINE__);
		}

		if (pwd != NULL) {
			if (!quiet)
				(void) fprintf(stderr, "--- \033[1;32mChanging pwd for %s to %s\033[0m ---\n", args[0], pwd);

			run_syscall_print_err_w_act(chdir(pwd), _exit(EXIT_FAILURE); return -EXIT_FAILURE;, error_printf, __FILE__, __LINE__);
		}

		if (!quiet) {
			(void) fprintf(stderr, "--- \033[1;32mExecuting %s ", exec_path);
			for (int i = 1; args[i] != NULL; i++) {
				(void) fprintf(stderr, "%s ", args[i]);
			}
			(void) fprintf(stderr, "\033[0m---\n");
		}

		if (execv(exec_path, args) < 0) {
			(void) note_printf(" \033[1;31m%s execution failed!\033[0m\n", exec_path);
			(void) note_printf(" %s\n", strerror(errno));
			_exit(-1);
			return -EXIT_FAILURE;
		}

		_exit(-1);
		return -EXIT_FAILURE;
	} else {
		if (stdin_fd_in != STDIN_FILENO && stdin_fd_in >= 0) {
			run_syscall_print_err_w_act(close(stdin_fd_in), ;, warning_printf, __FILE__, __LINE__);
		}
		if (stdout_fd != NULL) {
			run_syscall_print_err_w_act(close(pipefds[1]), ;, warning_printf, __FILE__, __LINE__);
			*stdout_fd = pipefds[0];
		}

		if (!wait_exit) {
			return subprocess;
		}
		int proc_stat;
		run_syscall_print_w_act(waitpid(subprocess, &proc_stat, 0), return -EXIT_FAILURE;, "ERROR", __FILE__, __LINE__);
	}

	return WEXITSTATUS(wait_exit);
}

/**
 * Runs a subprocess by using UNIX syscalls.
 * 
 * @param pwd The directory to change to before running subprocess
 * @param quiet Set this non-zero to silence execution information
 * @param wait_exit Set this to non-zero for the function to wait for subprocess to complete.
 *                  A zero value means the function will return with the subprocess still running.
 * @param stdout_fd An output which the stdout pipe fd will be placed.
 *                  Read from this to get the stdout string.
 * @param stdin_fd_in Supply an alternative file descripter for the subprocess to read as stdin.
 *                    Should supply STDIN_FILENO for default stdin.
 *                    Custom supplied stdin_fd will get closed in the main process.
 * @param fds_to_close FDs to close for the child process.
 *                     The last item should be 0 or negative number.
 * @param exec_path The absolute path to the executable
 * @param arg An array of arguments to be supplied, including argv0. The last argument should be NULL.
 * 
 * @return An integer: return code of subprocess if wait_exit is true, pid of child if wait_exit
 *         is false. On error it will give -1. To clean Zombie processes, call UNIX wait for the
 *         child pid.
 */
int run_subprocess(const char* pwd, char quiet, char wait_exit, int* __restrict__ stdout_fd, int stdin_fd_in, int* fds_to_close, const char* exec_path, char* arg, ...) {
	va_list args;
	va_list args1;
	va_start(args, arg);

	va_copy(args1, args);

	int count = 0;
	for (char* i = va_arg(args, char*); i != NULL; i = va_arg(args, char*)) {
		count++;
	}

	char* cmd_args[count + 2];

	cmd_args[0] = arg;

	for (int i = 1; i < count + 1; i++) {
		cmd_args[i] = va_arg(args1, char*);
	}

	cmd_args[count + 1] = NULL;

	va_end(args);
	va_end(args1);

	return run_subprocess_v(pwd, exec_path, cmd_args, stdout_fd, stdin_fd_in, fds_to_close, quiet, wait_exit);
}
