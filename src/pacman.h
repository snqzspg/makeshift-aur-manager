#ifndef PACMAN_H_INCLUDED
#define PACMAN_H_INCLUDED

typedef struct {
	char* name;
	char* version;
} pacman_name_ver_t;

typedef struct {
	pacman_name_ver_t* pkg_names_vers;
	size_t n_items;
} pacman_names_vers_t;

pacman_names_vers_t NULL_RET = {
    .n_items        = 0,
    .pkg_names_vers = NULL
};

#endif // PACMAN_H_INCLUDED