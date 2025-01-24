#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "aur.h"
#include "aur/pkg_cache.h"
#include "file_utils.h"
#include "logger/logger.h"
#include "pacman.h"
#include "hashtable.h"
#include "subprocess_unix.h"
#include "unistd_helper.h"

#include "aur/pkg_install_stages.h"
#include "aur/pkg_update_report.h"
#include "aur/pkgver_cache.h"
#include "aur_pkg_parse.h"

static int pkg_name_cmp(const void* a, const void* b) {
	return strcmp(*((char* const*) a), *((char* const*) b));
}

int aur_check_less_wrap(char **pkg_namelist, size_t pkg_namelist_len, hashtable_t installed_pkgs_dict) {
	if (!isatty(STDOUT_FILENO)) {
		setvbuf(stdout, NULL, _IONBF, 0);
		perform_updates_summary_report(pkg_namelist, pkg_namelist_len, installed_pkgs_dict);
		return 0;
	}
	int less_pipe_fds[2];

	run_syscall_print_w_act(pipe(less_pipe_fds), return -1;, "ERROR", __FILE__, __LINE__);

	int aur_upd_child = fork();

	if (aur_upd_child == -1) {
		(void) close(less_pipe_fds[0]);
		(void) close(less_pipe_fds[1]);
		(void) fprintf(stderr, "[ERROR][%s:%d]: %s\n", __FILE__, __LINE__ - 3, strerror(errno));
		return -1;
	} else if (aur_upd_child == 0) {
		run_syscall_print_w_act(close(less_pipe_fds[0]), ;, "WARNING", __FILE__, __LINE__);
		run_syscall_print_w_act(dup2(less_pipe_fds[1], STDOUT_FILENO), (void) close(less_pipe_fds[1]); return -1;, "ERROR", __FILE__, __LINE__);
		setvbuf(stdout, NULL, _IONBF, 0);

		perform_updates_summary_report(pkg_namelist, pkg_namelist_len, installed_pkgs_dict);

		run_syscall_print_w_act(close(less_pipe_fds[1]), ;, "WARNING", __FILE__, __LINE__);

		_exit(0);
		return 0;
	} else {
		run_syscall_print_w_act(close(less_pipe_fds[1]), ;, "WARNING", __FILE__, __LINE__);

		(void) waitpid(aur_upd_child, NULL, 0);

		(void) run_subprocess(NULL, 1, 1, NULL, less_pipe_fds[0], NULL, "/usr/bin/less", "less", "-F", NULL);

		return 0;
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
	(void) fprintf(stderr, "       %s aur-upgrade\n", arg0);
	(void) fprintf(stderr, "       %s aur-fetchgit\n", arg0);
	(void) fprintf(stderr, "       %s aur-buildgit\n", arg0);
	(void) fprintf(stderr, "       %s aur-upgradegit\n", arg0);
}

int main(int argc, char** argv) {
	config_logging(DEBUG, isatty(STDOUT_FILENO));
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
		if (
			strcmp(argv[1], "aur-fetchupdates")    == 0 ||
			strcmp(argv[1], "aur-fetchdowngrades") == 0 ||
			strcmp(argv[1], "aur-fetchgit")        == 0 ||
			strcmp(argv[1], "aur-buildupdates")    == 0 ||
			strcmp(argv[1], "aur-builddowngrades") == 0 ||
			strcmp(argv[1], "aur-buildgit")        == 0 ||
			strcmp(argv[1], "aur-upgrade")         == 0 ||
			strcmp(argv[1], "aur-upgradegit")      == 0
		) {
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
		} else if (
			strcmp(argv[1], "aur-fetch") == 0 || 
			strcmp(argv[1], "aur-build") == 0 || 
			strcmp(argv[1], "aur-install") == 0
		) {
			return pkg_list_manage_subseq(argv[1], argc - 2, argv + 2);
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
			char is_fetch_updates_selected    = strcmp(argv[1], "aur-fetchupdates")    == 0;
			char is_fetch_downgrades_selected = strcmp(argv[1], "aur-fetchdowngrades") == 0;
			char is_fetch_git_selected        = strcmp(argv[1], "aur-fetchgit")        == 0;
			char is_build_updates_selected    = strcmp(argv[1], "aur-buildupdates")    == 0;
			char is_build_downgrades_selected = strcmp(argv[1], "aur-builddowngrades") == 0;
			char is_build_git_selected        = strcmp(argv[1], "aur-buildgit")        == 0;
			char is_upgrade_selected          = strcmp(argv[1], "aur-upgrade")         == 0;
			char is_upgrade_git_selected      = strcmp(argv[1], "aur-upgradegit")      == 0;

			if (
				is_fetch_updates_selected    ||
				is_fetch_downgrades_selected ||
				is_fetch_git_selected        ||
				is_build_downgrades_selected ||
				is_build_git_selected        ||
				is_build_updates_selected    ||
				is_upgrade_selected          ||
				is_upgrade_git_selected
			) {
				enum __aur_fetch_mode fetch_mode = NON_GIT_UPGRADES;

				if (is_fetch_git_selected || is_build_git_selected) {
					fetch_mode = GIT;
				}

				if (is_fetch_downgrades_selected || is_build_downgrades_selected) {
					fetch_mode = NON_GIT_DOWNGRADES;
				}

				enum __aur_action action = FETCH;

				if (is_build_downgrades_selected || is_build_git_selected || is_build_updates_selected) {
					action = BUILD;
				}

				if (is_upgrade_selected || is_upgrade_git_selected) {
					action = INSTALL;
				}

				if (argc - 2 > 0) {
					int n_excludes = (argc - 2) / 2 + (((argc - 2) & 1) != 0);
					char* excludes[n_excludes];

					for (int i = 2, j = 0; i < argc; i += 2, j++) {
						excludes[j] = argv[i + 1];
					}

					qsort(excludes, n_excludes, sizeof(char*), pkg_name_cmp);
					aur_fetch_updates(pkg_namelist, actual_namelist_size, installed_pkgs_dict, excludes, n_excludes, NULL, 0, fetch_mode, action);
				} else {
					aur_fetch_updates(pkg_namelist, actual_namelist_size, installed_pkgs_dict, NULL, 0, NULL, 0, fetch_mode, action);
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
