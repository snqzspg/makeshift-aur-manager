#ifndef PKG_CACHE_H_INCLUDED
#define PKG_CACHE_H_INCLUDED

int fetch_pkg_base(const char* pkg_base);

char is_pwd_git_repo();

int fetch_pkg_base(const char* pkg_base);
void git_clone_aur_pkgs(char** pkg_bases, size_t n_pkg_bases);

/**
 * WARNING: A potentially destructive function when tested inside
 *          a git repo!!
 */
void git_reset_aur_pkgs(char** pkg_bases, size_t n_pkg_bases);

void build_aur_pkgs(char** pkg_bases, size_t n_pkg_bases);

#endif // PKG_CACHE_H_INCLUDED