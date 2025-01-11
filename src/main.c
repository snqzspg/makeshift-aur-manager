#include <stdio.h>

#include "aur.h"
#include "pacman.h"
#include "hashtable.h"

char *sample_packages[] = {
	"dropbox",
	"zoom"
};

int main(void) {
	pacman_names_vers_t installed_pkgs = get_installed_non_pacman();
	char* pkg_namelist[installed_pkgs.n_items];
	size_t actual_namelist_size = 0;

	struct hashtable_node* map  [installed_pkgs.n_items];
	struct hashtable_node  stash[installed_pkgs.n_items];
	hashtable_t installed_pkgs_dict = hashtable_init(map, installed_pkgs.n_items, stash, installed_pkgs.n_items);

	for (size_t i = 0; i < installed_pkgs.n_items; i++) {
		if (installed_pkgs.pkg_names_vers[i].valid) {
			hashtable_set_item(&installed_pkgs_dict, installed_pkgs.pkg_names_vers[i].name, installed_pkgs.pkg_names_vers[i].version, NULL, 0);
			pkg_namelist[actual_namelist_size] = installed_pkgs.pkg_names_vers[i].name;
			actual_namelist_size++;
		}
	}

	for (size_t i = 0; i < installed_pkgs.n_items; i++) {
		if (installed_pkgs.pkg_names_vers[i].valid) {
			struct hashtable_node* found_node = hashtable_find_inside_map(installed_pkgs_dict, installed_pkgs.pkg_names_vers[i].name);
			(void) printf("{.name = \"%s\", .version = \"%s\"}\n", installed_pkgs.pkg_names_vers[i].name, found_node -> installed_ver);
		}
	}

	const char* response_json = get_packages_info((const char* const*) pkg_namelist, actual_namelist_size);
	(void) printf("%s\n", response_json);
	clean_up_pkg_info_buffer();
	clean_up_pacman_output();
	return 0;
}
