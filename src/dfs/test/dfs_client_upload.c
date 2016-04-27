
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>


int 
main(int argc, char *argv[])
{
	int i, fd, loop, f;
	ssize_t n;
	uint16_t qtype, type = 0x0001;
	uint16_t rcode;
	uint32_t body_n;
	struct sockaddr_in sin;
	char send_buf[102406], recv_buf[1024]; 
	
	f = open("upload_list", O_CREAT|O_TRUNC|O_WRONLY, 0644);
	if (f == -1) {
		fprintf(stderr, "open() failed %s\n", strerror(errno));
		return -1;
	}

	loop = 1024;
	if (argc != 3 && argc != 4) {
		fprintf(stderr, "dfs_client_upload <ip> <port> [0|1, upload(default)|upload_sc]\n");
		fprintf(stderr, "dfs_client_upload 10.101.69.16 1304\n");
		return 1;
	}
	/*if (argc == 2) {
		loop = atoi(argv[1]);
	}*/

	if (argc == 4) {
		if (atoi(argv[3]) == 1) {
			type = 0x0008;
		}
	}

	memset(&sin, 0x00, sizeof(struct sockaddr_in));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(atoi(argv[2]));
	sin.sin_addr.s_addr = inet_addr(argv[1]);

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1) {
		fprintf(stderr, "socket() failed: %s\n", strerror(errno));
		return 1;
	}

	if (connect(fd, (struct sockaddr *) &sin, sizeof(struct sockaddr_in)) == -1) {
		fprintf(stderr, "connect(%s:%hu) failed: %s\n", argv[1], atoi(argv[2]), strerror(errno));
		return 1;
	}

	qtype = htons(type);
	rcode = 0;
	//for (i = 0; i < loop; ++i) {
		body_n = htonl((i + 1));
		memcpy(send_buf, &body_n, 4);
		memcpy(send_buf + 6, &qtype, 2);	
		//memcpy(send_buf + 6, &rcode, 2);
		memcpy(send_buf + 8, "0123456789", 10);

		n = send(fd, send_buf,  8 + (i + 1), 0);
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
		body_n = ntohl(*((uint32_t *) recv_buf));
		fprintf(stderr, "recv %ld, body_n %u, fid %s\n", n, body_n, recv_buf + 8);
		recv_buf[n] = '\n';
		write(f, recv_buf + 8, body_n + 1);
	//}

	close(fd);
	close(f);

	return 0;
}
