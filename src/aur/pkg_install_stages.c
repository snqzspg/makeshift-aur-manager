#define _GNU_SOURCE

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "../arg_parse/arg_commands.h"
#include "../aur.h"
#include "../file_utils.h"
#include "../logger/logger.h"
#include "../pacman.h"
#include "../hashtable.h"
#include "../subprocess_unix.h"

#include "../aur_pkg_parse.h"

#include "pkg_cache.h"
#include "pkgver_cache.h"

#include "pkg_install_stages.h"

const char* exec_arg0 = NULL;

void fill_arg0(const char* arg0) {
	exec_arg0 = arg0;
}

size_t filter_pkg_updates(char** __restrict__ filtered_list_out, size_t filtered_list_limit, char **pkg_namelist, size_t pkg_namelist_len, hashtable_t installed_pkgs_dict, char **ignore_list, size_t ignore_list_len, enum __aur_fetch_mode fetch_type) {
	size_t pkg_count = 0;
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

		if (pkg_count < filtered_list_limit && filtered_list_out != NULL) {
			filtered_list_out[pkg_count] = pkg_namelist[i];
		}

		pkg_count++;
	}

	return pkg_count;
}

void aur_perform_action(char** pkgs, size_t pkg_count, hashtable_t installed_pkgs_dict, enum __aur_action action, char** pacman_opts, int n_pacman_opts, int reset) {
	if (action == FETCH) {
		for (size_t i = 0; i < pkg_count; i++) {
			struct hashtable_node* found_node = hashtable_find_inside_map(installed_pkgs_dict, pkgs[i]);
			char* pkgbase = found_node -> package_base == NULL ? pkgs[i] : found_node -> package_base;

			if (reset) {
				int r = update_existing_pkg_base(pkgbase);
				if (r != 0) {
					(void) syscall(SYS_exit, EXIT_FAILURE);
					return;
				}
			}
			(void) fetch_pkg_base(pkgbase);
		}
		download_package_namelist();
	}

	if (action == FETCH /* PATCH */) {
		if (!reset) {
			pacman_name_ver_t git_pkgs[pkg_count];
			char* new_ver_strs[pkg_count];
			(void) memset(new_ver_strs, 0, pkg_count * sizeof(char*));

			size_t git_pkgs_count = 0;

			for (size_t i = 0; i < pkg_count; i++) {
				struct hashtable_node* found_node = hashtable_find_inside_map(installed_pkgs_dict, pkgs[i]);
				char* pkgbase = found_node -> package_base == NULL ? pkgs[i] : found_node -> package_base;

				if (!found_node -> is_git_package) {
					continue;
				}

				git_pkgs[git_pkgs_count].name    = pkgs[i];
				git_pkgs[git_pkgs_count].valid   = 1;
				git_pkgs[git_pkgs_count].version = found_node -> updated_ver;

				update_existing_git_pkg_base(pkgbase);
				char* v = extract_existing_pkg_base_ver(pkgbase, 0);
				if (v != NULL) {
					new_ver_strs[git_pkgs_count] = (char*) malloc((strlen(v) + 1) * sizeof(char));
					(void) strncpy(new_ver_strs[git_pkgs_count], v, strlen(v) + 1);
					v = new_ver_strs[git_pkgs_count];
					git_pkgs[git_pkgs_count].version = v;
				}
				git_pkgs_count++;
			}

			(void) load_ver_cache();
			merge_in_ver_cache(git_pkgs, git_pkgs_count);
			save_ver_cache();
			discard_ver_cache();
			for (size_t i = 0; i < git_pkgs_count; i++) {
				free(new_ver_strs[i]);
			}
		}
	}

	if (action >= BUILD) {
		char* failed_pkgs[pkg_count];
		size_t failed_pkgs_count = 0;
		for (size_t i = 0; i < pkg_count; i++) {
			struct hashtable_node* found_node = hashtable_find_inside_map(installed_pkgs_dict, pkgs[i]);

			char* pkgbase = found_node -> package_base == NULL ? pkgs[i] : found_node -> package_base;

			if (!does_pkg_cache_exist() || !pkg_base_in_cache(found_node -> package_base)) {
				(void) note_printf(" the package base, '%s', of package '%s' is not found.\n", pkgbase, pkgs[i]);
				failed_pkgs[failed_pkgs_count++] = pkgs[i];
			}

			(void) build_existing_pkg_base(pkgbase);
		}

		if (failed_pkgs_count > 0) {
			(void) fprintf(stderr, "One or more packages are not fetched.\n");
			(void) fprintf(stderr, "Run with command '\033[1;32m%s aur-fetch ", exec_arg0);
			for (size_t i = 0; i < failed_pkgs_count; i++) {
				(void) fprintf(stderr, "%s%s", failed_pkgs[i], i == failed_pkgs_count - 1 ? "" : " ");
			}
			(void) fprintf(stderr, "\033[0m' to fetch the packages first, \033[1;32minspect the PKGBUILDs\033[0m, then re-run the build or install command.\n");
			return;
		}
	}

	if (action == INSTALL) {
		int n_pacman_args = pkg_count + n_pacman_opts + 4;
        char* pacman_args[n_pacman_args];
		pacman_args[0] = "sudo";
		pacman_args[1] = "pacman";
		pacman_args[2] = "-U";
		pacman_args[n_pacman_args - 1] = NULL;

		if (pacman_opts != NULL) {
			for (int i = 0; i < n_pacman_opts; i++) {
				pacman_args[i + 3] = pacman_opts[i];
			}
		}

		int pacman_args_count = n_pacman_opts + 3;

		for (size_t i = 0; i < pkg_count; i++) {
			struct hashtable_node* found_node = hashtable_find_inside_map(installed_pkgs_dict, pkgs[i]);
			char* pkgbase = found_node -> package_base == NULL ? pkgs[i] : found_node -> package_base;

			char* pkg_file = pkg_file_path_stream_alloc(pkgs[i], pkgbase, 0);
			if (pkg_file == NULL) {
				continue;
			}

			(void) debug_printf(" This package would be at \033[1;33m%s\033[0m.\n", pkg_file);

			pacman_args[pacman_args_count++] = pkg_file;
		}

		pacman_args[pacman_args_count] = NULL;

		(void) info_printf(" Use the command \033[1;32m");
		if (is_logging_type_enabled(INFO)) {
			for (size_t i = 0; i < pacman_args_count; i++) {
				(void) fprintf(stderr, "%s ", pacman_args[i]);
			}
			(void) fprintf(stderr, "\033[0mto install.\n");
		}

		if (pacman_args_count > n_pacman_opts + 3)
			(void) run_subprocess_v(NULL, "/usr/bin/sudo", (char**) pacman_args, NULL, STDIN_FILENO, NULL, 0, 1);

		for (size_t i = n_pacman_opts + 3; i < pacman_args_count; i++) {
			free(pacman_args[i]);
		}
	}
}

