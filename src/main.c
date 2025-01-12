#include <stdio.h>
#include <string.h>

#include "aur.h"
#include "pacman.h"
#include "hashtable.h"

#include "aur_pkg_parse.h"

int main(int argc, char** argv) {
	if (argc > 1) {
		if (strcmp(argv[1], "pacman-checkupdates") == 0) {
			return perform_pacman_checkupdates();
		}
		if (strcmp(argv[1], "pacman-upgrade") == 0) {
			return perform_pacman_upgrade(argc - 2, argv + 2);
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
	// (void) printf("%s\n", response_json);

	size_t res_json_len = strlen(response_json);
	char mutable_res_json[res_json_len + 1];
	(void) strncpy(mutable_res_json, response_json, res_json_len + 1);
	clean_up_pkg_info_buffer();

	process_response_json(mutable_res_json, &installed_pkgs_dict);

	for (size_t i = 0; i < installed_pkgs.n_items; i++) {
		if (installed_pkgs.pkg_names_vers[i].valid) {
			struct hashtable_node* found_node = hashtable_find_inside_map(installed_pkgs_dict, installed_pkgs.pkg_names_vers[i].name);
			(void) printf("{.name = \"%s\", .version = \"%s%s%s\", .is_git = %s}\n", installed_pkgs.pkg_names_vers[i].name, found_node -> installed_ver, found_node -> is_non_aur ? "" : "\", .aur_ver = \"", found_node -> is_non_aur ? "" : found_node -> updated_ver, found_node -> is_git_package ? "true": "false");
		}
	}
	
	clean_up_pacman_output();
	return 0;
}
