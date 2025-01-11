#include <stdio.h>

#include "aur.h"
#include "pacman.h"

char *sample_packages[] = {
	"dropbox",
	"zoom"
};

int main(void) {
	pacman_names_vers_t installed_pkgs = get_installed_non_pacman();

	for (size_t i = 0; i < installed_pkgs.n_items; i++) {
		if (installed_pkgs.pkg_names_vers[i].valid) {
			(void) printf("{.name = \"%s\", .version = \"%s\"}\n", installed_pkgs.pkg_names_vers[i].name, installed_pkgs.pkg_names_vers[i].version);
		}
	}

	const char* response_json = get_packages_info((const char* const*) sample_packages, sizeof(sample_packages) / sizeof(char*));
	(void) printf("%s\n", response_json);
	clean_up_pkg_info_buffer();
	return 0;
}
