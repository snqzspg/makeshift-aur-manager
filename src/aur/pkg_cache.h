#ifndef PKG_CACHE_H_INCLUDED
#define PKG_CACHE_H_INCLUDED

int create_pkg_cache();

char does_pkg_cache_exist();
char pkg_base_in_cache(const char* pkg_base);

size_t write_pkg_file_path(char* __restrict__ dest, size_t limit, const char* pkg_name, const char* pkg_base, char quiet);
char* pkg_file_path_stream_alloc(const char* pkg_name, const char* pkg_base, char quiet);

int fetch_pkg_base(const char* pkg_base);

char is_pwd_git_repo();

int fetch_pkg_base(const char* pkg_base);
void git_clone_aur_pkgs(char** pkg_bases, size_t n_pkg_bases);

/**
 * WARNING: A potentially destructive function when tested inside
 *          a git repo!!
 */
void git_reset_aur_pkgs(char** pkg_bases, size_t n_pkg_bases);

int build_existing_pkg_base(const char* pkg_base);
void build_aur_pkgs(char** pkg_bases, size_t n_pkg_bases);

char* extract_existing_pkg_base_ver(const char* pkg_base, char quiet);
void clean_up_extract_existing_pkg_base_ver();

void extract_ver_from_pkgs_muti_proc(char** __restrict__ s_out, size_t* __restrict__ slens_out, const char* pkg_bases[], size_t n_items);

int update_existing_git_pkg_base(const char* pkg_base);

#endif // PKG_CACHE_H_INCLUDED