#ifndef ARG_COMMANDS_H_INCLUDED
#define ARG_COMMANDS_H_INCLUDED

typedef enum {
    HELP,
    UPDATES_SUMMARY,
    UNKNOWN_COMMAND,

    AUR_FETCH,
    AUR_TRUST,
    AUR_PATCH,
    AUR_BUILD,
    AUR_INSTALL,

    AUR_CHECKUPDATES,
    AUR_FETCH_UPDATES,
    AUR_TRUST_UPDATES,
    AUR_PATCH_UPDATES,
    AUR_BUILD_UPDATES,
    AUR_INSTALL_UPDATES,

    AUR_CHECKDOWNGRADES,
    AUR_FETCH_DOWNGRADES,
    AUR_TRUST_DOWNGRADES,
    AUR_PATCH_DOWNGRADES,
    AUR_BUILD_DOWNGRADES,
    AUR_INSTALL_DOWNGRADES,

    AUR_CHECKUPDATES_GIT,
    AUR_FETCH_GIT,
    AUR_SYNC_GIT_VERSIONS,
    AUR_TRUST_GIT,
    AUR_PATCH_GIT,
    AUR_BUILD_GIT,
    AUR_INSTALL_GIT,

    PACMAN_CHECKUPDATES,
    PACMAN_UPGRADE,

    GEN_BASH_COMPLETION,
    GEN_ZSH_COMPLETION
} aurman_arg_command_t;

typedef struct {
    char** exclude_pkgs;
    char** pacman_opts;     // Install only
    int    n_exclude_pkgs;
    int    n_pacman_opts;   // Install only
    int    reset_pkgbuilds; // Fetch only
} aur_upgrade_options_t;

typedef struct {
    char** pkgs;
    char** pacman_opts;     // Install only
    int    n_pkgs;
    int    n_pacman_opts;   // Install only
    int    reset_pkgbuilds; // Fetch only
} aur_install_options_t;

aurman_arg_command_t get_command(const char* arg1);
int parse_aur_update_args(aur_upgrade_options_t* __restrict__ opts_out, char** __restrict__ exclude_pkgs, int excpkg_limit, char** __restrict__ pacman_opts, int pacman_opts_limit, int argc, char** argv, aurman_arg_command_t command);
int parse_aur_install_args(aur_install_options_t* __restrict__ opts_out, char** __restrict__ pkgs, int pkg_limit, char** __restrict__ pacman_opts, int pacman_opts_limit, int argc, char** argv, aurman_arg_command_t command);

#endif // ARG_COMMANDS_H_INCLUDED