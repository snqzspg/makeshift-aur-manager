#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

typedef struct {
	char *response;
	size_t size;
} recv_response_t;

size_t receive_info(char *ptr, size_t size, size_t nmemb, void* userdata) {
	recv_response_t *resp = (recv_response_t *) userdata;
	size_t total_size = size * nmemb;

	if (resp -> response == NULL) {
		resp -> response = malloc(total_size + 1);
	} else {
		resp -> response = realloc(resp -> response, resp -> size + total_size + 1);
	}
	if (resp -> response == NULL) {
		perror("[ERROR]");
		return 0;
	}
	memcpy(resp -> response + resp -> size, ptr, total_size);
	resp -> size += total_size;
	resp -> response[resp -> size] = '\0';
	return nmemb;
}

int main(void) {
	char *content_buffer = NULL;
	recv_response_t response = {
		content_buffer,
		0
	};
	CURL* curl = curl_easy_init();
	if (curl == NULL) {
		return 1;
	}
	CURLcode res;
	curl_easy_setopt(curl, CURLOPT_URL, "https://aur.archlinux.org/rpc/v5/info");
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "identity");
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, receive_info);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)(&response));
	res = curl_easy_perform(curl);
	if (res == CURLE_OK) {
		char *content_type;
		res = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &content_type);
		if (res == CURLE_OK && content_type != NULL) {
			printf("Content type: %s\n", content_type);
		}
	}

	(void) fprintf(stderr, "%s\n", response.response);

	return 0;
}