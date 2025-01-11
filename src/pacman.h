#ifndef PACMAN_H_INCLUDED
#define PACMAN_H_INCLUDED

typedef struct {
	char* name;
	char* version;
	char  valid;
} pacman_name_ver_t;

typedef struct {
	pacman_name_ver_t* pkg_names_vers;
	size_t n_items;
} pacman_names_vers_t;

pacman_names_vers_t PACMAN_PKGS_NULL_RET = {
    .n_items        = 0,
    .pkg_names_vers = NULL
};

pacman_names_vers_t get_installed_non_pacman();
void clean_up_pacman_output(void);

#endif // PACMAN_H_INCLUDED