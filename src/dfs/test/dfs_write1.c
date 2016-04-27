
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>


int main(int argc, char *argv[])
{
	int fd;

	fd = open("dfs_write.data", O_CREAT|O_WRONLY, 0644);
	
	write(fd, "01234", 5);
	write(fd, "56789", 5);
	close(fd);

	return 0;
}
