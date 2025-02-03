#ifndef PKG_INFO_H_INCLUDED
#define PKG_INFO_H_INCLUDED

char* get_pkgbase_ver_alloc(const char* pkg_base);
char* get_pkg_file_from_pkg_alloc(const char* pkg_name, const char* pkg_base);

int init_pkgdest();

/**
 * Is NULL if no custom PKGDEST is set.
 */
const char* get_pkg_dest();
void deinit_pkgdest();

#endif // PKG_INFO_H_INCLUDED