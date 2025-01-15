#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "aur.h"
#include "aur/pkg_cache.h"
#include "pacman.h"
#include "hashtable.h"
#include "unistd_helper.h"

#include "aur_pkg_parse.h"

static int pkg_name_cmp(const void* a, const void* b) {
	return strcmp(*((char* const*) a), *((char* const*) b));
}

void aur_list_git(char **pkg_namelist, size_t pkg_namelist_len, hashtable_t installed_pkgs_dict) {
	(void) write(STDOUT_FILENO, "--- \033[1;36mAUR Git Packages\033[0m ---\n", 36);
	for (size_t i = 0; i < pkg_namelist_len; i++) {
		struct hashtable_node* found_node = hashtable_find_inside_map(installed_pkgs_dict, pkg_namelist[i]);

		if (found_node -> is_non_aur || !found_node -> is_git_package) {
			continue;
		}

		char* revised_updated_ver = extract_existing_pkg_base_ver(found_node -> package_base, 0);
		if (revised_updated_ver == NULL) {
			revised_updated_ver = found_node -> updated_ver;
		}

		int  vercmp = compare_versions(found_node -> installed_ver, revised_updated_ver);
		enum update_stat revised_update_type = vercmp == 0 ? UP_TO_DATE : (vercmp < 0 ? UPGRADE : DOWNGRADE);

		char *col   = revised_update_type == UPGRADE ? "\033[32m" : (revised_update_type == DOWNGRADE ? "\033[31m" : "");
		char *arrow = revised_update_type == UPGRADE ? "->" : (revised_update_type == DOWNGRADE ? "<-" : "==");
		(void) printf("%s %s\033[1m%s\033[0m %s %s\033[1m%s\033[0m\n", pkg_namelist[i], col, found_node -> installed_ver, arrow, col, revised_updated_ver);
	}
	clean_up_extract_existing_pkg_base_ver();
}

void aur_check_non_git_downgrades(char **pkg_namelist, size_t pkg_namelist_len, hashtable_t installed_pkgs_dict) {
	(void) write(STDOUT_FILENO, "--- \033[1;36mAUR \033[31mDowngrades\033[0m ---\n", 39);
	for (size_t i = 0; i < pkg_namelist_len; i++) {
		struct hashtable_node* found_node = hashtable_find_inside_map(installed_pkgs_dict, pkg_namelist[i]);

		if (found_node -> is_non_aur || found_node -> is_git_package || found_node -> update_type != DOWNGRADE) {
			continue;
		}

		char *col   = found_node -> update_type == UPGRADE ? "\033[32m" : (found_node -> update_type == DOWNGRADE ? "\033[31m" : "");
		char *arrow = found_node -> update_type == UPGRADE ? "->" : (found_node -> update_type == DOWNGRADE ? "<-" : "==");
		(void) printf("%s %s\033[1m%s\033[0m %s %s\033[1m%s\033[0m\n", pkg_namelist[i], col, found_node -> installed_ver, arrow, col, found_node -> updated_ver);
	}
}

void aur_check_non_git(char **pkg_namelist, size_t pkg_namelist_len, hashtable_t installed_pkgs_dict) {
	(void) write(STDOUT_FILENO, "--- \033[1;36mAUR Updates\033[0m ---\n", 31);
	for (size_t i = 0; i < pkg_namelist_len; i++) {
		struct hashtable_node* found_node = hashtable_find_inside_map(installed_pkgs_dict, pkg_namelist[i]);

		if (found_node -> is_non_aur || found_node -> is_git_package || found_node -> update_type != UPGRADE) {
			continue;
		}

		char *col   = found_node -> update_type == UPGRADE ? "\033[32m" : (found_node -> update_type == DOWNGRADE ? "\033[31m" : "");
		char *arrow = found_node -> update_type == UPGRADE ? "->" : (found_node -> update_type == DOWNGRADE ? "<-" : "==");
		(void) printf("%s %s\033[1m%s\033[0m %s %s\033[1m%s\033[0m\n", pkg_namelist[i], col, found_node -> installed_ver, arrow, col, found_node -> updated_ver);
	}
}

enum __aur_fetch_mode {
	NON_GIT_UPGRADES,
	NON_GIT_DOWNGRADES,
	GIT
};

void aur_fetch_non_git_updates(char **pkg_namelist, size_t pkg_namelist_len, hashtable_t installed_pkgs_dict, char **ignore_list, size_t ignore_list_len, enum __aur_fetch_mode fetch_type) {
	for (size_t i = 0, j = 0; i < pkg_namelist_len; i++) {
		if (ignore_list != NULL && j < ignore_list_len) {
			int c = strcmp(pkg_namelist[i], ignore_list[j]);
			while (j < ignore_list_len && c > 0) {
				j++;
			}
			if (c == 0) {
				j++;
				continue;
			}
		}
		struct hashtable_node* found_node = hashtable_find_inside_map(installed_pkgs_dict, pkg_namelist[i]);

		if (found_node -> is_non_aur) {
			continue;
		}

		if (fetch_type == GIT && !found_node -> is_git_package) {
			continue;
		}

		if (fetch_type != GIT && found_node -> is_git_package) {
			continue;
		}

		if (fetch_type == NON_GIT_UPGRADES && found_node -> update_type != UPGRADE) {
			continue;
		}

		if (fetch_type == NON_GIT_DOWNGRADES && found_node -> update_type != DOWNGRADE) {
			continue;
		}

		(void) fetch_pkg_base(found_node -> package_base == NULL ? pkg_namelist[i] : found_node -> package_base);
	}
}

