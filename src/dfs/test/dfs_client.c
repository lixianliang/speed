
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>


int 
main(int argc, char *argv[])
{
	int i, fd, loop;
	struct sockaddr_in sin;
	char send_buf[102406], recv_buf[1024]; 

	loop = 1024;
	if (argc == 2) {
		loop = atoi(argv[1]);
	}

	memset(&sin, 0x00, sizeof(struct sockaddr_in));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(1304);
	sin.sin_addr.s_addr = inet_addr("127.0.0.1");

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1) {
		fprintf(stderr, "socket() failed: %s\n", strerror(errno));
		return 1;
	}

	if (connect(fd, (struct sockaddr *) &sin, sizeof(struct sockaddr_in)) == -1) {
		fprintf(stderr, "connect() failed: %s\n", strerror(errno));
		return 1;
	}

	for (i = 0; i < loop; ++i) {
		uint32_t body_n = htonl((i + 1));
		uint16_t qtype = htons(1);
		memcpy(send_buf, &body_n, 4);
		memcpy(send_buf + 4, &qtype, 2);	
		memcpy(send_buf + 6, "0123456789", 10);

		ssize_t n = send(fd, send_buf,  6 + (i + 1), 0);
		if (n == -1) {
			fprintf(stderr, "send() failed: %s\n", strerror(errno));
			return 1;
		}

		fprintf(stderr, "send %ld\n", n);

		n = recv(fd, recv_buf, 1024, 0);
		if (n == -1) {
			fprintf(stderr, "recv() failed: %s\n", strerror(errno));
			return 1;
		}

		recv_buf[n] = '\0';
		fprintf(stderr, "recv %ld, fid %s\n", n, recv_buf + 6);
	}

	close(fd);

	return 0;
}
