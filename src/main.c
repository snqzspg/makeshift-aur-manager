#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "aur.h"
#include "aur/pkg_cache.h"
#include "file_utils.h"
#include "pacman.h"
#include "hashtable.h"
#include "subprocess_unix.h"
#include "unistd_helper.h"

#include "aur/pkg_install_stages.h"
#include "aur/pkgver_cache.h"
#include "aur_pkg_parse.h"

static int pkg_name_cmp(const void* a, const void* b) {
	return strcmp(*((char* const*) a), *((char* const*) b));
}

void aur_list_git(char **pkg_namelist, size_t pkg_namelist_len, hashtable_t installed_pkgs_dict) {
	(void) write(STDOUT_FILENO, "--- \033[1;36mAUR Git Packages\033[0m ---\n", 36);
	pacman_names_vers_t* ver_cache = load_ver_cache();
	for (size_t i = 0, j = 0; i < pkg_namelist_len; i++) {
		struct hashtable_node* found_node = hashtable_find_inside_map(installed_pkgs_dict, pkg_namelist[i]);

		if (found_node -> is_non_aur || !found_node -> is_git_package) {
			continue;
		}

		char* revised_updated_ver = found_node -> updated_ver;

		if (j < ver_cache -> n_items) {
			while (strcmp(pkg_namelist[i], ver_cache -> pkg_names_vers[j].name) > 0 && j < ver_cache -> n_items) {
				j++;
			}
			if (strcmp(pkg_namelist[i], ver_cache -> pkg_names_vers[j].name) == 0) {
				revised_updated_ver = ver_cache -> pkg_names_vers[j].version;
			} else {
				revised_updated_ver = extract_existing_pkg_base_ver(found_node -> package_base, 0);
				if (revised_updated_ver == NULL) {
					revised_updated_ver = found_node -> updated_ver;
				}
			}
		}

		int  vercmp = compare_versions(found_node -> installed_ver, revised_updated_ver);
		enum update_stat revised_update_type = vercmp == 0 ? UP_TO_DATE : (vercmp < 0 ? UPGRADE : DOWNGRADE);

		char *col   = revised_update_type == UPGRADE ? "\033[32m" : (revised_update_type == DOWNGRADE ? "\033[31m" : "");
		char *arrow = revised_update_type == UPGRADE ? "->" : (revised_update_type == DOWNGRADE ? "<-" : "==");
		(void) printf("%s %s\033[1m%s\033[0m %s %s\033[1m%s\033[0m\n", pkg_namelist[i], col, found_node -> installed_ver, arrow, col, revised_updated_ver);
	}
	clean_up_extract_existing_pkg_base_ver();
	discard_ver_cache();
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

// enum __aur_fetch_mode {
// 	NON_GIT_UPGRADES,
// 	NON_GIT_DOWNGRADES,
// 	GIT
// };

// enum __aur_action {
// 	FETCH,
// 	TRUST,
// 	PATCH,
// 	BUILD,
// 	INSTALL
// };

// size_t filter_pkg_updates(pacman_name_ver_t* __restrict__ filtered_list_out, size_t filtered_list_limit, char **pkg_namelist, size_t pkg_namelist_len, hashtable_t installed_pkgs_dict, char **ignore_list, size_t ignore_list_len, enum __aur_fetch_mode fetch_type) {
// 	size_t pkg_count = 0;
// 	for (size_t i = 0, j = 0; i < pkg_namelist_len; i++) {
// 		if (ignore_list != NULL && j < ignore_list_len) {
// 			int c = strcmp(pkg_namelist[i], ignore_list[j]);
// 			while (j < ignore_list_len && c > 0) {
// 				j++;
// 			}
// 			if (c == 0) {
// 				j++;
// 				continue;
// 			}
// 		}
// 		struct hashtable_node* found_node = hashtable_find_inside_map(installed_pkgs_dict, pkg_namelist[i]);

// 		if (found_node -> is_non_aur) {
// 			continue;
// 		}

// 		if (fetch_type == GIT && !found_node -> is_git_package) {
// 			continue;
// 		}

// 		if (fetch_type != GIT && found_node -> is_git_package) {
// 			continue;
// 		}

// 		if (fetch_type == NON_GIT_UPGRADES && found_node -> update_type != UPGRADE) {
// 			continue;
// 		}

// 		if (fetch_type == NON_GIT_DOWNGRADES && found_node -> update_type != DOWNGRADE) {
// 			continue;
// 		}

// 		if (pkg_count < filtered_list_limit && filtered_list_out != NULL) {
// 			filtered_list_out[pkg_count].name    = pkg_namelist[i];
// 			filtered_list_out[pkg_count].valid   = 1;
// 			filtered_list_out[pkg_count].version = found_node -> updated_ver;
// 		}

// 		pkg_count++;
// 	}

// 	return pkg_count;
// }

void aur_fetch_updates(char **pkg_namelist, size_t pkg_namelist_len, hashtable_t installed_pkgs_dict, char **ignore_list, size_t ignore_list_len, enum __aur_fetch_mode fetch_type, enum __aur_action action) {
	size_t pkg_count = filter_pkg_updates(NULL, 0, pkg_namelist, pkg_namelist_len, installed_pkgs_dict, ignore_list, ignore_list_len, fetch_type);

	pacman_name_ver_t filtered_list[pkg_count];

	(void) filter_pkg_updates(filtered_list, pkg_count, pkg_namelist, pkg_namelist_len, installed_pkgs_dict, ignore_list, ignore_list_len, fetch_type);

	char* new_ver_strs[pkg_count];

	(void) memset(new_ver_strs, 0, pkg_count * sizeof(char*));

	if (action >= FETCH) {
		for (size_t i = 0; i < pkg_count; i++) {
			struct hashtable_node* found_node = hashtable_find_inside_map(installed_pkgs_dict, filtered_list[i].name);
			char* pkgbase = found_node -> package_base == NULL ? filtered_list[i].name : found_node -> package_base;

			(void) fetch_pkg_base(pkgbase);
		}
	}

	if (action >= FETCH /* PATCH */) {
		if (fetch_type == GIT) {
			for (size_t i = 0; i < pkg_count; i++) {
				struct hashtable_node* found_node = hashtable_find_inside_map(installed_pkgs_dict, filtered_list[i].name);
				char* pkgbase = found_node -> package_base == NULL ? filtered_list[i].name : found_node -> package_base;

				update_existing_git_pkg_base(pkgbase);
				char* v = extract_existing_pkg_base_ver(pkgbase, 0);
				if (v != NULL) {
					new_ver_strs[i] = (char*) malloc((strlen(v) + 1) * sizeof(char));
					(void) strncpy(new_ver_strs[i], v, strlen(v) + 1);
					v = new_ver_strs[i];
					filtered_list[i].version = v;
				}
			}

			(void) load_ver_cache();
			merge_in_ver_cache(filtered_list, pkg_count);
			save_ver_cache();
			discard_ver_cache();
			for (size_t i = 0; i < pkg_count; i++) {
				free(new_ver_strs[i]);
			}
		}
	}

	if (action == BUILD) {
		for (size_t i = 0; i < pkg_count; i++) {
			struct hashtable_node* found_node = hashtable_find_inside_map(installed_pkgs_dict, filtered_list[i].name);

			char* pkgbase = found_node -> package_base == NULL ? filtered_list[i].name : found_node -> package_base;

			(void) build_existing_pkg_base(pkgbase);
		}
	}

	if (action == INSTALL) {
        char* pacman_args[pkg_count + 4];
		pacman_args[0] = "sudo";
		pacman_args[1] = "pacman";
		pacman_args[2] = "-U";
		pacman_args[pkg_count + 3] = NULL;

		for (size_t i = 0; i < pkg_count; i++) {
			struct hashtable_node* found_node = hashtable_find_inside_map(installed_pkgs_dict, filtered_list[i].name);
			char* pkgbase = found_node -> package_base == NULL ? filtered_list[i].name : found_node -> package_base;

			char* pkg_file = pkg_file_path_stream_alloc(filtered_list[i].name, pkgbase, 0);
			if (pkg_file == NULL) {
				continue;
			}

			(void) fprintf(stderr, "[DEBUG] This package would be at \033[1;33m%s\033[0m.\n", pkg_file);

			pacman_args[i + 3] = pkg_file;
		}

		(void) fprintf(stderr, "[INFO] Use the command \033[1;32m");
		for (size_t i = 0; i < pkg_count + 3; i++) {
			(void) fprintf(stderr, "%s ", pacman_args[i]);
		}
		(void) fprintf(stderr, "\033[0mto install.\n");

		(void) run_subprocess_v(NULL, "/usr/bin/sudo", (char**) pacman_args, NULL, STDIN_FILENO, NULL, 0, 1);

		for (size_t i = 3; i < pkg_count + 3; i++) {
			free(pacman_args[i]);
		}
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
			strcmp(argv[1], "aur-fetchupdates") == 0 ||
			strcmp(argv[1], "aur-fetchdowngrades") == 0 ||
			strcmp(argv[1], "aur-fetchgit") == 0 ||
			strcmp(argv[1], "aur-buildupdates") == 0 ||
			strcmp(argv[1], "aur-builddowngrades") == 0 ||
			strcmp(argv[1], "aur-buildgit") == 0 ||
			strcmp(argv[1], "aur-upgrade") == 0 ||
			strcmp(argv[1], "aur-upgradegit") == 0
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
			char is_fetch_updates_selected = strcmp(argv[1], "aur-fetchupdates") == 0;
			char is_fetch_downgrades_selected = strcmp(argv[1], "aur-fetchdowngrades") == 0;
			char is_fetch_git_selected = strcmp(argv[1], "aur-fetchgit") == 0;
			char is_build_updates_selected    = strcmp(argv[1], "aur-buildupdates") == 0;
			char is_build_downgrades_selected = strcmp(argv[1], "aur-builddowngrades") == 0;
			char is_build_git_selected        = strcmp(argv[1], "aur-buildgit") == 0;
			char is_upgrade_selected          = strcmp(argv[1], "aur-upgrade") == 0;
			char is_upgrade_git_selected      = strcmp(argv[1], "aur-upgradegit") == 0;

			if (
				is_fetch_updates_selected ||
				is_fetch_downgrades_selected ||
				is_fetch_git_selected ||
				is_build_downgrades_selected ||
				is_build_git_selected ||
				is_build_updates_selected ||
				is_upgrade_selected ||
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
					aur_fetch_updates(pkg_namelist, actual_namelist_size, installed_pkgs_dict, excludes, n_excludes, fetch_mode, action);
				} else {
					aur_fetch_updates(pkg_namelist, actual_namelist_size, installed_pkgs_dict, NULL, 0, fetch_mode, action);
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
