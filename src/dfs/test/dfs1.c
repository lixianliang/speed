

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>


int 
main(int argc, char *argv[])
{
	int fd;

	fd = open("dfs1.txt", O_CREAT|O_WRONLY|O_APPEND, 0644);
	
	write(fd, "0123456789", 10);

	if (rename("dfs1.txt", "dfs1_bak.txt") == -1) {
		fprintf(stderr, "rename() failed: %s\n", strerror(errno));
		return 1;
	}

	if (fsync(fd) == -1) {
		fprintf(stderr, "fsync() failed: %s\n", strerror(errno));
		return 1;
	}

	if (unlink("dfs1_bak.txt") == -1) {
		fprintf(stderr, "unlink() failed: %s\n", strerror(errno));
		return 1;
	}

	if (close(fd) == -1) {
		fprintf(stderr, "close() failed: %s\n", strerror(errno));
		return 1;
	}
	
	return 0;
}
