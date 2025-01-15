#include <stdio.h>

#include <aur/pkg_cache.h>

char* pkgs[] = {
	"nordic-wallpapers-git",
	"graphite-gtk-theme-git"
};

int main(void) {
	git_clone_aur_pkgs(pkgs, sizeof(pkgs) / sizeof(char*));
	for (int i = 0; i < (sizeof(pkgs) / sizeof(char*)); i++) {
		(void) update_existing_git_pkg_base(pkgs[i]);
		char* pkgver = extract_existing_pkg_base_ver(pkgs[i], 0);
		(void) printf("%s: %s\n", pkgs[i], pkgver);
	}
	clean_up_extract_existing_pkg_base_ver();
	return 0;
}
