#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <subprocess_unix.h>

char* ver_pairs[][2] = {
    {"7.28.0-5", "7.30.0-1"},
    {"r20.8da7f79-1", "r16.f5d5323-2"},
    {"24.004.60.r88.ga0e8b6cf-1", "24.004.60.r88.ga0e8b6cf-1"},
    {"2.3.1-1", "2.3.1-1"},
    {"1.2-1", "1:0.1-1"}
};

int main(void) {
    int use_parallel = 1;

    int fds[5];
    int p1 = run_subprocess(NULL, 0, !use_parallel, fds, "/usr/bin/sh", "sh", "slow_vercmp.sh", ver_pairs[0][0], ver_pairs[0][1], NULL);
    int p2 = run_subprocess(NULL, 0, !use_parallel, fds + 1, "/usr/bin/sh", "sh", "slow_vercmp.sh", ver_pairs[1][0], ver_pairs[1][1], NULL);
    int p3 = run_subprocess(NULL, 0, !use_parallel, fds + 2, "/usr/bin/sh", "sh", "slow_vercmp.sh", ver_pairs[2][0], ver_pairs[2][1], NULL);
    int p4 = run_subprocess(NULL, 0, !use_parallel, fds + 3, "/usr/bin/sh", "sh", "slow_vercmp.sh", ver_pairs[3][0], ver_pairs[3][1], NULL);
    int p5 = run_subprocess(NULL, 0, !use_parallel, fds + 4, "/usr/bin/sh", "sh", "slow_vercmp.sh", ver_pairs[4][0], ver_pairs[4][1], NULL);

    (void) waitpid(p1, NULL, 0);
    (void) waitpid(p2, NULL, 0);
    (void) waitpid(p3, NULL, 0);
    (void) waitpid(p4, NULL, 0);
    (void) waitpid(p5, NULL, 0);

    char b[5];
    b[4] = '\n';

    for (int i = 0; i < 5; i++) {
        int r = read(fds[i], &b, 4);
        (void) write(STDOUT_FILENO, b, r);
    }

    return 0;
}
