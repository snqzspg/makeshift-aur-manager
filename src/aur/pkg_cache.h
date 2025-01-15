#ifndef PKG_CACHE_H_INCLUDED
#define PKG_CACHE_H_INCLUDED

int create_pkg_cache();

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

char* extract_existing_pkg_base_ver(const char* pkg_base, char quiet);
void clean_up_extract_existing_pkg_base_ver();

int update_existing_git_pkg_base(const char* pkg_base);

#endif // PKG_CACHE_H_INCLUDED