void perform_updates_summary_report(char **pkg_namelist, size_t pkg_namelist_len, hashtable_t installed_pkgs_dict) {
	aur_check_non_git(pkg_namelist, pkg_namelist_len, installed_pkgs_dict);
	(void) write(STDOUT_FILENO, "\n", 1);
	aur_check_non_git_downgrades(pkg_namelist, pkg_namelist_len, installed_pkgs_dict);
	(void) write(STDOUT_FILENO, "\n", 1);
	aur_list_git(pkg_namelist, pkg_namelist_len, installed_pkgs_dict);
	(void) write(STDOUT_FILENO, "\n--- \033[1;34mPacman Updates\033[0m ---\n", 35);
	(void) perform_pacman_checkupdates();
}

int aur_check_less_wrap(char **pkg_namelist, size_t pkg_namelist_len, hashtable_t installed_pkgs_dict) {
	if (!isatty(STDOUT_FILENO)) {
		setvbuf(stdout, NULL, _IONBF, 0);
		perform_updates_summary_report(pkg_namelist, pkg_namelist_len, installed_pkgs_dict);
		return 0;
	}
	int less_pipe_fds[2];

	run_syscall_print_err_w_ret(pipe(less_pipe_fds), -1, __FILE__, __LINE__);

	int aur_upd_child = fork();

	if (aur_upd_child == -1) {
		(void) close(less_pipe_fds[0]);
		(void) close(less_pipe_fds[1]);
		(void) fprintf(stderr, "[ERROR][%s:%d]: %s\n", __FILE__, __LINE__ - 3, strerror(errno));
		return -1;
	} else if (aur_upd_child == 0) {
		run_syscall_print_err_w_ret(close(less_pipe_fds[0]), -1, __FILE__, __LINE__);
		run_syscall_print_err_w_ret(dup2(less_pipe_fds[1], STDOUT_FILENO), -1, __FILE__, __LINE__);
		setvbuf(stdout, NULL, _IONBF, 0);

		perform_updates_summary_report(pkg_namelist, pkg_namelist_len, installed_pkgs_dict);
		// aur_check_non_git(pkg_namelist, pkg_namelist_len, installed_pkgs_dict);
		// (void) write(STDOUT_FILENO, "\n", 1);
		// aur_check_non_git_downgrades(pkg_namelist, pkg_namelist_len, installed_pkgs_dict);
		// (void) write(STDOUT_FILENO, "\n", 1);
		// aur_list_git(pkg_namelist, pkg_namelist_len, installed_pkgs_dict);
		// (void) write(STDOUT_FILENO, "\n--- \033[1;34mPacman Updates\033[0m ---\n", 35);
		// (void) perform_pacman_checkupdates();

		run_syscall_print_err_w_ret(close(less_pipe_fds[1]), -1, __FILE__, __LINE__);

		_exit(0);
		return 0;
	} else {
		run_syscall_print_err_w_ret(close(less_pipe_fds[1]), -1, __FILE__, __LINE__);

		int less_child = fork();

		if (less_child == -1) {
			(void) close(less_pipe_fds[0]);
			(void) fprintf(stderr, "[ERROR][%s:%d]: %s\n", __FILE__, __LINE__ - 3, strerror(errno));
			return -1;
		} else if (less_child == 0) {
			run_syscall_print_err_w_ret(dup2(less_pipe_fds[0], STDIN_FILENO), -1, __FILE__, __LINE__);
			run_syscall_print_err_w_ret(close(less_pipe_fds[0]), -1, __FILE__, __LINE__);

			if (execl("/usr/bin/less", "less", "-F", NULL) < 0) {
				(void) fprintf(stderr, "[NOTE] /usr/bin/less \033[1;31mexecution failed!\033[0m\n");
				(void) fprintf(stderr, "[NOTE] %s\n", strerror(errno));
				return -1;
			}

			return -1;
		} else {
			run_syscall_print_err_w_ret(close(less_pipe_fds[0]), -1, __FILE__, __LINE__);

			(void) waitpid(less_child, NULL, 0);

			return 0;
		}
	}
}

void print_usage(const char* arg0) {
	(void) fprintf(stderr, "usage: %s\n",      arg0);
	(void) fprintf(stderr, "       %s help\n", arg0);
	(void) fprintf(stderr, "       %s pacman-checkupdates\n", arg0);
	(void) fprintf(stderr, "       %s pacman-upgrade\n", arg0);
	(void) fprintf(stderr, "       %s aur-checkupdates\n", arg0);
	(void) fprintf(stderr, "       %s aur-fetchupdates\n", arg0);
	(void) fprintf(stderr, "       %s aur-buildupdates\n", arg0);
	(void) fprintf(stderr, "       %s aur-fetchgit\n", arg0);
}

