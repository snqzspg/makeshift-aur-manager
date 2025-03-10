#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <curl/curl.h>

#include "aur/pkg_cache.h"
#include "file_utils.h"
#include "logger/logger.h"

#include "aur.h"

typedef struct {
	char *response;
	size_t size;
} recv_response_t;

static size_t receive_info(char *ptr, size_t size, size_t nmemb, void* userdata) {
	recv_response_t *resp = (recv_response_t *) userdata;
	size_t total_size = size * nmemb;

	if (resp -> response == NULL) {
		resp -> response = malloc(total_size + 1);
	} else {
		resp -> response = realloc(resp -> response, resp -> size + total_size + 1);
	}
	if (resp -> response == NULL) {
		error_perror("[fetching https://aur.archlinux.org/rpc/v5/info][receive_info]");
		return 0;
	}
	memcpy(resp -> response + resp -> size, ptr, total_size);
	resp -> size += total_size;
	resp -> response[resp -> size] = '\0';
	return nmemb;
}

recv_response_t pkg_info_response = {
	NULL,
	0
};

static size_t detemine_postfields_size(const char* const* packages, size_t n_packages) {
	size_t r = 0;
	for (size_t i = 0; i < n_packages; i++) {
		r += strlen(packages[i]);
	}
	r += n_packages * 7; // This accounted for the null terminating character.
	return r;
}

static void gen_postfields(char* __restrict__ postfields_out, const char* const* packages, size_t n_packages) {
	size_t p = 0;
	for (size_t i = 0; i < n_packages; i++) {
		postfields_out[p++] = 'a';
		postfields_out[p++] = 'r';
		postfields_out[p++] = 'g';
		postfields_out[p++] = '[';
		postfields_out[p++] = ']';
		postfields_out[p++] = '=';
		size_t l = strlen(packages[i]);
		(void) strncpy(postfields_out + p, packages[i], l);
		p += l;
		postfields_out[p++] = '&';
	}
	postfields_out[p - 1] = '\0';
}

const char* get_packages_info(const char* const* packages, size_t n_packages) {
	size_t post_content_size = detemine_postfields_size(packages, n_packages);
	char   post_content[post_content_size];

	gen_postfields(post_content, packages, n_packages);

	post_content[post_content_size - 1] = '\0';

	CURL* curl = curl_easy_init();
	if (curl == NULL) {
		return NULL;
	}
	CURLcode res;
	curl_easy_setopt(curl, CURLOPT_URL, "https://aur.archlinux.org/rpc/v5/info");
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_content);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, receive_info);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)(&pkg_info_response));

	res = curl_easy_perform(curl);

	if (res != CURLE_OK) {
		(void) error_printf(" Failed getting response from %s - %s\n", "https://aur.archlinux.org/rpc/v5/info", curl_easy_strerror(res));
		return NULL;
	}

	curl_easy_cleanup(curl);

	return pkg_info_response.response;
}

void clean_up_pkg_info_buffer() {
	if (pkg_info_response.response != NULL) {
		free(pkg_info_response.response);
		pkg_info_response.response = NULL;
		pkg_info_response.size     = 0;
	}
}

const char* aur_pkglist_file            = "__pkg_cache__/aur_package_names.txt";
const char* aur_pkglist_file_compressed = "__pkg_cache__/aur_package_names.txt.gz";

static size_t write_pkgs_gz(void* ptr, size_t size, size_t nmemb, void* stream) {
	int out_fd = *((int*) stream);

	return write(out_fd, ptr, size * nmemb);
}

int does_pkglist_exist() {
	return file_exists(aur_pkglist_file_compressed);
}

/**
 * Inefficient in terms of a lot of I/O operations.
 */
int download_package_namelist() {
	CURL* curl = curl_easy_init();
	if (curl == NULL) {
		return -1;
	}
	CURLcode res;
	curl_easy_setopt(curl, CURLOPT_URL, "https://aur.archlinux.org/packages.gz");
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_pkgs_gz);

	(void) debug_printf(" Creating file '%s'.\n", aur_pkglist_file_compressed);
	create_pkg_cache();
	int pkg_namelist_fd = open(aur_pkglist_file_compressed, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if (pkg_namelist_fd < 0) {
		(void) error_printf("[Updating package names] Creating file '%s' failed!\n", aur_pkglist_file_compressed);
		(void) error_perror("[Updating package names] ");
		curl_easy_cleanup(curl);
		return -1;
	}

	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)(&pkg_namelist_fd));

	(void) debug_printf(" Performing download from \"https://aur.archlinux.org/packages.gz\".\n");
	res = curl_easy_perform(curl);

	if (close(pkg_namelist_fd) < 0) {
		(void) error_printf("[Updating package name] Failed to close \"%s\" file: %s", aur_pkglist_file_compressed, strerror(errno));
	}

	if (res != CURLE_OK) {
		(void) error_printf(" Failed getting response from %s - %s\n", "https://aur.archlinux.org/packages.gz", curl_easy_strerror(res));
		return -1;
	}

	curl_easy_cleanup(curl);
	return 0;
}
