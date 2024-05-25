import json
import logging
import requests

from typing import Optional

def get_all_aur_packages() -> set:
	response = requests.get("https://aur.archlinux.org/packages.gz")
	if not response.ok:
		logging.error("Unable to get updated AUR packages")
		logging.error(f"{response.status_code} - {response.reason}")
		response.raise_for_status()
	return set(response.text.split('\n'))

def get_packages_info(query_packages:list[str]) -> list[dict]:
	response = requests.post('https://aur.archlinux.org/rpc/v5/info', data = {'arg[]': query_packages})
	if not response.ok:
		logging.error("Unable to get updated AUR package information.")
		logging.error(f"{response.status_code} - {response.reason}")
		response.raise_for_status()
	return json.loads(response.text)['results']

def extract_versions_list(aur_json_results:list[dict]) -> dict[str, str]:
	return {result['PackageBase']: result['Version'] for result in aur_json_results}
