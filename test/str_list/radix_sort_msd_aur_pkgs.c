#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int aur_char_to_id(char c) {
	switch (c) {
		case '+':
			return 1;
		case '-':
			return 2;
		case '.':
			return 3;
		case '@':
			return 4;
		case '_':
			return 5;
	}

	if (c >= 48 && c < 58) {
		return (c & 15) + 6;
	}

	if ((c & 64) != 64) {
		return 0;
	}

	int alph = c & 31;
	if (alph < 27 && alph > 0) {
		return alph + 15;
	}

	return 0;
}

void dummy(size_t s) {
	(void) s;
}

#define radix_index 42
static void __radix_sort_rec(char** strs, size_t n_strs, size_t nth_char, size_t max_len) {
	if (n_strs < 2) {
		return;
	}

	if (nth_char >= max_len) {
		return;
	}

	char* buckets[radix_index][n_strs];
	size_t bucket_sizes[radix_index];

	(void) memset(bucket_sizes, 0, radix_index * sizeof(size_t));

	for (size_t i = 0; i < n_strs; i++) {
		size_t l = strlen(strs[i]);
		if (nth_char >= l) {
			buckets[0][bucket_sizes[0]++] = strs[i];
			continue;
		}

		int bucket_sel = aur_char_to_id(strs[i][nth_char]);
		buckets[bucket_sel][bucket_sizes[bucket_sel]++] = strs[i];
	}

	size_t c = 0;
	for (size_t i = 0; i < radix_index; i++) {
		__radix_sort_rec(buckets[i], bucket_sizes[i], nth_char + 1, max_len);

		for (size_t j = 0; j < bucket_sizes[i]; j++) {
			strs[c++] = buckets[i][j];
		}
	}
}

void radix_sort_aur_str(char** strs, size_t n_strs) {
	size_t max_len = 0;
	for (size_t i = 0; i < n_strs; i++) {
		size_t l = strlen(strs[i]);
		if (l > max_len) {
			max_len = l;
		}
	}

	__radix_sort_rec(strs, n_strs, 0, max_len);
}

int main(void) {
	int aurnames_fd = open("aur_pkg_names.txt", O_RDONLY);
	if (aurnames_fd < 0) {
		perror("[ERROR]");
		return EXIT_FAILURE;
	}

	size_t total_file_size = 0;
	size_t n_newlines = 0;
	char fragment[16384];
	ssize_t n_read = 0;
	do {
		n_read = read(aurnames_fd, fragment, 16384);
		total_file_size += n_read;
		for (ssize_t i = 0; i < n_read; i++) {
			if (fragment[i] == '\n') {
				n_newlines++;
			}
		}
	}
	while (n_read != 0);

	if (close(aurnames_fd) < 0) {
		perror("[ERROR]");
		return EXIT_FAILURE;
	}

	char file_contents[total_file_size + 1];

	aurnames_fd = open("aur_pkg_names.txt", O_RDONLY);
	if (aurnames_fd < 0) {
		perror("[ERROR]");
		return EXIT_FAILURE;
	}

	(void) read(aurnames_fd, file_contents, total_file_size);
	file_contents[total_file_size] = '\0';

	char* names[n_newlines];

	char* tok = strtok(file_contents, "\n");

	size_t names_count = 0;

	while (tok != NULL) {
		names[names_count++] = tok;
		tok = strtok(NULL, "\n");
	}

	radix_sort_aur_str(names, names_count);

	for (int i = 0; i < names_count; i++) {
		(void) printf("%s\n", names[i]);
	}

	return 0;
}
