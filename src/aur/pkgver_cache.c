#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../file_utils.h"
#include "../logger/logger.h"
#include "../pacman.h"
#include "pkg_cache.h"

#include "pkgver_cache.h"

char* pkg_ver_cache_file = "__pkg_cache__/versions.txt";
int   pkg_ver_cache_file_len = 26;

pacman_names_vers_t ver_cache_loaded = {
    .pkg_names_vers = NULL,
    .n_items        = 0
};

streamed_content_t ver_cache_fdstream = {
    .content = NULL,
    .len     = 0
};

pacman_names_vers_t* load_ver_cache() {
    int fd = open(pkg_ver_cache_file, O_RDONLY);
	if (fd < 0) {
		if (errno == ENOENT) {
			(void) debug_printf(" '%s' does not exist - returning NULL to trigger creation of the file.", pkg_ver_cache_file);
			return &ver_cache_loaded;
		}
		(void) error_printf(" \033[1;31mOpening %s failed\033[0m! - %s\n", pkg_ver_cache_file, strerror(errno));
		return &ver_cache_loaded;
	}

    ver_cache_fdstream = stream_fd_content_alloc(fd);
    
	if (close(fd) < 0) {
		(void) error_printf(" \033[1;31mClosing %s failed\033[0m! - %s\n", pkg_ver_cache_file, strerror(errno));
	}

	if (ver_cache_fdstream.content == NULL) {
		discard_ver_cache();
		return &ver_cache_loaded;
	}

    ver_cache_loaded.n_items = process_pacman_output(NULL, 0, ver_cache_fdstream.content, ver_cache_fdstream.len);

    if (ver_cache_loaded.pkg_names_vers == NULL) {
        ver_cache_loaded.pkg_names_vers = malloc(ver_cache_loaded.n_items * sizeof(pacman_name_ver_t));
    } else {
        ver_cache_loaded.pkg_names_vers = realloc(ver_cache_loaded.pkg_names_vers, ver_cache_loaded.n_items * sizeof(pacman_name_ver_t));
    }
    if (ver_cache_loaded.pkg_names_vers == NULL) {
        error_perror("[load_ver_cache]");
        stream_fd_content_dealloc(&ver_cache_fdstream);
        return &ver_cache_loaded;
    }

    ver_cache_loaded.n_items = process_pacman_output(ver_cache_loaded.pkg_names_vers, ver_cache_loaded.n_items, ver_cache_fdstream.content, ver_cache_fdstream.len + 1);

    return &ver_cache_loaded;
}

void discard_ver_cache() {
    if (ver_cache_loaded.pkg_names_vers != NULL) {
        free(ver_cache_loaded.pkg_names_vers);
        ver_cache_loaded.n_items = 0;
    }

    if (ver_cache_fdstream.content != NULL) {
        stream_fd_content_dealloc(&ver_cache_fdstream);
    }
}

void save_ver_cache() {
    if (ver_cache_loaded.pkg_names_vers == NULL) {
        return;
    }

    (void) create_pkg_cache();

    FILE* f = fopen(pkg_ver_cache_file, "w");
	if (f == NULL) {
		(void) fprintf(stderr, "[ERROR] \033[1;31mOpening %s failed\033[0m! - %s\n", pkg_ver_cache_file, strerror(errno));
		return;
	}

    for (size_t i = 0; i < ver_cache_loaded.n_items; i++) {
        if (!ver_cache_loaded.pkg_names_vers[i].valid) {
            continue;
        }

        (void) fprintf(f, "%s %s\n", ver_cache_loaded.pkg_names_vers[i].name, ver_cache_loaded.pkg_names_vers[i].version);
    }

    if (fclose(f) < 0) {
		(void) fprintf(stderr, "[ERROR] \033[1;31mClosing %s failed\033[0m! - %s\n", pkg_ver_cache_file, strerror(errno));
	}
}

int pacman_nv_cmp(pacman_name_ver_t a, pacman_name_ver_t b) {
    return strcmp(a.name, b.name);
}

size_t zip_merge_sorted_list(pacman_name_ver_t* __restrict__ dest, size_t limit, pacman_name_ver_t* strs1, size_t n_strs1, pacman_name_ver_t* strs2, size_t n_strs2) {
	size_t n_added = 0;
	size_t i = 0, j = 0;

	while (i < n_strs1 && j < n_strs2) {
		int c = pacman_nv_cmp(strs1[i], strs2[j]);
		if (c < 0) {
			if (dest != NULL && n_added < limit) {
				dest[n_added] = strs1[i];
			}
			i++;
		} else if (c == 0) {
			if (dest != NULL && n_added < limit) {
				dest[n_added] = strs2[j];
			}
			i++;
			j++;
		} else {
			if (dest != NULL && n_added < limit) {
				dest[n_added] = strs2[j];
			}
			j++;
		}

		n_added++;
	}

	while (i < n_strs1) {
		if (dest != NULL && n_added < limit) {
			dest[n_added] = strs1[i];
		}
		i++;
		n_added++;
	}

	while (j < n_strs2) {
		if (dest != NULL && n_added < limit) {
			dest[n_added] = strs2[j];
		}
		j++;
		n_added++;
	}

	return n_added;
}

void merge_in_ver_cache(pacman_name_ver_t* sorted_pkgs, size_t n_items) {
    pacman_name_ver_t* new_cache = (pacman_name_ver_t* ) malloc((n_items + ver_cache_loaded.n_items) * sizeof(pacman_name_ver_t));

    if (ver_cache_loaded.pkg_names_vers == NULL) {
        ver_cache_loaded.pkg_names_vers = new_cache;
        ver_cache_loaded.n_items        = n_items;
		(void) memcpy(new_cache, sorted_pkgs, n_items * sizeof(pacman_name_ver_t));
        return;
    }

    ver_cache_loaded.n_items = zip_merge_sorted_list(new_cache, n_items + ver_cache_loaded.n_items, ver_cache_loaded.pkg_names_vers, ver_cache_loaded.n_items, sorted_pkgs, n_items);
    free(ver_cache_loaded.pkg_names_vers);
    ver_cache_loaded.pkg_names_vers = (pacman_name_ver_t*) realloc(new_cache, ver_cache_loaded.n_items * sizeof(pacman_name_ver_t));
}
