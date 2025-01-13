#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../unistd_helper.h"

char* pkg_cache_folder = "__pkg_cache__";
int   pkg_cache_folder_len = 13;

int create_pkg_cache() {
	if (mkdir(pkg_cache_folder, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) < 0) {
		if (errno != EEXIST) {
			(void) fprintf(stderr, "[ERROR][%s:%d]: %s\n", __FILE__, __LINE__ - 2, strerror(errno));
			return -1;
		}
	}
	return 0;
}

char does_pkg_cache_exist() {
	struct stat sb;
	return stat(pkg_cache_folder, &sb) == 0 && S_ISDIR(sb.st_mode);
}

size_t write_pkg_base_path(char* __restrict__ dest, size_t limit, const char* pkg_base) {
	size_t path_len = strlen(pkg_base) + pkg_cache_folder_len + 2;
	
	if (dest == NULL) {
		return path_len - 1;
	}

	(void) strncpy(dest, pkg_cache_folder, limit);
	(void) strncat(dest, "/", limit);
	(void) strncat(dest, pkg_base, limit);

	path_len--;

	return limit < path_len ? limit : path_len;
}

static const char* aur_url_fmt = "https://aur.archlinux.org/%s.git";

int gen_aur_git_url(char* __restrict__ dest, size_t limit, const char* pkg_base) {

	if (dest == NULL) {
		size_t str_size = snprintf(NULL, 0, aur_url_fmt, pkg_base);
		return str_size;
	}

	return snprintf(dest, limit, aur_url_fmt, pkg_base);
}

char pkg_base_folder_exists(const char* pkg_base) {
	struct stat sb;

	size_t path_len = strlen(pkg_base) + pkg_cache_folder_len + 2;

	char path[path_len];

	(void) strncpy(path, pkg_cache_folder, path_len + 1);
	(void) strncat(path, "/", path_len + 1);
	(void) strncat(path, pkg_base, path_len + 1);

	return stat(path, &sb) == 0 && S_ISDIR(sb.st_mode);
}

char pkg_base_in_cache(const char* pkg_base) {
	return pkg_base_folder_exists(pkg_base);
}

int fetch_pkg_base(const char* pkg_base) {
	if (pkg_base_in_cache(pkg_base)) {
		return 0;
	}

	if (create_pkg_cache() < 0) {
		return -1;
	}

	size_t url_len = gen_aur_git_url(NULL, 0, pkg_base);
	char fetch_url[url_len + 1];
	(void) gen_aur_git_url(fetch_url, url_len + 1, pkg_base);

	size_t dest_len = snprintf(NULL, 0, "%s/%s", pkg_cache_folder, pkg_base);
	char dest[dest_len + 1];
	(void) snprintf(dest, dest_len + 1, "%s/%s", pkg_cache_folder, pkg_base);

	int git_subprocess = fork();

	if (git_subprocess == -1) {
		(void) fprintf(stderr, "[ERROR][%s:%d]: %s\n", __FILE__, __LINE__ - 3, strerror(errno));
		return -1;
	} else if (git_subprocess == 0) {
		(void) fprintf(stderr, "--- \033[1;32mExecuting /usr/bin/git clone %s %s\033[0m ---\n", fetch_url, dest);

		if (execl("/usr/bin/git", "git", "clone", fetch_url, dest, NULL) < 0) {
			(void) fprintf(stderr, "[NOTE] \033[1;31m/usr/bin/git execution failed!\033[0m\n");
			(void) fprintf(stderr, "[NOTE] %s\n", strerror(errno));
			return -1;
		}

		return -1;
	} else {
		int git_proc_stat;
		run_syscall_print_err_w_ret(waitpid(git_subprocess, &git_proc_stat, 0), -1, __FILE__, __LINE__);

		if (!WIFEXITED(git_proc_stat)) {
			(void) fprintf(stderr, "[WARNING] git exited with code %d.\n", WEXITSTATUS(git_proc_stat));
			return -1;
		}

		return 0;
	}
}

char is_pkg_in_cache(const char* pkg_name, const char* pkg_base) {
	if (!does_pkg_cache_exist()) {
		return 0;
	}

	if (!pkg_base_in_cache(pkg_base)) {
		return 0;
	}

	return 1;
}

void git_clone_aur_pkgs(char** pkg_bases, size_t n_pkg_bases) {
	for (size_t i = 0; i < n_pkg_bases; i++) {
		if (pkg_base_in_cache(pkg_bases[i])) {
			continue;
		}

		(void) fetch_pkg_base(pkg_bases[i]);
	}
}

char is_pwd_git_repo() {
	int git_subprocess = fork();

	if (git_subprocess == -1) {
		(void) fprintf(stderr, "[ERROR][%s:%d]: %s\n", __FILE__, __LINE__ - 3, strerror(errno));
		return 1;
	} else if (git_subprocess == 0) {
		int null_fd = open("/dev/null", O_WRONLY);

		run_syscall_print_err_w_ret(dup2(null_fd, STDOUT_FILENO), -1, __FILE__, __LINE__);
		run_syscall_print_err_w_ret(dup2(null_fd, STDERR_FILENO), -1, __FILE__, __LINE__);
		run_syscall_print_err_w_ret(close(null_fd), -1, __FILE__, __LINE__);

		if (execl("/usr/bin/git", "git", "status", NULL) < 0) {
			(void) fprintf(stderr, "[NOTE] \033[1;31m/usr/bin/git execution failed!\033[0m\n");
			(void) fprintf(stderr, "[NOTE] %s\n", strerror(errno));
			return -1;
		}

		_exit(-1);
		return -1;
	} else {
		int git_proc_stat;
		(void) waitpid(git_subprocess, &git_proc_stat, 0);

		if (WEXITSTATUS(git_proc_stat) != 0) {
			// (void) fprintf(stderr, "[WARNING] git exited with code %d.\n", WEXITSTATUS(git_proc_stat));
			return 0;
		}
	}
	return 1;
}

