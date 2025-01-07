#include <stdio.h>

#include "aur.h"

char *sample_packages[] = {
	"dropbox",
	"zoom"
};

int main(void) {
	const char* response_json = get_packages_info((const char* const*) sample_packages, sizeof(sample_packages) / sizeof(char*));
	(void) printf("%s\n", response_json);
	clean_up_pkg_info_buffer();
	return 0;
}
