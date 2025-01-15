#ifndef PKGVER_CACHE_H_INCLUDED
#define PKGVER_CACHE_H_INCLUDED

extern pacman_names_vers_t ver_cache_loaded;
extern streamed_content_t ver_cache_fdstream;

pacman_names_vers_t* load_ver_cache();
void discard_ver_cache();
void save_ver_cache();
void merge_in_ver_cache(pacman_name_ver_t* sorted_pkgs, size_t n_items);

#endif // PKGVER_CACHE_H_INCLUDED