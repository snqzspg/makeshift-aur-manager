_aurman_command_complete() {
	local commands=('help pacman-checkupdates pacman-upgrade aur-fetchupdates aur-fetchdowngrades aur-fetchgit aur-buildupdates aur-builddowngrades aur-buildgit aur-upgrade aur-upgradegit aur-fetch aur-build aur-install')
	local word=$1
	COMPREPLY=($(compgen -W "$commands" -- "$word"))
}

_aurman() {
	local nth_arg=${#COMP_WORDS[@]}

	if [ "$COMP_CWORD" -eq 1 ];
	then
		_aurman_command_complete "${COMP_WORDS[$COMP_CWORD]}"
		return
	fi

	local aur_pkgnames="__pkg_cache__/aur_package_names.txt.gz"

	if [ -f "$aur_pkgnames" ];
	then
		case "${COMP_WORDS[$((COMP_CWORD - 1))]}" in 
			"aur-build")
				# COMPREPLY=($(compgen -W "$(zcat "$aur_pkgnames")" -- ${COMP_WORDS[$COMP_CWORD]}))
				__aurman_search_pkgs ${COMP_WORDS[$COMP_CWORD]}
			;;

			"aur-fetch")
				# COMPREPLY=($(compgen -W "$(zcat "$aur_pkgnames")" -- ${COMP_WORDS[$COMP_CWORD]}))
				__aurman_search_pkgs ${COMP_WORDS[$COMP_CWORD]}
			;;

			"aur-install")
				# COMPREPLY=($(compgen -W "$(zcat "$aur_pkgnames")" -- ${COMP_WORDS[$COMP_CWORD]}))
				__aurman_search_pkgs ${COMP_WORDS[$COMP_CWORD]}
			;;
		esac
		return
	fi

	COMPREPLY=($(compgen -f -- ${COMP_WORDS[$COMP_CWORD]}))
}

__aurman_search_pkgs() {
	local completion_folder="__pkg_cache__/__completions__"
	if [ -z "$1" ] || [ ! -d "$completion_folder" ];
	then
		local aur_pkgnames="__pkg_cache__/aur_package_names.txt.gz"
		COMPREPLY=($(compgen -W "$(zcat "$aur_pkgnames")" -- "$1"))
		return
	fi
	local firstc=${1:0:1}

	COMPREPLY=($(compgen -W "$(zcat "$completion_folder/$firstc.txt.gz")" -- "$1"))
}

__aurman_index_aur_pkg_completions() {
	local completion_folder="__pkg_cache__/__completions__"
	gzip -cd "__pkg_cache__/aur_package_names.txt.gz" | while read i;
	do
		local firstc=${i:0:1}
		if [ ! -d "$completion_folder" ];
		then
			mkdir -p "$completion_folder"
		fi
		echo "$i" >> "$completion_folder/$firstc.txt" 
	done

	find "$completion_folder" -name '*.txt' -exec gzip {} \;
}

complete -F _aurman ./aur_man
