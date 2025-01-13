#include <stdio.h>

#include <aur/pkg_cache.h>

char* pkgs[] = {
	"nordic-wallpapers-git",
	"graphite-gtk-theme-git"
};

int main(void) {
	git_clone_aur_pkgs(pkgs, sizeof(pkgs) / sizeof(char*));
	return 0;
}
