
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
	int fd;
	char *fid;
	FILE *file;
	size_t fid_len;
	ssize_t n, nread;
	uint16_t qtype, type = 0x0002;
	uint32_t body_n;
	struct sockaddr_in sin;
	char send_buf[1024], recv_buf[10240]; 

	file = fopen("upload_list", "r");
	if (file == NULL) {
		fprintf(stderr, "fopen() failed %s\n", strerror(errno));
		return -1;
	}

	if (argc != 4) {
		fprintf(stderr, "dfs_client_download <ip> <port> <fid>\n");
		fprintf(stderr, "dfs_client_download 10.101.69.16 1304 <fid>\n");
		return -1;
	}

	fid = argv[3];
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

	/*while(fgets(fid, 64, file) != NULL) {
		fid_len = strlen(fid);
		fprintf(stderr, "read file len %lu\n", fid_len);
		fid[fid_len] = '\0';
		--fid_len;*/
		fid_len = strlen(fid);
		body_n = htonl(fid_len);
		qtype = htons(type);
		memcpy(send_buf, &body_n, 4);
		memcpy(send_buf + 6, &qtype, 2);	
		memcpy(send_buf + 8, fid, fid_len);

		n = send(fd, send_buf,  8 + fid_len, 0);
		if (n == -1) {
			fprintf(stderr, "send() failed: %s\n", strerror(errno));
			return 1;
		}

		fprintf(stderr, "send %ld\n", n);

		nread = 0;
		for (; ;) {
			n = recv(fd, recv_buf + nread, 10240, 0);
			if (n == -1) {
				fprintf(stderr, "recv() failed: %s\n", strerror(errno));
				return 1;
			}

			if (n == 0) {
				fprintf(stderr, "server close\n");
				break;
			}

			fprintf(stderr, "recv %ld\n", n);
			nread += n;
			if (nread >= 8) {
				body_n = ntohl(*((uint32_t *) recv_buf));
				break;
			}
		}

		//nread = 0;
		nread -= 8;
		for (; ;) {
			if (nread >= body_n) {
				break;
			}

			n = recv(fd, recv_buf + nread, 10240, 0);
			if (n == -1) {
				fprintf(stderr, "recv() failed: %s\n", strerror(errno));
				return 1;
			}

			if (n == 0) {
				fprintf(stderr, "server close\n");
				break;
			}

			fprintf(stderr, "recv %ld\n", n);
			nread += n;
		}
		
		fprintf(stderr, "recv %u, upload fid %s, download body_n %u\n", body_n + 8, fid, body_n);
//	}

	if (ferror(file) != 0) {
		fprintf(stderr, "file error\n");
	}

	fprintf(stderr, "process over\n");
	close(fd);
	fclose(file);

	return 0;
}
