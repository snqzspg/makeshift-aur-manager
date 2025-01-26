#ifndef AUR_C_INCLUDED
#define AUR_C_INCLUDED

const char* get_packages_info(const char* const* packages, size_t n_packages);
void clean_up_pkg_info_buffer();

int download_package_namelist();
int does_pkglist_exist();

#endif // AUR_C_INCLUDED