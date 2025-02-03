#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../file_utils.h"
#include "../logger/logger.h"
#include "../subprocess_unix.h"

#include "pkg_cache.h"

char* pkg_dest           = NULL;
int   need_free_pkg_dest = 0;

char* get_pkgbase_ver_alloc(const char* pkg_base) {
	if (!does_pkg_cache_exist() || !pkg_base_in_cache(pkg_base)) {
		return NULL;
	}

	size_t dir_len = write_pkg_base_path(NULL, 0, pkg_base);
	char   dir[dir_len + 1];
	(void) write_pkg_base_path(dir, dir_len + 1, pkg_base);

	int ver_out_fd;
	int r = run_subprocess(dir, 0, 1, &ver_out_fd, STDIN_FILENO, NULL, "/usr/bin/bash", "bash", "-c",
		"source /usr/share/makepkg/util/pkgbuild.sh;"
		"source /etc/makepkg.conf;"
		"source PKGBUILD;"
		"get_full_version"
	, NULL);
	if (r != 0) {
		return NULL;
	}

	streamed_content_t ver_stream = stream_fd_content_alloc(ver_out_fd);
	ver_stream.content[ver_stream.len - 1] = '\0';
	return stream_fd_content_detach_str(&ver_stream);
}

char* get_pkg_file_from_pkg_alloc(const char* pkg_name, const char* pkg_base) {
	if (!does_pkg_cache_exist() || !pkg_base_in_cache(pkg_base)) {
		return NULL;
	}

	size_t pkgbase_path_len = write_pkg_base_path(NULL, 0, pkg_base);
	char pkgbase_path[pkgbase_path_len + 1];
	write_pkg_base_path(pkgbase_path, pkgbase_path_len + 1, pkg_base);

	char bash_scr_fmt[] = 
		"source /usr/share/makepkg/util/pkgbuild.sh;"
		"source /etc/makepkg.conf;"
		"source PKGBUILD;"
		"if [ -z \"$PKGDEST\" ];"
		"then "
			"echo \"%s%s%s-$(get_full_version)-$(get_pkg_arch)${PKGEXT}\"; "
		"else "
			"echo \"$PKGDEST/%s-$(get_full_version)-$(get_pkg_arch)${PKGEXT}\";"
		"fi";
	
	size_t script_len = snprintf(NULL, 0, bash_scr_fmt, pkgbase_path, "/", pkg_name, pkg_name);
	char bash_scr[script_len + 1];
	(void) snprintf(bash_scr, script_len + 1, bash_scr_fmt, pkgbase_path, "/", pkg_name, pkg_name);

	int ver_out_fd;
	int r = run_subprocess(pkgbase_path, 0, 1, &ver_out_fd, STDIN_FILENO, NULL, "/usr/bin/bash", "bash", "-c", bash_scr, NULL);
	if (r != 0) {
		return NULL;
	}

	streamed_content_t out_stream = stream_fd_content_alloc(ver_out_fd);
	if (out_stream.len == 0) {
		stream_fd_content_dealloc(&out_stream);
		return NULL;
	}
	out_stream.content[out_stream.len - 1] = '\0';
	return stream_fd_content_detach_str(&out_stream);
}

int init_pkgdest() {
	char* env_pkgdest = getenv("PKGDEST");
	if (env_pkgdest != NULL) {
		if (need_free_pkg_dest) {
			free(pkg_dest);
			need_free_pkg_dest = 0;
		}
		pkg_dest = env_pkgdest;
		return 0;
	}

	int stream_out_fd;
	int r = run_subprocess(NULL, 0, 1, &stream_out_fd, STDIN_FILENO, NULL, "/usr/bin/bash", "bash", "-c", 
		"source /usr/share/makepkg/util/pkgbuild.sh;"
		"source /etc/makepkg.conf;"
	, NULL);

	if (r != 0) {
		return -1;
	}

	streamed_content_t out_stream = stream_fd_content_alloc(stream_out_fd);
	out_stream.content[out_stream.len - 1] = '\0';
	pkg_dest = stream_fd_content_detach_str(&out_stream);
	need_free_pkg_dest = 1;

	return 0;
}

/**
 * Is NULL if no custom PKGDEST is set.
 */
const char* get_pkg_dest() {
	return pkg_dest;
}

void deinit_pkgdest() {
	if (need_free_pkg_dest) {
		free(pkg_dest);
		need_free_pkg_dest = 0;
		pkg_dest = NULL;
	}
}
