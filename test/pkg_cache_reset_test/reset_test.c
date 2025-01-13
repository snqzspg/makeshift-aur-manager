#include <stdio.h>
#include <stdlib.h>

#include <aur/pkg_cache.h>

char* pkgs[] = {
	"nordic-wallpapers-git",
	"graphite-gtk-theme-git"
};

int main(void) {
	(void) fprintf(stderr, "is_pwd_git_repo() returns %s.\n", is_pwd_git_repo() ? "\033[32mtrue\033[0m" : "\033[31mfalse\033[0m");

	git_reset_aur_pkgs(pkgs, sizeof(pkgs) / sizeof(char*));
	return 0;
}