/**
 * WARNING: A potentially destructive function when tested inside
 *          a git repo!!
 */
int reset_pkg_base(const char* pkg_base) {
	if (is_pwd_git_repo()) {
		(void) write(STDERR_FILENO, 
			"--- \033[1;31mGIT RESET PACKAGE ABORTED\033[0m!!! ---\n"
			"This process involves \"git reset --hard\" and \"git clean -ffxd\", both are \033[1;31mdestructive git commands\033[0m.\n"
			"The present working directory (pwd) is detected to be a git repository. To avoid losing unstaged changes, this process is being aborted.\n"
		, 295);
		return -1;
	}

	if (!does_pkg_cache_exist() || !pkg_base_in_cache(pkg_base)) {
		return 0;
	}

	size_t dir_len = write_pkg_base_path(NULL, 0, pkg_base);
	char   dir[dir_len + 1];
	(void) write_pkg_base_path(dir, dir_len + 1, pkg_base);

	int git_subprocess = fork();

	if (git_subprocess == -1) {
		(void) fprintf(stderr, "[ERROR][%s:%d]: %s\n", __FILE__, __LINE__ - 3, strerror(errno));
		return -1;
	} else if (git_subprocess == 0) {
		(void) fprintf(stderr, "--- \033[1;32mChanging pwd for git to %s\033[0m ---\n", dir);

		run_syscall_print_err_w_ret(chdir(dir), -1, __FILE__, __LINE__);

		(void) fprintf(stderr, "--- \033[1;32mExecuting /usr/bin/git reset --hard\033[0m ---\n");

		if (execl("/usr/bin/git", "git", "reset", "--hard", NULL) < 0) {
			(void) fprintf(stderr, "[NOTE] \033[1;31m/usr/bin/git execution failed!\033[0m\n");
			(void) fprintf(stderr, "[NOTE] %s\n", strerror(errno));
			return -1;
		}

		_exit(-1);
		return -1;
	} else {
		int git_proc_stat;
		run_syscall_print_err_w_ret(waitpid(git_subprocess, &git_proc_stat, 0), -1, __FILE__, __LINE__);

		if (!WIFEXITED(git_proc_stat)) {
			(void) fprintf(stderr, "[WARNING] git exited with code %d.\n", WEXITSTATUS(git_proc_stat));
			return -1;
		}
	}

	git_subprocess = fork();

	if (git_subprocess == -1) {
		(void) fprintf(stderr, "[ERROR][%s:%d]: %s\n", __FILE__, __LINE__ - 3, strerror(errno));
		return -1;
	} else if (git_subprocess == 0) {
		(void) fprintf(stderr, "--- \033[1;32mChanging pwd for git to %s\033[0m ---\n", dir);

		run_syscall_print_err_w_ret(chdir(dir), -1, __FILE__, __LINE__);

		(void) fprintf(stderr, "--- \033[1;32mExecuting /usr/bin/git clean -ffxd\033[0m ---\n");

		if (execl("/usr/bin/git", "git", "clean", "-ffxd", NULL) < 0) {
			(void) fprintf(stderr, "[NOTE] \033[1;31m/usr/bin/git execution failed!\033[0m\n");
			(void) fprintf(stderr, "[NOTE] %s\n", strerror(errno));
			return -1;
		}

		_exit(-1);
		return -1;
	} else {
		int git_proc_stat;
		run_syscall_print_err_w_ret(waitpid(git_subprocess, &git_proc_stat, 0), -1, __FILE__, __LINE__);

		if (!WIFEXITED(git_proc_stat)) {
			(void) fprintf(stderr, "[WARNING] git exited with code %d.\n", WEXITSTATUS(git_proc_stat));
			return -1;
		}
	}

	return 0;
}

/**
 * WARNING: A potentially destructive function when tested inside
 *          a git repo!!
 */
void git_reset_aur_pkgs(char** pkg_bases, size_t n_pkg_bases) {
	if (is_pwd_git_repo()) {
		(void) write(STDERR_FILENO, 
			"--- \033[1;31mGIT RESET PACKAGE ABORTED\033[0m!!! ---\n"
			"This process involves \"git reset --hard\" and \"git clean -ffxd\", both are \033[1;31mdestructive git commands\033[0m.\n"
			"The present working directory (pwd) is detected to be a git repository. To avoid losing unstaged changes, this process is being aborted.\n"
		, 295);
		return;
	}

	for (size_t i = 0; i < n_pkg_bases; i++) {
		if (!pkg_base_in_cache(pkg_bases[i])) {
			continue;
		}

		reset_pkg_base(pkg_bases[i]);
	}
}
