#include <string.h>

#include "arg_commands.h"
#include "../logger/logger.h"

aurman_arg_command_t get_command(const char* arg1) {
	if (arg1 == NULL) {
		return UPDATES_SUMMARY;
	}
	if (strcmp(arg1, "help") == 0) {
		return HELP;
	}
	if (strcmp(arg1, "gen-bashcomplete") == 0) {
		return GEN_BASH_COMPLETION;
	}
	if (strcmp(arg1, "gen-zshcomplete") == 0) {
		return GEN_ZSH_COMPLETION;
	}

	if (strcmp(arg1, "aur-fetch") == 0) {
		return AUR_FETCH;
	}
	if (strcmp(arg1, "aur-trust") == 0) {
		return AUR_TRUST;
	}
	if (strcmp(arg1, "aur-patch") == 0) {
		return AUR_PATCH;
	}
	if (strcmp(arg1, "aur-build") == 0) {
		return AUR_BUILD;
	}
	if (strcmp(arg1, "aur-install") == 0) {
		return AUR_INSTALL;
	}

	if (strcmp(arg1, "aur-fetchupdates") == 0) {
		return AUR_FETCH_UPDATES;
	}
	if (strcmp(arg1, "aur-trustupdates") == 0) {
		return AUR_TRUST_UPDATES;
	}
	if (strcmp(arg1, "aur-patchupdates") == 0) {
		return AUR_PATCH_UPDATES;
	}
	if (strcmp(arg1, "aur-buildupdates") == 0) {
		return AUR_BUILD_UPDATES;
	}
	if (strcmp(arg1, "aur-upgrade") == 0) {
		return AUR_INSTALL_UPDATES;
	}

	if (strcmp(arg1, "aur-fetchdowngrades") == 0) {
		return AUR_FETCH_DOWNGRADES;
	}
	if (strcmp(arg1, "aur-trustdowngrades") == 0) {
		return AUR_TRUST_DOWNGRADES;
	}
	if (strcmp(arg1, "aur-patchdowngrades") == 0) {
		return AUR_PATCH_DOWNGRADES;
	}
	if (strcmp(arg1, "aur-builddowngrades") == 0) {
		return AUR_BUILD_DOWNGRADES;
	}
	if (strcmp(arg1, "aur-downgrade") == 0) {
		return AUR_INSTALL_DOWNGRADES;
	}

	if (strcmp(arg1, "aur-fetchgit") == 0) {
		return AUR_FETCH_GIT;
	}
	if (strcmp(arg1, "aur-trustgit") == 0) {
		return AUR_TRUST_GIT;
	}
	if (strcmp(arg1, "aur-patchgit") == 0) {
		return AUR_PATCH_GIT;
	}
	if (strcmp(arg1, "aur-buildgit") == 0) {
		return AUR_BUILD_GIT;
	}
	if (strcmp(arg1, "aur-upgradegit") == 0) {
		return AUR_INSTALL_GIT;
	}

	if (strcmp(arg1, "pacman-checkupdates") == 0) {
		return PACMAN_CHECKUPDATES;
	}
	if (strcmp(arg1, "pacman-upgrade") == 0) {
		return PACMAN_UPGRADE;
	}
	return UNKNOWN_COMMAND;
}

static int is_pacman_arg(const char* arg) {
	if (strcmp(arg, "--asdeps") == 0) {
		return 1;
	}

	if (strcmp(arg, "--asexplicit") == 0) {
		return 1;
	}

	if (strcmp(arg, "--needed") == 0) {
		return 1;
	}

	if (strcmp(arg, "--noconfirm") == 0) {
		return 1;
	}

	if (strcmp(arg, "--confirm") == 0) {
		return 1;
	}

	if (strcmp(arg, "--noprogressbar") == 0) {
		return 1;
	}

	return 0;
}

