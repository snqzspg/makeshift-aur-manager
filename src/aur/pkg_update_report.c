#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <alpm.h>

#include "../arg_parse/arg_commands.h"
#include "../aur.h"
#include "../file_utils.h"
#include "../logger/logger.h"
#include "../pacman.h"
#include "../hashtable.h"
#include "pkg_cache.h"
#include "pkg_install_stages.h"

#include "pkgver_cache.h"

#include "pkg_update_report.h"

static void print_aur_pkgnames_completion_status() {
	if (does_pkglist_exist()) {
		return;
	}

	(void) printf(
		"Package names command line completions are not downloaded. "
		"You can run '\033[1;32m%s aur-fetch\033[0m' or any fetch commands to download the list.\n\n"
	, exec_arg0);
}

void aur_list_git(char **pkg_namelist, size_t pkg_namelist_len, hashtable_t installed_pkgs_dict) {
	int git_pkg_available = 0;
	pacman_names_vers_t* ver_cache = load_ver_cache();
	for (size_t i = 0, j = 0; i < pkg_namelist_len; i++) {
		struct hashtable_node* found_node = hashtable_find_inside_map(installed_pkgs_dict, pkg_namelist[i]);

		if (found_node -> is_non_aur || !found_node -> is_git_package) {
			continue;
		}

		if (!git_pkg_available)
			(void) printf( 
				"--- \033[1;36mAUR Git Packages\033[0m ---\n"
				"\033[1;34mUse '\033[1;32m%s aur-fetchgit\033[1;34m', inspect the \033[1;32mPKGBUILDs\033[1;34m, then use '\033[1;32m%s aur-upgradegit\033[1;34m' to update.\033[0m\n\n"
			, exec_arg0, exec_arg0);
			
		git_pkg_available = 1;

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

		int  vercmp = alpm_pkg_vercmp(found_node -> installed_ver, revised_updated_ver);
		enum update_stat revised_update_type = vercmp == 0 ? UP_TO_DATE : (vercmp < 0 ? UPGRADE : DOWNGRADE);

		char *col   = revised_update_type == UPGRADE ? "\033[32m" : (revised_update_type == DOWNGRADE ? "\033[31m" : "\033[2m");
		char *arrow = revised_update_type == UPGRADE ? "->" : (revised_update_type == DOWNGRADE ? "<-" : "==");
		(void) printf("%s %s\033[1m%s\033[0m %s %s\033[1m%s\033[0m\n", pkg_namelist[i], col, found_node -> installed_ver, arrow, col, revised_updated_ver);
	}
	clean_up_extract_existing_pkg_base_ver();
	discard_ver_cache();
}

void aur_check_non_git_downgrades(char **pkg_namelist, size_t pkg_namelist_len, hashtable_t installed_pkgs_dict) {
	int downgrade_present = 0;
	for (size_t i = 0; i < pkg_namelist_len; i++) {
		struct hashtable_node* found_node = hashtable_find_inside_map(installed_pkgs_dict, pkg_namelist[i]);

		if (found_node -> is_non_aur || found_node -> is_git_package || found_node -> update_type != DOWNGRADE) {
			continue;
		}

		if (!downgrade_present)
			(void) printf(
				"--- \033[1;36mAUR \033[31mDowngrades\033[0m ---\n"
				"\033[1;34mUse '\033[1;31m%s aur-fetchdowngrades\033[1;34m', inspect the \033[1;32mPKGBUILDs\033[1;34m, then use \033[2m'\033[31m%s aur-downgrade\033[34m' (WIP)\033[22;1m to downgrade.\033[0m\n\n"
			, exec_arg0, exec_arg0);

		downgrade_present = 1;

		char *col   = found_node -> update_type == UPGRADE ? "\033[32m" : (found_node -> update_type == DOWNGRADE ? "\033[31m" : "");
		char *arrow = found_node -> update_type == UPGRADE ? "->" : (found_node -> update_type == DOWNGRADE ? "<-" : "==");
		(void) printf("%s %s\033[1m%s\033[0m %s %s\033[1m%s\033[0m\n", pkg_namelist[i], col, found_node -> installed_ver, arrow, col, found_node -> updated_ver);
	}
	(void) fflush(stdout);

	if (downgrade_present)
		(void) write(STDOUT_FILENO, "\n", 1);
}

void aur_check_non_git(char **pkg_namelist, size_t pkg_namelist_len, hashtable_t installed_pkgs_dict) {
	int upgrade_present = 0;
	for (size_t i = 0; i < pkg_namelist_len; i++) {
		struct hashtable_node* found_node = hashtable_find_inside_map(installed_pkgs_dict, pkg_namelist[i]);

		if (found_node -> is_non_aur || found_node -> is_git_package || found_node -> update_type != UPGRADE) {
			continue;
		}

		if (!upgrade_present) 
			(void) printf(
				"--- \033[1;36mAUR Updates\033[0m ---\n"
				"\033[1;34mUse '\033[1;32m%s aur-fetchupdates\033[1;34m', inspect the \033[1;32mPKGBUILDs\033[1;34m, then use '\033[1;32m%s aur-upgrade\033[1;34m' to update.\033[0m\n\n"
			, exec_arg0, exec_arg0);

		upgrade_present = 1;

		char *col   = found_node -> update_type == UPGRADE ? "\033[32m" : (found_node -> update_type == DOWNGRADE ? "\033[31m" : "");
		char *arrow = found_node -> update_type == UPGRADE ? "->" : (found_node -> update_type == DOWNGRADE ? "<-" : "==");
		(void) printf("%s %s\033[1m%s\033[0m %s %s\033[1m%s\033[0m\n", pkg_namelist[i], col, found_node -> installed_ver, arrow, col, found_node -> updated_ver);
	}
	(void) fflush(stdout);

	if (upgrade_present)
		(void) write(STDOUT_FILENO, "\n", 1);
}

void perform_updates_summary_report(char **pkg_namelist, size_t pkg_namelist_len, hashtable_t installed_pkgs_dict) {
	print_aur_pkgnames_completion_status();
	aur_check_non_git(pkg_namelist, pkg_namelist_len, installed_pkgs_dict);
	aur_check_non_git_downgrades(pkg_namelist, pkg_namelist_len, installed_pkgs_dict);
	aur_list_git(pkg_namelist, pkg_namelist_len, installed_pkgs_dict);
	(void) perform_pacman_checkupdates();

	// if (!does_pkglist_exist()) {
	// 	download_package_namelist();
	// }
}
