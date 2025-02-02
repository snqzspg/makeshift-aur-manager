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

extern pacman_names_vers_t PACMAN_PKGS_NULL_RET;

/**
 * pacman_output is only mutated if pacman_objs_out is not NULL.
 */
size_t process_pacman_output(pacman_name_ver_t* __restrict__ pacman_objs_out, size_t pacman_objs_size, char* pacman_output, size_t pacman_output_len);

pacman_names_vers_t get_installed_non_pacman();
void clean_up_pacman_output(void);

int perform_pacman_checkupdates();
int perform_pacman_upgrade(int argc, char** argv);

// int compare_versions(const char* ver1, const char* ver2);

#endif // PACMAN_H_INCLUDED