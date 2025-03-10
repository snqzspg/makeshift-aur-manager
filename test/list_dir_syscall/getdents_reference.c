/**
 * https://www.perlmonks.org/?node_id=1148448
 */

#define _GNU_SOURCE
#include <dirent.h>     /* Defines DT_* constants */
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/syscall.h>

#define handle_error(msg) \
	   do { perror(msg); exit(EXIT_FAILURE); } while (0)

struct linux_dirent {
   long           d_ino;
   off_t          d_off;
   unsigned short d_reclen;
   char           d_name[];
};

// #define BUF_SIZE 1024*1024*5
#define BUF_SIZE 256

int main(int argc, char *argv[]) {
	int fd, nread;
	char buf[BUF_SIZE];
	struct linux_dirent *d;
	int bpos;
	char d_type;

	fd = open(argc > 1 ? argv[1] : ".", O_RDONLY | O_DIRECTORY);
	if (fd == -1)
		handle_error("open");

	for ( ; ; ) {
		nread = syscall(SYS_getdents, fd, buf, BUF_SIZE);
		if (nread == -1)
			handle_error("getdents");
		
		(void) fprintf(stderr, "%d\n", nread);

		if (nread == 0)
			break;

		for (bpos = 0; bpos < nread;) {
			d = (struct linux_dirent *) (buf + bpos);
			if (d->d_ino != 0) printf("%s\n", (char *) d->d_name);
			bpos += d->d_reclen;
		}
	}

	exit(EXIT_SUCCESS);
}