int main(int argc, char** argv) {
	if (argc > 1) {
		if (strcmp(argv[1], "help") == 0) {
			print_usage(argv[0]);
			return EXIT_SUCCESS;
		}
		if (strcmp(argv[1], "pacman-checkupdates") == 0) {
			return perform_pacman_checkupdates();
		}
		if (strcmp(argv[1], "pacman-upgrade") == 0) {
			return perform_pacman_upgrade(argc - 2, argv + 2);
		}
		if (strcmp(argv[1], "aur-fetchupdates") == 0 || strcmp(argv[1], "aur-fetchdowngrades") == 0 || strcmp(argv[1], "aur-fetchgit") == 0) {
			// Checking argument validity before performing actual update fetching.
			// This will exit the program immediately.
			if (argc - 2 > 0) {
				for (int i = 2, j = 0; i < argc; i += 2, j++) {
					if (strcmp(argv[i], "--exclude") != 0) {
						(void) fprintf(stderr, "[ERROR][aur-fetchupdates] Unrecognized option. Currently only supports --exclude.\n");
						return 1;
					}

					if (i + 1 >= argc) {
						(void) fprintf(stderr, "[ERROR][aur-fetchupdates] Missing operand for --exclude.\n");
						return 1;
					}
				}
			}
		} else {
			(void) fprintf(stderr, "[ERROR] Unknown option supplied.\n");
			print_usage(argv[0]);
			return EXIT_FAILURE;
		}
	}

	pacman_names_vers_t installed_pkgs = get_installed_non_pacman();
	char* pkg_namelist[installed_pkgs.n_items];
	size_t actual_namelist_size = 0;

	struct hashtable_node* map  [installed_pkgs.n_items];
	struct hashtable_node  stash[installed_pkgs.n_items];
	hashtable_t installed_pkgs_dict = hashtable_init(map, installed_pkgs.n_items, stash, installed_pkgs.n_items);

	for (size_t i = 0; i < installed_pkgs.n_items; i++) {
		if (installed_pkgs.pkg_names_vers[i].valid) {
			hashtable_set_item(&installed_pkgs_dict, installed_pkgs.pkg_names_vers[i].name, installed_pkgs.pkg_names_vers[i].version, NULL, 1);
			pkg_namelist[actual_namelist_size] = installed_pkgs.pkg_names_vers[i].name;
			actual_namelist_size++;
		}
	}

	const char* response_json = get_packages_info((const char* const*) pkg_namelist, actual_namelist_size);

	if (response_json != NULL) {
		size_t res_json_len = strlen(response_json);
		char mutable_res_json[res_json_len + 1];
		(void) strncpy(mutable_res_json, response_json, res_json_len + 1);
		clean_up_pkg_info_buffer();

		process_response_json(mutable_res_json, &installed_pkgs_dict);

		if (argc > 1) {
			int is_fetch_updates_selected = strcmp(argv[1], "aur-fetchupdates") == 0;
			int is_fetch_downgrades_selected = strcmp(argv[1], "aur-fetchdowngrades") == 0;
			int is_fetch_git_selected = strcmp(argv[1], "aur-fetchgit") == 0;
			if (is_fetch_updates_selected || is_fetch_downgrades_selected || is_fetch_git_selected) {
				enum __aur_fetch_mode fetch_mode = is_fetch_git_selected ? GIT : (is_fetch_downgrades_selected ? NON_GIT_DOWNGRADES : NON_GIT_UPGRADES);
				if (argc - 2 > 0) {
					int n_excludes = (argc - 2) / 2 + (((argc - 2) & 1) != 0);
					char* excludes[n_excludes];

					for (int i = 2, j = 0; i < argc; i += 2, j++) {
						excludes[j] = argv[i + 1];
					}

					qsort(excludes, n_excludes, sizeof(char*), pkg_name_cmp);
					aur_fetch_non_git_updates(pkg_namelist, actual_namelist_size, installed_pkgs_dict, excludes, n_excludes, fetch_mode);
				} else {
					aur_fetch_non_git_updates(pkg_namelist, actual_namelist_size, installed_pkgs_dict, NULL, 0, fetch_mode);
				}
				clean_up_pacman_output();
				return 0;
			}
		}

		// Need to repeat here inside the if statement or else
		// the updated version names gets deallocated.
		(void) aur_check_less_wrap(pkg_namelist, actual_namelist_size, installed_pkgs_dict);
	} else {
		(void) aur_check_less_wrap(pkg_namelist, actual_namelist_size, installed_pkgs_dict);
	}

	// aur_check_non_git(pkg_namelist, actual_namelist_size, installed_pkgs_dict);
	
	clean_up_pacman_output();
	return 0;
}
