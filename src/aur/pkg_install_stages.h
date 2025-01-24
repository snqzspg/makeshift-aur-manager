#ifndef PKG_INSTALL_STSGES_H_INCLUDED
#define PKG_INSTALL_STSGES_H_INCLUDED

enum __aur_fetch_mode {
	NON_GIT_UPGRADES,
	NON_GIT_DOWNGRADES,
	GIT
};

enum __aur_action {
	FETCH,
	TRUST,
	PATCH,
	BUILD,
	INSTALL
};

size_t filter_pkg_updates(pacman_name_ver_t* __restrict__ filtered_list_out, size_t filtered_list_limit, char **pkg_namelist, size_t pkg_namelist_len, hashtable_t installed_pkgs_dict, char **ignore_list, size_t ignore_list_len, enum __aur_fetch_mode fetch_type);

int pkg_list_manage_subseq(const char* aur_cmd, int argc, char** argv);

#endif // PKG_INSTALL_STSGES_H_INCLUDED
