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

extern const char* exec_arg0;

void fill_arg0(const char* arg0);

size_t filter_pkg_updates(char** __restrict__ filtered_list_out, size_t filtered_list_limit, char **pkg_namelist, size_t pkg_namelist_len, hashtable_t installed_pkgs_dict, char **ignore_list, size_t ignore_list_len, enum __aur_fetch_mode fetch_type);

int pkg_list_manage_subseq(const char* aur_cmd, int argc, char** argv, int fetch_resets_pkgs);

void aur_fetch_updates(char **pkg_namelist, size_t pkg_namelist_len, hashtable_t installed_pkgs_dict, char **ignore_list, size_t ignore_list_len, char **pacman_opts, size_t n_pacman_opts, enum __aur_fetch_mode fetch_type, enum __aur_action action, int resets_pkgbuilds);

#endif // PKG_INSTALL_STSGES_H_INCLUDED
