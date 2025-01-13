#include <stdio.h>
#include <stdlib.h>

#include <aur/pkg_cache.h>

int main(void) {
	(void) fprintf(stderr, "is_pwd_git_repo() returns %s.\n", is_pwd_git_repo() ? "\033[32mtrue\033[0m" : "\033[31mfalse\033[0m");
	return 0;
}
