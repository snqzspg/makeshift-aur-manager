#ifndef UNISTD_HELPER_H_INCLUDED
#define UNISTD_HELPER_H_INCLUDED

#define run_syscall_print_err_w_ret(syscall_fx, ret_if_err, file_name, line_no) if ((syscall_fx) < 0) {\
	(void) fprintf(stderr, "[ERROR][%s:%d]: %s\n", (file_name), (line_no), strerror(errno)); \
	return (ret_if_err); \
}

#endif // UNISTD_HELPER_H_INCLUDED