int parse_aur_update_args(aur_upgrade_options_t* __restrict__ opts_out, char** __restrict__ exclude_pkgs, int excpkg_limit, char** __restrict__ pacman_opts, int pacman_opts_limit, int argc, char** argv, aurman_arg_command_t command) {
	opts_out -> exclude_pkgs    = exclude_pkgs;
	opts_out -> n_exclude_pkgs  = 0;
	opts_out -> pacman_opts     = pacman_opts;
	opts_out -> n_pacman_opts   = 0;
	opts_out -> reset_pkgbuilds = 0;
	for (int i = 2; i < argc; i++) {
		// https://stackoverflow.com/questions/11076941/warning-case-not-evaluated-in-enumerated-type
		switch ((int) command) {
			case AUR_INSTALL_UPDATES:
			case AUR_INSTALL_DOWNGRADES:
			case AUR_INSTALL_GIT:
				if (is_pacman_arg(argv[i])) {
					if (pacman_opts != NULL && opts_out -> n_pacman_opts < pacman_opts_limit) {
						pacman_opts[opts_out -> n_pacman_opts] = argv[i];
					}
					opts_out -> n_pacman_opts++;
					continue;
				}
		}

		// if (argv[i][0] != '-') {
		// 	continue;
		// }

		if (strcmp(argv[i], "--") == 0) {
			break;
		}

		if (strcmp(argv[i], "--exclude") == 0) {
			if (i + 1 >= argc) {
				(void) error_printf("[%s] Missing operand for --exclude.\n", argv[1]);
				return 1;
			}
			if (exclude_pkgs != NULL && opts_out -> n_exclude_pkgs < excpkg_limit) {
				exclude_pkgs[opts_out -> n_exclude_pkgs] = argv[i + 1];
			}
			(void) i++;
			(void) opts_out -> n_exclude_pkgs++;
			continue;
		}

		// https://stackoverflow.com/questions/11076941/warning-case-not-evaluated-in-enumerated-type
		switch ((int) command) {
			case AUR_FETCH_UPDATES:
			case AUR_FETCH_DOWNGRADES:
			case AUR_FETCH_GIT:
				if (strcmp(argv[i], "--reset") == 0) {
					opts_out -> reset_pkgbuilds = 1;
					continue;
				}
				(void) error_printf("[%s] Unrecognized option '%s'. Currently only supports --exclude and --reset.\n", argv[1], argv[i]);
				return 1;
		}

		(void) error_printf("[%s] Unrecognized option '%s'. Currently only supports --exclude.\n", argv[1], argv[i]);
		return 1;
	}

	return 0;
}


int parse_aur_install_args(aur_install_options_t* __restrict__ opts_out, char** __restrict__ pkgs, int pkg_limit, char** __restrict__ pacman_opts, int pacman_opts_limit, int argc, char** argv, aurman_arg_command_t command) {
	opts_out -> pkgs            = pkgs;
	opts_out -> n_pkgs          = 0;
	opts_out -> pacman_opts     = pacman_opts;
	opts_out -> n_pacman_opts   = 0;
	opts_out -> reset_pkgbuilds = 0;
	for (int i = 2; i < argc; i++) {
		if (command == AUR_INSTALL && is_pacman_arg(argv[i])) {
			if (pacman_opts != NULL && opts_out -> n_pacman_opts < pacman_opts_limit) {
				pacman_opts[opts_out -> n_pacman_opts] = argv[i];
			}
			opts_out -> n_pacman_opts++;
			continue;
		}

		if (argv[i][0] != '-') {
			if (pkgs != NULL && opts_out -> n_pkgs < pkg_limit) {
				pkgs[opts_out -> n_pkgs] = argv[i];
			}
			opts_out -> n_pkgs++;
			continue;
		}

		if (strcmp(argv[i], "--") == 0) {
			break;
		}

		// https://stackoverflow.com/questions/11076941/warning-case-not-evaluated-in-enumerated-type
		if (command == AUR_FETCH) {
			if (strcmp(argv[i], "--reset") == 0) {
				opts_out -> reset_pkgbuilds = 1;
				continue;
			}
			(void) error_printf("[%s] Unrecognized option '%s'. Currently only supports --reset.\n", argv[1], argv[i]);
			return 1;
		}

		(void) error_printf("[%s] Unrecognized option '%s'. Currently supports no options.\n", argv[1], argv[i]);
		return 1;
	}

	return 0;
}
