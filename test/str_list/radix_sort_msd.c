#include <stdio.h>
#include <string.h>

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

#define radix_index 42
static void __radix_sort_rec(char** __restrict__ strs, size_t n_strs, size_t nth_char, size_t max_len) {
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

char *test_list[] = {
	"wanyu",
	"emma",
	"sarah",
	"selina",
	"su_en",
	"hwee_ki",
	"laelia"
};
size_t tl_len = sizeof(test_list) / sizeof(char*);

int main(void) {
	(void) printf("Before sorting: \n");

	for (int i = 0; i < tl_len; i++) {
		(void) printf("%s\n", test_list[i]);
	}

	(void) printf("\n");

	radix_sort_aur_str(test_list, tl_len);

	(void) printf("After sorting: \n");

	for (int i = 0; i < tl_len; i++) {
		(void) printf("%s\n", test_list[i]);
	}

	return 0;
}
