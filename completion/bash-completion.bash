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
	COMPREPLY=($(compgen -f -- ${COMP_WORDS[$COMP_CWORD]}))
}

complete -F _aurman ./aur_man
