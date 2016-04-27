
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>


int main(int argc, char *argv[])
{
	ssize_t n;
	char buf1[64], buf2[64];
	int fd1, fd2;

	fd1 = open("./read_test", O_RDONLY);
	fd2 = open("./read_test", O_RDONLY);

	n = read(fd1, buf1, 5);
	buf1[n] = '\0';
	fprintf(stderr, "%s\n", buf1);

	n = read(fd2, buf2, 5);
	buf2[n] = '\0';
	fprintf(stderr, "%s\n", buf2);
	
	return 0;
}
