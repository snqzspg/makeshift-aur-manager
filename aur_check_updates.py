#!/usr/bin/env python3
import argparse
from git import Repo
import json
import requests
from os import sep, path, mkdir
from subprocess import run as sprun

PACKAGE_CACHE_DIR = "__pkg_cache__"

def get_aur_packages_set():
	response = requests.get("https://aur.archlinux.org/packages.gz")
	if not response.ok:
		print("Fetching AUR packages failed")
		print(f"{response.status_code} - {response.reason}")
		response.raise_for_status()
	return set(response.text.split('\n'))

def create_cache_dir():
	if not path.isdir(PACKAGE_CACHE_DIR):
		mkdir(PACKAGE_CACHE_DIR)

def fetch_package(package_name):
	package_loc = PACKAGE_CACHE_DIR + sep + package_name
	if not path.isdir(package_loc):
		Repo.clone_from(f"https://aur.archlinux.org/{package_name}.git", package_loc)

def get_installed_non_pacman():
	p = [tuple(i.split(' ')) for i in sprun(['pacman', '-Qm'], capture_output = True).stdout.decode().split('\n')]
	p.pop()
	d = {}
	for i in p:
		d[i[0]] = i[1]
	return d

def get_packages_info(package_list):
	response = requests.post('https://aur.archlinux.org/rpc/v5/info', data = {'arg[]': package_list})
	if not response.ok:
		print("Fetching AUR packages failed")
		print(f"{response.status_code} - {response.reason}")
		response.raise_for_status()
	o = json.loads(response.text)
	d = {}
	for i in o['results']:
		d[i['PackageBase']] = i['Version']
	return d

def find_ver_diff(all_git = False, no_git = False):
	installed_vers = get_installed_non_pacman()
	installed_package_names = list(installed_vers.keys())
	aur_data = get_packages_info(installed_package_names)
	installed_and_on_aur = {}
	for name, version in aur_data.items():
		if name in installed_vers:
			installed_and_on_aur[name] = (installed_vers[name], version)
	diffs = {}
	if all_git:
		for name, vers in installed_and_on_aur.items():
			if name[-4:] == '-git':
				diffs[name] = vers
	else:
		for name, vers in installed_and_on_aur.items():
			if no_git and name[-4:] == '-git':
				continue
			if vers[0] != vers[1]:
				diffs[name] = vers
	return diffs

def main():
	parser = argparse.ArgumentParser(
		prog="Manual AUR checker",
		description="Check your list of installed packages against the AUR for any version discrepancies."
		)
	parser.add_argument("--no-git", help = "Will not list any packages that ends with '-git'. Ignored if --all-git is present.", action = "store_true")
	parser.add_argument("--all-git", help = "List all the packages ends with '-git' regardless of version matches.", action = "store_true")
	args = parser.parse_args()
	ver_diff = find_ver_diff(args.all_git, args.no_git)
	for name, versions in ver_diff.items():
		print("{}: {} {} {}".format(name, versions[0], '==' if versions[0] == versions[1] else "!=", versions[1]))
	ans = input("Write the names into a file? [Y/n] ")
	if ans.lower() != 'n':
		with open('package_names.txt', 'wb') as f:
			f.write('\n'.join(ver_diff.keys()).encode())

if __name__ == '__main__':
	main()
