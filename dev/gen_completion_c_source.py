#!/usr/bin/env python3

from sys import argv, stderr, stdin

def print_usage() -> None:
	print(f'usage: {argv[0]} variable_name include_header_name', file = stderr)
	print( '       variable_name       The name of the variable of the generated C source file.', file = stderr)
	print( '       include_header_name The name of the .h file to include in the generated C source file.', file = stderr)
	print( 'NOTE: This script is not meant to be used as a standalone tool. It is part of the build process of a project.', file = stderr)

def main() -> None:
	if len(argv) <= 2:
		print_usage()
		exit(1)
		return

	var_name   = argv[1]
	inc_h_name = argv[2]

	bytes_list = []
	for line in stdin.buffer:
		bytes_list.extend('{}{:#04x}'.format('-' if i > 127 else ' ', 256 - i if i > 127 else i) for i in line)

	print(f'#include <{inc_h_name}.h>\n')
	print(f'char {var_name}[] =', '{')

	bytes_list_len  = len(bytes_list)
	n_bytes_per_row = 16

	group, group_start, group_end = bytes_list[0:n_bytes_per_row], 0, n_bytes_per_row
	while group:
		print('\t', ','.join(group), '' if bytes_list_len < group_end + 1 else ',', sep = '')

		group_start = group_end
		group_end  += n_bytes_per_row
		group       = bytes_list[group_start:group_end]

	print('};\n')
	print(f'int  {var_name}_size = sizeof({var_name}) / sizeof(char);')

if __name__ == '__main__':
	main()
