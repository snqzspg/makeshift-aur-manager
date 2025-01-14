#include <stdio.h>
#include <string.h>

size_t zip_merge_sorted_list(char** __restrict__ dest, size_t limit, char** strs1, size_t n_strs1, char** strs2, size_t n_strs2) {
	size_t n_added = 0;
	size_t i = 0, j = 0;

	while (i < n_strs1 && j < n_strs2) {
		int c = strcmp(strs1[i], strs2[j]);
		if (c < 0) {
			if (dest != NULL && n_added < limit) {
				dest[n_added] = strs1[i];
			}
			i++;
		} else if (c == 0) {
			if (dest != NULL && n_added < limit) {
				dest[n_added] = strs1[i];
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

char* l1[] = {
	"a",
	"b",
	"c",
	"d",
	"f",
	"g",
	"k",
	"l",
	"m"
};

char* l2[] = {
	"a",
	"b",
	"d",
	"e",
	"f",
	"h",
	"i",
	"j",
	"k"
};

int main(void) {
	size_t n = zip_merge_sorted_list(NULL, 0, l1, sizeof(l1) / sizeof(char**), l2, sizeof(l2) / sizeof(char**));
	(void) printf("%zu (expected 13)\n", n);
	char* merged[n];
	(void) zip_merge_sorted_list(merged, n, l1, sizeof(l1) / sizeof(char**), l2, sizeof(l2) / sizeof(char**));

	for (int i = 0; i < n; i++) {
		(void) printf("%s\n", merged[i]);
	}
	return 0;
}
