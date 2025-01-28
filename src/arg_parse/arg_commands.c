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
	return UNKNOWN_COMMAND;
}

int parse_aur_update_args(aur_upgrade_options_t* __restrict__ opts_out, char** __restrict__ exclude_pkgs, int excpkg_limit, int argc, char** argv, aurman_arg_command_t command) {
	opts_out -> exclude_pkgs    = exclude_pkgs;
	opts_out -> n_exclude_pkgs  = 0;
	opts_out -> reset_pkgbuilds = 0;
	for (int i = 2; i < argc; i++) {
		if (strcmp(argv[i], "--exclude") == 0) {
			if (i + 1 >= argc) {
				(void) error_printf("[%s] Missing operand for --exclude.\n", argv[1]);
				return 1;
			}
			if (exclude_pkgs != NULL) {
				exclude_pkgs[opts_out -> n_exclude_pkgs] = argv[i + 1];
			}
			(void) i++;
			(void) opts_out -> n_exclude_pkgs++;
			continue;
		}

		// https://stackoverflow.com/questions/11076941/warning-case-not-evaluated-in-enumerated-type
		switch ((int) command) {
			case AUR_FETCH:
			case AUR_FETCH_UPDATES:
			case AUR_FETCH_DOWNGRADES:
			case AUR_FETCH_GIT:
				if (strcmp(argv[i], "--reset") == 0) {
					opts_out -> reset_pkgbuilds = 0;
					continue;
				}
		}

		(void) error_printf("[%s] Unrecognized option. Currently only supports --exclude.\n", argv[1]);
		return 1;
	}

	return 0;
}
