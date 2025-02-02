#ifndef PKG_INFO_H_INCLUDED
#define PKG_INFO_H_INCLUDED

char* get_pkgbase_ver_alloc(const char* pkg_base);
char* get_pkg_file_from_pkg_alloc(const char* pkg_name, const char* pkg_base);

#endif // PKG_INFO_H_INCLUDED