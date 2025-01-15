#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../unistd_helper.h"
#include "pkg_cache.h"

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

/**
 * WARNING: A potentially destructive function when tested inside
 *          a git repo!!
 */
int update_existing_pkg_base(const char* pkg_base) {
	if (!does_pkg_cache_exist() || !pkg_base_in_cache(pkg_base)) {
		return 0;
	}

	if (reset_pkg_base(pkg_base) < 0) {
		return -1;
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

		(void) fprintf(stderr, "--- \033[1;32mExecuting /usr/bin/git fetch\033[0m ---\n");

		if (execl("/usr/bin/git", "git", "fetch", NULL) < 0) {
			(void) fprintf(stderr, "[NOTE] \033[1;31m/usr/bin/git execution failed!\033[0m\n");
			(void) fprintf(stderr, "[NOTE] %s\n", strerror(errno));
			return -1;
		}

		_exit(-1);
		return -1;
	} else {
		int git_proc_stat;
		run_syscall_print_err_w_ret(waitpid(git_subprocess, &git_proc_stat, 0), -1, __FILE__, __LINE__);

		if (WEXITSTATUS(git_proc_stat) != 0) {
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

		(void) fprintf(stderr, "--- \033[1;32mExecuting /usr/bin/git rebase\033[0m ---\n");

		if (execl("/usr/bin/git", "git", "rebase", NULL) < 0) {
			(void) fprintf(stderr, "[NOTE] \033[1;31m/usr/bin/git execution failed!\033[0m\n");
			(void) fprintf(stderr, "[NOTE] %s\n", strerror(errno));
			return -1;
		}

		_exit(-1);
		return -1;
	} else {
		int git_proc_stat;
		run_syscall_print_err_w_ret(waitpid(git_subprocess, &git_proc_stat, 0), -1, __FILE__, __LINE__);

		if (WEXITSTATUS(git_proc_stat) != 0) {
			(void) fprintf(stderr, "[WARNING] git exited with code %d.\n", WEXITSTATUS(git_proc_stat));
			return -1;
		}
	}

	return 0;
}

int build_existing_pkg_base(const char* pkg_base) {
	if (!does_pkg_cache_exist() || !pkg_base_in_cache(pkg_base)) {
		return 0;
	}

	size_t dir_len = write_pkg_base_path(NULL, 0, pkg_base);
	char   dir[dir_len + 1];
	(void) write_pkg_base_path(dir, dir_len + 1, pkg_base);

	int makepkg_subprocess = fork();

	if (makepkg_subprocess == -1) {
		(void) fprintf(stderr, "[ERROR][%s:%d]: %s\n", __FILE__, __LINE__ - 3, strerror(errno));
		return -1;
	} else if (makepkg_subprocess == 0) {
		(void) fprintf(stderr, "--- \033[1;32mChanging pwd for makepkg to %s\033[0m ---\n", dir);

		run_syscall_print_err_w_ret(chdir(dir), -1, __FILE__, __LINE__);

		(void) fprintf(stderr, "--- \033[1;32mExecuting /usr/bin/makepkg -s\033[0m ---\n");

		if (execl("/usr/bin/makepkg", "makepkg", "-s", NULL) < 0) {
			(void) fprintf(stderr, "[NOTE] \033[1;31m/usr/bin/makepkg execution failed!\033[0m\n");
			(void) fprintf(stderr, "[NOTE] %s\n", strerror(errno));
			return -1;
		}

		_exit(-1);
		return -1;
	} else {
		int makepkg_stat;
		run_syscall_print_err_w_ret(waitpid(makepkg_subprocess, &makepkg_stat, 0), -1, __FILE__, __LINE__);

		if (WEXITSTATUS(makepkg_stat) != 0) {
			(void) fprintf(stderr, "[WARNING] git exited with code %d.\n", WEXITSTATUS(makepkg_stat));
			return -1;
		}
	}

	return 0;
}

void build_aur_pkgs(char** pkg_bases, size_t n_pkg_bases) {
	for (size_t i = 0; i < n_pkg_bases; i++) {
		if (!pkg_base_in_cache(pkg_bases[i])) {
			continue;
		}

		(void) build_existing_pkg_base(pkg_bases[i]);
	}
}

char* extract_existing_ver_ret = NULL;

char* extract_existing_pkg_base_ver(const char* pkg_base, char quiet) {
	if (!does_pkg_cache_exist() || !pkg_base_in_cache(pkg_base)) {
		return NULL;
	}

	size_t dir_len = write_pkg_base_path(NULL, 0, pkg_base);
	char   dir[dir_len + 1];
	(void) write_pkg_base_path(dir, dir_len + 1, pkg_base);

	int output_fds[2];

	run_syscall_print_err_w_ret(pipe(output_fds), NULL, __FILE__, __LINE__);

	int makepkg_subprocess = fork();

	if (makepkg_subprocess == -1) {
		(void) fprintf(stderr, "[ERROR][%s:%d]: %s\n", __FILE__, __LINE__ - 3, strerror(errno));
		return NULL;
	} else if (makepkg_subprocess == 0) {
		run_syscall_print_err_w_ret(close(output_fds[0]), NULL, __FILE__, __LINE__);
		run_syscall_print_err_w_ret(dup2(output_fds[1], STDOUT_FILENO), NULL, __FILE__, __LINE__);
		run_syscall_print_err_w_ret(close(output_fds[1]), NULL, __FILE__, __LINE__);

		if (!quiet)
			(void) fprintf(stderr, "--- \033[1;32mChanging pwd for makepkg to %s\033[0m ---\n", dir);

		run_syscall_print_err_w_ret(chdir(dir), NULL, __FILE__, __LINE__);

		if (!quiet)
			(void) fprintf(stderr, "--- \033[1;32mExecuting /usr/bin/makepkg --printsrcinfo\033[0m ---\n");

		if (execl("/usr/bin/makepkg", "makepkg", "--printsrcinfo", NULL) < 0) {
			(void) fprintf(stderr, "[NOTE] \033[1;31m/usr/bin/makepkg execution failed!\033[0m\n");
			(void) fprintf(stderr, "[NOTE] %s\n", strerror(errno));
			return NULL;
		}

		_exit(-1);
		return NULL;
	} else {
		run_syscall_print_err_w_ret(close(output_fds[1]), NULL, __FILE__, __LINE__);

		int makepkg_stat;
		run_syscall_print_err_w_ret(waitpid(makepkg_subprocess, &makepkg_stat, 0), NULL, __FILE__, __LINE__);

		if (WEXITSTATUS(makepkg_stat) != 0) {
			(void) fprintf(stderr, "[WARNING] git exited with code %d.\n", WEXITSTATUS(makepkg_stat));
			return NULL;
		}
	}

	char output_buf[1025];
	
	for(;;) {
		int r_size = read(output_fds[0], output_buf, 1024);

		if (r_size < 0) {
			(void) fprintf(stderr, "[ERROR][%s:%d]: %s\n", __FILE__, __LINE__ - 3, strerror(errno));
			return NULL;
		}

		if (r_size == 0) {
			return NULL;
		}

		output_buf[1024] = '\0';

		// Finding needle in haystack, needle is the string to find.
		char*  needle     = "pkgver = ";
		size_t needle_len = strlen(needle);
		
		char* found_loc = strstr(output_buf, needle);
		if (found_loc == NULL) {
			continue;
		}

		char* end_loc = strchr(found_loc, '\n');
		if (end_loc != NULL) {
			*end_loc = '\0';
			char* pkgver      = found_loc + needle_len;
			size_t pkgver_len = strlen(pkgver);
			if (extract_existing_ver_ret == NULL) {
				extract_existing_ver_ret = (char*) malloc((pkgver_len + 1) * sizeof(char));
			} else {
				extract_existing_ver_ret = (char*) realloc(extract_existing_ver_ret, (pkgver_len + 1) * sizeof(char));
			}
			if (extract_existing_ver_ret == NULL) {
				perror("[ERROR][extract_existing_pkg_base_ver]");
				return NULL;
			}
			(void) strncpy(extract_existing_ver_ret, pkgver, pkgver_len + 1);
			return extract_existing_ver_ret;
		}

		char second_part[1025];
		r_size = read(output_fds[0], second_part, 1024);

		if (r_size == 0) {
			return NULL;
		}

		output_buf[1024] = '\0';
		end_loc = strchr(second_part, '\n');
		if (end_loc == NULL) {
			return NULL;
		}
		*end_loc = '\0';
		size_t total_len = strlen(found_loc) + strlen(second_part);
		if (extract_existing_ver_ret == NULL) {
			extract_existing_ver_ret = (char*) malloc((total_len + 1) * sizeof(char));
		} else {
			extract_existing_ver_ret = (char*) realloc(extract_existing_ver_ret, (total_len + 1) * sizeof(char));
		}
		if (extract_existing_ver_ret == NULL) {
			perror("[ERROR][extract_existing_pkg_base_ver]");
			return NULL;
		}
		(void) strncpy(extract_existing_ver_ret, found_loc, total_len + 1);
		(void) strncat(extract_existing_ver_ret, second_part, total_len + 1);
		return extract_existing_ver_ret;
	}

	return NULL;
}

void clean_up_extract_existing_pkg_base_ver() {
	if (extract_existing_ver_ret != NULL) {
		free(extract_existing_ver_ret);
		extract_existing_ver_ret = NULL;
	}
}

void stream_ver_to_main_proc(const char* pkg_base, int write_fd, size_t* slen_out) {
	char* ver = extract_existing_pkg_base_ver(pkg_base, 1);
	*slen_out = strlen(ver);
	int r = write(write_fd, ver, strlen(ver));
	clean_up_extract_existing_pkg_base_ver();
	if (r < 0) {
		_exit(errno);
	}
	_exit(0);
}

int (*mp_extr_fd_matrix)[2] = NULL;
void extract_ver_from_pkgs_muti_proc(char** __restrict__ s_out, size_t* __restrict__ slens_out, const char* pkg_bases[], size_t n_items) {
	if (mp_extr_fd_matrix != NULL) {
		if (s_out == NULL || slens_out == NULL) {
			return;
		}
		for (size_t i = 0; i < n_items; i++) {
			int r = read(mp_extr_fd_matrix[i][0], s_out[i], slens_out[i]);
			assert(r == slens_out[i]);
			run_syscall_print_err(close(mp_extr_fd_matrix[i][0]), __FILE__, __LINE__);
		}

		free(mp_extr_fd_matrix);
		mp_extr_fd_matrix = NULL;
		return;
	}

	mp_extr_fd_matrix = malloc(n_items * sizeof(int[2]));
	int child_ids[n_items];

	int shmid = shmget(IPC_PRIVATE, n_items * sizeof(size_t), IPC_CREAT | 0600);
	size_t* strlens = shmat(shmid, NULL, 0);

	for (size_t i = 0; i < n_items; i++) {
		run_syscall_print_err(pipe(mp_extr_fd_matrix[i]), __FILE__, __LINE__);

		int child = fork();
		child_ids[i] = child;

		if (child < 0) {
			(void) fprintf(stderr, "[ERROR][%s:%d]: %s\n", __FILE__, __LINE__ - 4, strerror(errno));
			return;
		} else if (child == 0) {
			run_syscall_print_err(close(mp_extr_fd_matrix[i][0]), __FILE__, __LINE__);
			stream_ver_to_main_proc(pkg_bases[i], mp_extr_fd_matrix[i][1], strlens + i);
		} else {
			run_syscall_print_err(close(mp_extr_fd_matrix[i][1]), __FILE__, __LINE__);
		}
	}

	// size_t total_str_len = 0;
	for (size_t i = 0; i < n_items; i++) {
		int stat;
		(void) waitpid(child_ids[i], &stat, 0);
		
		if (WEXITSTATUS(stat) != 0) {
			(void) fprintf(stderr, "[WARNING][extract_proc_%zu][SUBPROCESS_ERROR] %s\n", i, strerror(WEXITSTATUS(stat)));
		}

		if (slens_out != NULL) {
			slens_out[i] = strlens[i];
		}
	}

	(void) shmdt(strlens);
	(void) shmctl(shmid, IPC_RMID, 0);
}

int update_existing_git_pkg_base(const char* pkg_base) {
	if (!does_pkg_cache_exist() || !pkg_base_in_cache(pkg_base)) {
		return 0;
	}

	size_t dir_len = write_pkg_base_path(NULL, 0, pkg_base);
	char   dir[dir_len + 1];
	(void) write_pkg_base_path(dir, dir_len + 1, pkg_base);

	int makepkg_subprocess = fork();

	if (makepkg_subprocess == -1) {
		(void) fprintf(stderr, "[ERROR][%s:%d]: %s\n", __FILE__, __LINE__ - 3, strerror(errno));
		return -1;
	} else if (makepkg_subprocess == 0) {
		(void) fprintf(stderr, "--- \033[1;32mChanging pwd for makepkg to %s\033[0m ---\n", dir);

		run_syscall_print_err_w_ret(chdir(dir), -1, __FILE__, __LINE__);

		(void) fprintf(stderr, "--- \033[1;32mExecuting /usr/bin/makepkg -o\033[0m ---\n");

		if (execl("/usr/bin/makepkg", "makepkg", "-o", NULL) < 0) {
			(void) fprintf(stderr, "[NOTE] \033[1;31m/usr/bin/makepkg execution failed!\033[0m\n");
			(void) fprintf(stderr, "[NOTE] %s\n", strerror(errno));
			return -1;
		}

		_exit(-1);
		return -1;
	} else {
		int makepkg_stat;
		run_syscall_print_err_w_ret(waitpid(makepkg_subprocess, &makepkg_stat, 0), -1, __FILE__, __LINE__);

		if (WEXITSTATUS(makepkg_stat) != 0) {
			(void) fprintf(stderr, "[WARNING] git exited with code %d.\n", WEXITSTATUS(makepkg_stat));
			return -1;
		}
	}

	return 0;
}
