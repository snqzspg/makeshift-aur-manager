#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int file_exists(const char* fname) {
	struct stat sb;

	int r = stat(fname, &sb);
	if (r >= 0) {
		return 1;
	}
	if (errno == ENOENT) {
		return 0;
	}
	// Warn unknown error occured.
	return 0;
}

int main(int argc, char** argv) {
	if (argc < 2) {
		(void) fprintf(stderr, "usage: %s filename\n", argv[0]);
		return 1;
	}

	(void) printf("File '%s' exist? %s\n", argv[1], file_exists(argv[1]) ? "true" : "false");
	return 0;
}