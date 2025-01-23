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

int pkg_list_manage_subseq(const char* aur_cmd, int argc, char** argv);

#endif // PKG_INSTALL_STSGES_H_INCLUDED
