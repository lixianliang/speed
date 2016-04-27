
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_config.h>
#include <lxl_core.h>
/*#include <lxl_log.h>
#include <lxl_socket.h>
#include <lxl_connection.h>*/
#include <lxl_event.h>
//#include <lxl_event_connect.h>


int
lxl_event_connect_peer(lxl_peer_connection_t *pc)
{
	int 			   fd, rc;
	lxl_event_t 	  *rev, *wev;
	lxl_connection_t  *c;

	rc = pc->get(pc, pc->data);
	if (rc != 0) {
		return rc;
	}

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1) {
		lxl_log_error(LXL_LOG_ALERT, errno, "socket() faild");
		return LXL_ERROR;
	}

	lxl_log_debug(LXL_LOG_DEBUG_EVENT, 0, "socket %d", fd);

	c = lxl_get_connection(fd);
	if (c == NULL) {
		if (close(fd) == -1) {
			lxl_log_error(LXL_LOG_ALERT, errno, "close() failed");
		}

		return LXL_ERROR;
	}

	if (pc->rcvbuf) {
		if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &pc->rcvbuf, sizeof(int)) == -1) {
			lxl_log_error(LXL_LOG_ALERT, errno, "setsockopt(SO_RCVBUF) failed");
			goto failed;
		}
	}

	if (lxl_nonblocking(fd) == -1) {
		lxl_log_error(LXL_LOG_ALERT, errno, lxl_nonblocking_n "failed");
		goto failed;
	}

	//if (pc->local) {
		/* bind() */
	//}

	c->recv = lxl_recv;
	c->send = lxl_send;
	//c->closefd = 1;
	c->sendfile = 1;

	rev = c->read;
	wev = c->write;
	pc->connection = c;
	
	if (lxl_add_conn(c) == -1) {
		goto failed;
	}

	lxl_log_debug(LXL_LOG_DEBUG_EVENT, 0, "connect to %s, fd:%d", pc->name, fd);
	rc = connect(fd, pc->sockaddr, pc->socklen);
	if (rc == -1) {
		if (errno != EINPROGRESS) {
			lxl_log_error(LXL_LOG_ERROR, errno, "connect() to failed");
			lxl_close_connection(c);
			pc->connection = NULL;

			return LXL_DECLINED;
		}
	}

	/*if (lxl_add_event(rev, LXL_READ_EVENT, LXL_CLEAR_EVENT) != 0) {
		goto failed;
	}

	if (rc == -1) {
		// EINPROGRESS 
		if (lxl_add_event(wev, LXL_WRITE_EVENT, LXL_CLEAR_EVENT) != 0) {
			goto failed;
		}

		return  LXL_EAGAIN;
	}*/
		
	if (rc == -1) {
		return LXL_EAGAIN;
	}

	lxl_log_debug(LXL_LOG_DEBUG_EVENT, 0, "connected");
	wev->ready = 1;
	
	return LXL_OK;

failed:

	lxl_close_connection(c);
	pc->connection = NULL;
	
	return LXL_ERROR;
}

int
lxl_event_udp_connect_peer(lxl_peer_connection_t *pc)
{
	int fd;
	lxl_connection_t *c;

	//lxl_log_debug(LXL_LOG_DEBUG_EVENT, 0, "event udp connect peer");
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == -1) {
		lxl_log_error(LXL_LOG_ERROR, errno, "socket() failed");
		return -1;
	}

	lxl_log_debug(LXL_LOG_DEBUG_EVENT, 0, "socket %d", fd);
	c = lxl_get_connection(fd);
	if (c == NULL) {
		lxl_log_error(LXL_LOG_WARN, 0, "get connection() failed");
		close(fd);
		return -1;
	}
	
	// nonblock
	//connection->sockaddr = *sa;
	//connection->socklen = len;
	c->recv = lxl_recvfrom;
	c->send = lxl_sendto;
	c->udp = 1;
	//c->closefd = 1;
	pc->connection = c;

	return 0;
}

int
lxl_event_get_peer(lxl_peer_connection_t *pc, void *data)
{
	return 0;
}
