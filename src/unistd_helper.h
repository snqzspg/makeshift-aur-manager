#ifndef UNISTD_HELPER_H_INCLUDED
#define UNISTD_HELPER_H_INCLUDED

#define run_syscall_print_err(syscall_fx, file_name, line_no) if ((syscall_fx) < 0) {\
	(void) fprintf(stderr, "[ERROR][%s:%d]: %s\n", (file_name), (line_no), strerror(errno)); \
    return; \
}

#define run_syscall_print_err_w_ret(syscall_fx, ret_if_err, file_name, line_no) if ((syscall_fx) < 0) {\
	(void) fprintf(stderr, "[ERROR][%s:%d]: %s\n", (file_name), (line_no), strerror(errno)); \
	return (ret_if_err); \
}

#define run_syscall_print_err_w_ret_errno(syscall_fx, file_name, line_no) if ((syscall_fx) < 0) {\
	(void) fprintf(stderr, "[ERROR][%s:%d]: %s\n", (file_name), (line_no), strerror(errno)); \
	return errno; \
}

#define run_syscall_print_err_w_act(syscall_fx, action_statements, log_pf, file_name, line_no) if ((syscall_fx) < 0) {\
	(void) (log_pf)("[%s:%d]: %s\n", (file_name), (line_no), strerror(errno)); \
    action_statements \
}

#define run_syscall_print_w_act(syscall_fx, action_statements, log_type, file_name, line_no) if ((syscall_fx) < 0) {\
	(void) fprintf(stderr, "[%s][%s:%d]: %s\n", (log_type), (file_name), (line_no), strerror(errno)); \
    action_statements \
}

#endif // UNISTD_HELPER_H_INCLUDED