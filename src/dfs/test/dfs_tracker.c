
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>


int
main(int argc, char *argv[])
{
	int 	   			fd, s;
	char 	  		   *ip;
	char 				buf[1024];
	ssize_t 			n;
	uint32_t			body_n;
	uint16_t			qtype, rcode;
	in_port_t  			port;
	in_addr_t			addr;
	struct sockaddr_in  sa;

	if (argc != 3) {
		fprintf(stderr, "Usage: dfss_tracker ip port\n");
		return 1;
	}

	ip = argv[1];
	port = (in_port_t) atoi(argv[2]);

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1) {
		fprintf(stderr, "socket() failed: %s\n", strerror(errno));
		return 1;
	}

	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = inet_addr(ip);
	if (bind(fd, (struct sockaddr *) &sa, sizeof(struct sockaddr_in)) != 0) {
		fprintf(stderr, "bind() failed: %s\n", strerror(errno));
		return 1;
	}

	if (listen(fd, 256) != 0) {
		fprintf(stderr, "listen() failed: %s\n", strerror(errno));
		return 1;
	}

	body_n = htonl(6);
	qtype = htons(0);
	rcode = 0;
	port = htons(1404);
	addr = inet_addr("127.0.0.1");
	for (; ;) {
		s = accept(fd, NULL, NULL);
		close(s);
		continue;
		if (s == -1) {
			fprintf(stderr, "accept()  failed: %s\n", strerror(errno));
			continue;
		}

		fprintf(stderr, "accept %d\n", s);
		n = recv(s, buf, sizeof(1024), 0);
		if (n == -1) {
			fprintf(stderr, "recv() failed: %s\n", strerror(errno));
			continue;
		}

		fprintf(stderr, "recv() %ld\n", n);

		memcpy(buf, &body_n, 4);
		memcpy(buf + 4, &qtype, 2);
		memcpy(buf + 6, &rcode, 2);
		memcpy(buf + 8, &addr, 4);
		memcpy(buf + 12, &port, 2);

		n = send(s, buf, 14, 0);
		if (n == -1) {
			fprintf(stderr, "send() failed: %s\n", strerror(errno));
		}
	
		fprintf(stderr, "send() %ld", n);
	}

	close(fd);
	
	return 0;
}
