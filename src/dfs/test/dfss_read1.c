#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>


int main(int argc, char *argv[])
{
	int fd;
	char buf[1024];
	ssize_t n;

	fd = open("0153881891000013000000130111", O_RDONLY);
	for ( ;; ) {
		n = read(fd, buf, 1024);
		if (n == 0) {
			break;
		}

		fprintf(stderr, "read %ld\n", n);
	}
	
	return 0;
}