size_t filter_aur_pkgs(char** __restrict__ pkgs_out, size_t n_pkgs_limit, char** pkgs_in, size_t n_pkgs_in, hashtable_t pkgs_info, int quiet) {
	size_t pkgs_count = 0;

	for (size_t i = 0; i < n_pkgs_in; i++) {
		struct hashtable_node* found_node = hashtable_find_inside_map(pkgs_info, pkgs_in[i]);

		if (!found_node -> is_non_aur) {
			if (pkgs_count < n_pkgs_limit && pkgs_out != NULL) {
				pkgs_out[pkgs_count] = pkgs_in[i];
			}
			pkgs_count++;
		} else if (!quiet) {
			(void) note_printf(" '%s' is not found on the AUR.\n", pkgs_in[i]);
		}
	}

	return pkgs_count;
}

void fetch_aur_pkgs_from_args(int argc, char** argv, hashtable_t pkgs_info, int reset) {
	aur_perform_action(argv, argc, pkgs_info, FETCH, NULL, 0, reset);
}

void build_aur_pkgs_from_args(int argc, char** argv, hashtable_t pkgs_info) {
	aur_perform_action(argv, argc, pkgs_info, BUILD, NULL, 0, 0);
}

void install_aur_pkgs_from_args(int argc, char** argv, hashtable_t pkgs_info, char** pacman_opts, int n_pacman_opts) {
	// aur_perform_action(argv, argc, pkgs_info, BUILD, NULL, 0);
	aur_perform_action(argv, argc, pkgs_info, INSTALL, pacman_opts, n_pacman_opts, 0);
}

