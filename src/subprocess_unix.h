#ifndef SUBPROCESS_UNIX_H_INCLUDED
#define SUBPROCESS_UNIX_H_INCLUDED

/**
 * Runs a subprocess by using UNIX syscalls.
 * 
 * @param pwd The directory to change to before running subprocess
 * @param exec_path The absolute path to the executable
 * @param args An array of arguments to be supplied, including argv0
 *             The last item should be NULL
 * @param stdout_fd An output which the stdout pipe fd will be placed.
 *                  Read from this to get the stdout string.
 * @param quiet Set this non-zero to silence execution information
 * @param wait_exit Set this to non-zero for the function to wait for subprocess to complete.
 *                  A zero value means the function will return with the subprocess still running.
 * 
 * @return An integer: return code of subprocess if wait_exit is true, pid of child if wait_exit
 *         is false. On error it will give -1. To clean Zombie processes, call UNIX wait for the
 *         child pid.
 */
int run_subprocess_v(const char* pwd, const char* exec_path, const char* args[], int* __restrict__ stdout_fd, char quiet, char wait_exit);

/**
 * Runs a subprocess by using UNIX syscalls.
 * 
 * @param pwd The directory to change to before running subprocess
 * @param quiet Set this non-zero to silence execution information
 * @param wait_exit Set this to non-zero for the function to wait for subprocess to complete.
 *                  A zero value means the function will return with the subprocess still running.
 * @param stdout_fd An output which the stdout pipe fd will be placed.
 *                  Read from this to get the stdout string.
 * @param exec_path The absolute path to the executable
 * @param arg An array of arguments to be supplied, including argv0. The last argument should be NULL.
 * 
 * @return An integer: return code of subprocess if wait_exit is true, pid of child if wait_exit
 *         is false. On error it will give -1. To clean Zombie processes, call UNIX wait for the
 *         child pid.
 */
int run_subprocess(const char* pwd, char quiet, char wait_exit, int* __restrict__ stdout_fd, const char* exec_path, const char* arg, ...);

#endif // SUBPROCESS_UNIX_H_INCLUDED