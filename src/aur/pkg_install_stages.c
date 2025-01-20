#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../pacman.h"
#include "../hashtable.h"

#include "pkgver_cache.h"

#include "pkg_install_stages.h"

size_t filter_pkg_updates(pacman_name_ver_t* __restrict__ filtered_list_out, size_t filtered_list_limit, char **pkg_namelist, size_t pkg_namelist_len, hashtable_t installed_pkgs_dict, char **ignore_list, size_t ignore_list_len, enum __aur_fetch_mode fetch_type) {
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
			filtered_list_out[pkg_count].name    = pkg_namelist[i];
			filtered_list_out[pkg_count].valid   = 1;
			filtered_list_out[pkg_count].version = found_node -> updated_ver;
		}

		pkg_count++;
	}

	return pkg_count;
}

void aur_fetch_updates(pacman_name_ver_t *pkgs, size_t pkg_count, hashtable_t installed_pkgs_dict, char **ignore_list, size_t ignore_list_len, enum __aur_fetch_mode fetch_type, enum __aur_action action) {
	char* new_ver_strs[pkg_count];

	(void) memset(new_ver_strs, 0, pkg_count * sizeof(char*));

	if (action >= FETCH) {
		for (size_t i = 0; i < pkg_count; i++) {
			struct hashtable_node* found_node = hashtable_find_inside_map(installed_pkgs_dict, pkgs[i].name);
			char* pkgbase = found_node -> package_base == NULL ? pkgs[i].name : found_node -> package_base;

			(void) fetch_pkg_base(pkgbase);
		}
	}

	if (action >= FETCH /* PATCH */) {
		if (fetch_type == GIT) {
			for (size_t i = 0; i < pkg_count; i++) {
				struct hashtable_node* found_node = hashtable_find_inside_map(installed_pkgs_dict, pkgs[i].name);
				char* pkgbase = found_node -> package_base == NULL ? pkgs[i].name : found_node -> package_base;

				update_existing_git_pkg_base(pkgbase);
				char* v = extract_existing_pkg_base_ver(pkgbase, 0);
				if (v != NULL) {
					new_ver_strs[i] = (char*) malloc((strlen(v) + 1) * sizeof(char));
					(void) strncpy(new_ver_strs[i], v, strlen(v) + 1);
					v = new_ver_strs[i];
					pkgs[i].version = v;
				}
			}

			(void) load_ver_cache();
			merge_in_ver_cache(pkgs, pkg_count);
			save_ver_cache();
			discard_ver_cache();
			for (size_t i = 0; i < pkg_count; i++) {
				free(new_ver_strs[i]);
			}
		}
	}

	if (action == BUILD) {
		for (size_t i = 0; i < pkg_count; i++) {
			struct hashtable_node* found_node = hashtable_find_inside_map(installed_pkgs_dict, pkgs[i].name);

			char* pkgbase = found_node -> package_base == NULL ? pkgs[i].name : found_node -> package_base;

			(void) build_existing_pkg_base(pkgbase);
		}
	}

	if (action == INSTALL) {
		// (void) fprintf(stderr, "[INFO] Use the command \033[1;32msudo pacman -U ");
		for (size_t i = 0; i < pkg_count; i++) {
			struct hashtable_node* found_node = hashtable_find_inside_map(installed_pkgs_dict, pkgs[i].name);
			char* pkgbase = found_node -> package_base == NULL ? pkgs[i].name : found_node -> package_base;

			size_t pkg_path_len = write_pkg_file_path(NULL, 0, pkgs[i].name, pkgbase, 0);
			if (pkg_path_len == 0) {
				continue;
			}
			char pkg_path[pkg_path_len + 1];

			(void) write_pkg_file_path(pkg_path, pkg_path_len + 1, pkgs[i].name, pkgbase, 0);

			// (void) fprintf(stderr, "'%s' ", pkg_path);
			(void) fprintf(stderr, "[DEBUG] This package would be at \033[1;33m%s\033[0m.\n", pkg_path);
		}
		// (void) fprintf(stderr, "to install.\033[0m\n");
	}
}
