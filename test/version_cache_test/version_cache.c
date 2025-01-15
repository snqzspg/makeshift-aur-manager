#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <pacman.h>
#include <file_utils.h>

#include <aur/pkgver_cache.h>

pacman_name_ver_t sample[] = {
	{.name = "abc", .version = "1.2.3", .valid = 1},
	{.name = "emma", .version = "2.1", .valid = 1},
	{.name = "wanjuin", .version = "4", .valid = 1}
};

int main(void) {
	pacman_names_vers_t* cache_list = load_ver_cache();

	for (int i = 0; i < cache_list -> n_items; i++) {
		if (cache_list -> pkg_names_vers[i].valid)
			(void) printf("%s %s\n", cache_list -> pkg_names_vers[i].name, cache_list -> pkg_names_vers[i].version);
	}
	
	merge_in_ver_cache(sample, sizeof(sample) / sizeof(sample[0]));

	save_ver_cache();

	discard_ver_cache();

	return 0;
}
