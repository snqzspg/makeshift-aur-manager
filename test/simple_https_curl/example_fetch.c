#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

typedef struct {
	char *response;
	size_t size;
} recv_response_t;

size_t receive_info(char *ptr, size_t size, size_t nmemb, void* userdata) {
    size_t data_size = size * nmemb;

    char data_str[data_size + 1];
    (void) memcpy(data_str, ptr, data_size);
    (void) fprintf(stderr, "%s\n", ptr);

	return data_size;
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
	curl_easy_setopt(curl, CURLOPT_URL, "https://example.com");
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

	return 0;
}