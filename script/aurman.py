#!/usr/bin/env python3

import logging

def config_logging() -> None:
	logging.basicConfig(level = logging.DEBUG, format = '[%(levelname)s] %(message)s')
	logging.getLogger("httpx").setLevel(logging.WARNING)
	logging.getLogger("httpcore").setLevel(logging.WARNING)
	logging.getLogger("requests").setLevel(logging.WARNING)
	logging.getLogger("urllib3").setLevel(logging.WARNING)
	logging.getLogger("chardet").setLevel(logging.WARNING)
	logging.getLogger("asyncio").setLevel(logging.WARNING)

def main() -> None:
	pass