int pkg_list_manage_subseq(aurman_arg_command_t command, int argc, char** argv) {
	// size_t npkgs       = filter_pkgs_opts_args(NULL, 0, NULL, 0, argv, argc);
	// size_t npacmanopts = argc - npkgs;

	aur_install_options_t ins_opts;
	char* pkgs[argc];
	char* pacman_opts[argc];

	if (parse_aur_install_args(&ins_opts, pkgs, argc, pacman_opts, argc, argc, argv, command) != 0) {
		return 1;
	}

	// (void) filter_pkgs_opts_args(pkgs, npkgs, npacmanopts == 0 ? NULL : pacman_opts, npacmanopts, argv, argc);

	struct hashtable_node* map  [argc];
	struct hashtable_node  stash[argc];
	hashtable_t pkgs_info = hashtable_init(map, argc, stash, argc);

	for (size_t i = 0; i < ins_opts.n_pkgs; i++) {
		hashtable_set_item(&pkgs_info, pkgs[i], NULL, NULL, 1);
	}

	const char* response_json = get_packages_info((const char* const*) pkgs, ins_opts.n_pkgs);
	if (response_json != NULL) {
		size_t res_json_len = strlen(response_json);
		char mutable_res_json[res_json_len + 1];
		(void) strncpy(mutable_res_json, response_json, res_json_len + 1);
		clean_up_pkg_info_buffer();

		process_response_json(mutable_res_json, &pkgs_info);

		size_t n_aur_pkgs = filter_aur_pkgs(NULL, 0, pkgs, ins_opts.n_pkgs, pkgs_info, 1);
		char* aur_pkgs[n_aur_pkgs];
		(void) filter_aur_pkgs(aur_pkgs, n_aur_pkgs, pkgs, ins_opts.n_pkgs, pkgs_info, 0);

		// https://stackoverflow.com/questions/11076941/warning-case-not-evaluated-in-enumerated-type
		switch ((int) command) {
			case AUR_FETCH:
				fetch_aur_pkgs_from_args(n_aur_pkgs, aur_pkgs, pkgs_info, ins_opts.reset_pkgbuilds);
				break;
			case AUR_BUILD:
				build_aur_pkgs_from_args(n_aur_pkgs, aur_pkgs, pkgs_info);
				break;
			case AUR_INSTALL:
				install_aur_pkgs_from_args(n_aur_pkgs, aur_pkgs, pkgs_info, pacman_opts, ins_opts.n_pacman_opts);
				break;
		}

		return 0;
	}

	return 1;
}

void aur_fetch_updates(char **pkg_namelist, size_t pkg_namelist_len, hashtable_t installed_pkgs_dict, char **ignore_list, size_t ignore_list_len, char **pacman_opts, size_t n_pacman_opts, enum __aur_fetch_mode fetch_type, enum __aur_action action, int resets_pkgbuilds) {
	size_t pkg_count = filter_pkg_updates(NULL, 0, pkg_namelist, pkg_namelist_len, installed_pkgs_dict, ignore_list, ignore_list_len, fetch_type);

	char* filtered_list[pkg_count];

	(void) filter_pkg_updates(filtered_list, pkg_count, pkg_namelist, pkg_namelist_len, installed_pkgs_dict, ignore_list, ignore_list_len, fetch_type);

	aur_perform_action(filtered_list, pkg_count, installed_pkgs_dict, action, pacman_opts, n_pacman_opts, resets_pkgbuilds);
}
