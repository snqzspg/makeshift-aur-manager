#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <file_utils.h>

int main(void) {
	int aurnames_fd = open("aur_pkg_names.txt", O_RDONLY);
	if (aurnames_fd < 0) {
		perror("[ERROR]");
		return EXIT_FAILURE;
	}

	streamed_content_t file_stream = stream_fd_content_alloc(aurnames_fd);

	(void) printf("%s\n", file_stream.content);

	stream_fd_content_dealloc(&file_stream);

	return 0;
}
