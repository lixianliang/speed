
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_dfss.h>


typedef struct {
	lxl_dfss_upstream_conf_t  upstream;
} lxl_dfss_storage_srv_conf_t;


static void lxl_dfss_storage_init_request(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u);
static int	lxl_dfss_storage_create_request(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u);

static void lxl_dfss_storage_handler(lxl_event_t *ev);

static void lxl_dfss_storage_connect(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u);
static void lxl_dfss_storage_send_request(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u);
static void lxl_dfss_storage_send_request_body(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u);
static void lxl_dfss_storage_send_request_body_handler(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u);
static int	lxl_dfss_storage_do_send_request_body(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u);
static void lxl_dfss_storage_process_response_header(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u);

static void lxl_dfss_storage_dummy_handler(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u);
static void lxl_dfss_storage_next(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u, lxl_uint_t ft_type);
static void lxl_dfss_storage_finalize_request(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u, lxl_int_t rc);

static void *lxl_dfss_storage_create_conf(lxl_conf_t *cf);
static char *lxl_dfss_storage_init_conf(lxl_conf_t *cf, void *conf);
static char *lxl_dfss_storage_pass(lxl_conf_t *cf, lxl_command_t *cmd, void *conf);
static char *lxl_dfss_storage_lowat_check(lxl_conf_t *cf, void *post, void *data);

static lxl_conf_post_t  lxl_dfss_storage_lowat_post = { lxl_dfss_storage_lowat_check };

lxl_module_t  lxl_dfss_storage_module;

static lxl_command_t  lxl_dfss_storage_commands[] = {
	
	{ lxl_string("storage_pass"),
  	  LXL_DFSS_SRV_CONF|LXL_CONF_TAKE1,
	  lxl_dfss_storage_pass,
	  LXL_DFSS_SRV_CONF_OFFSET,
	  0,
	  NULL },

	{ lxl_string("storage_connect_timeout"),
	  LXL_DFSS_MAIN_CONF|LXL_DFSS_SRV_CONF|LXL_CONF_TAKE1,
	  lxl_conf_set_msec_slot,
	  LXL_DFSS_SRV_CONF_OFFSET,
	  offsetof(lxl_dfss_storage_srv_conf_t, upstream.connect_timeout),
	  NULL },

	{ lxl_string("storage_send_timeout"),
	  LXL_DFSS_MAIN_CONF|LXL_DFSS_SRV_CONF|LXL_CONF_TAKE1,
	  lxl_conf_set_msec_slot,
	  LXL_DFSS_SRV_CONF_OFFSET,
	  offsetof(lxl_dfss_storage_srv_conf_t, upstream.send_timeout),
	  NULL },

	{ lxl_string("storage_read_timeout"),
	  LXL_DFSS_MAIN_CONF|LXL_DFSS_SRV_CONF|LXL_CONF_TAKE1,
	  lxl_conf_set_msec_slot,
	  LXL_DFSS_SRV_CONF_OFFSET,
	  offsetof(lxl_dfss_storage_srv_conf_t, upstream.read_timeout),
	  NULL }, 

	{ lxl_string("storage_send_lowat"),
	  LXL_DFSS_MAIN_CONF|LXL_DFSS_SRV_CONF|LXL_CONF_TAKE1,
	  lxl_conf_set_size_slot,
	  LXL_DFSS_SRV_CONF_OFFSET,
	  offsetof(lxl_dfss_storage_srv_conf_t, upstream.send_lowat),
	  &lxl_dfss_storage_lowat_post },

	lxl_null_command
};

lxl_dfss_module_t  lxl_dfss_storage_module_ctx = {
	NULL,
	NULL,
	lxl_dfss_storage_create_conf,
	lxl_dfss_storage_init_conf
};

lxl_module_t  lxl_dfss_storage_module = {
	0,
	0,
	(void *) &lxl_dfss_storage_module_ctx,
	lxl_dfss_storage_commands,
	LXL_DFSS_MODULE,
	NULL,
	NULL
};

void
lxl_dfss_storage_init(lxl_dfss_request_t *r)
{
	//lxl_dfss_upstream_t			 *u;
	lxl_dfss_storage_srv_conf_t  *sscf;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss storage init request, %s", r->rid);

	r->storage = lxl_pcalloc(r->pool, sizeof(lxl_dfss_upstream_t));
	if (r->storage == NULL) {
		return;
	}

	sscf = lxl_dfss_get_module_srv_conf(r, lxl_dfss_storage_module);
	r->storage->conf = &sscf->upstream;

	lxl_dfss_storage_init_request(r, r->storage);
}

static void
lxl_dfss_storage_init_request(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u)
{
	lxl_str_t					  *fid;
	lxl_dfs_request_header_t	  *header;
	lxl_dfss_upstream_srv_conf_t  *uscf;

	fid = &r->body->fid;
	u->header_buf = r->tracker->header_buf;
	header = &u->request_header;

	if (r->request_header.qtype == LXL_DFS_UPLOAD || r->request_header.qtype == LXL_DFS_UPLOAD_SC) {
		header->body_n = r->request_header.body_n;
		header->flen = fid->len;
		header->qtype = LXL_DFS_STORAGE_SYNC_PUSH;

	} else {
		header->body_n = fid->len;
		header->flen = 0;
		header->qtype = LXL_DFS_STORAGE_SYNC_PULL;
	}

	lxl_dfss_storage_create_request(r, u);

	uscf = u->conf->upstream;
	
	if (uscf == NULL) {
		lxl_log_error(LXL_LOG_ALERT, 0, "no upstream configuration");
		return;
	}

	/*if (uscf->peer.init(r, uscf, u) != 0) {
	}*/

	lxl_dfss_storage_connect(r, u);
}

static int
lxl_dfss_storage_create_request(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u)
{
	size_t 					   size, old_size;
	lxl_str_t				  *fid;
	lxl_buf_t 				  *b;
	lxl_dfs_request_header_t  *header;

	fid = &r->body->fid;
//	u = r->storage;
//	u->header_buf = r->tracker->header_buf;
	header = &u->request_header;

	b = u->header_buf;
	lxl_reset_buf(b);

	*((uint32_t *) b->last) = htonl(header->body_n);
	b->last += 4;
	*((uint16_t *) b->last) = htons(header->flen);
	b->last += 2;
	*((uint16_t *) b->last) = htons(header->qtype);
	b->last += 2;
	memcpy(b->last, fid->data, fid->len);
	b->last += fid->len;
	/*header->qtype = r->tracker->request_header.qtype;
	if (header->qtype == LXL_DFS_STORAGE_SYNC_PUSH) {
		header->body_n = r->request_header.body_n;
		header->flen = r->body->fid.len;
		*((uint32_t *) b->last) = htonl(header->body_n);
		b->last += 4;
		*((uint16_t *) b->last) = htons(header->flen);
		b->last += 2;
		*((uint16_t *) b->last) = htons(header->qtype);
		b->last += 2;
		memcpy(b->last, r->body->fid.data, header->flen);
		b->last += header->flen;
	} else {
		// LXL_DFS_STORAGE_SYNC_FID_PULL recovery 
		// lxl_queue_t  fid recovery
	}*/

	return 0;
}

static void
lxl_dfss_storage_handler(lxl_event_t *ev)
{
	lxl_connection_t  	 *c;
	lxl_dfss_request_t   *r;
	lxl_dfss_upstream_t  *u;

	c = ev->data;
	r = c->data;
	u = r->storage;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss storage run request %s", r->rid);

	if (ev->write) {
		u->write_event_handler(r, u);
	} else {
		u->read_event_handler(r, u);
	}

	// posted_request;
}

/*static void
lxl_dfss_storage_next(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u, lxl_uint_t ft_type)
{
	int 			   rc;
	lxl_connection_t  *c;
	lxl_dfss_addr_t   *addr;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss next storage, %lu", ft_type);*/

	//lxl_dfss_storage_connect(r, u);
	/*addr = lxl_queue_out(&r->addrs);
	if (addr == NULL) {
		lxl_dfss_storage_finalize_request(r, u, );
		return;
	}

	u->peer.sockaddr = addr->addr.sockaddr;
	u->peer.socklen = addr->addr.socklen;
	u->peer.name = addr->addr.name;
	u->peer.get = lxl_event_get_peer;

	rc = lxl_event_connect_peer(&u->peer);
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss idc %hu storage connect: %d", addr->idc_id, rc);
	if (rc == LXL_ERROR) {
		// lxl_dfss_finialize_request()
		return;
	}*/

	/*if (rc == LXL_BUSY) {
		lxl_log_error(LXL_LOG_ERROR, 0, "dfss no live storages");
		lxl_dfss_storage_next(r, u, LXL_DFSS_UPSTREAM_FT_NOLIVE);
		return;
	}

	if (rc == LXL_DECLINED) {
		lxl_dfss_storage_next(r, u, LXL_DFSS_UPSTREAM_FT_ERROR);
		return;
	}*/

	/*c = u->peer.connection;
	c->data = r;
	c->write->handler = lxl_dfss_storage_handler;
	c->read->handler = lxl_dfss_storage_handler;
	u->write_event_handler = lxl_dfss_storage_send_request;
	u->read_event_handler = lxl_dfss_storage_process_response_header;

	if (rc == LXL_EAGAIN) {
		lxl_add_timer(c->write, 3000);
		return;
	}

	lxl_dfss_storage_send_request(r, u);*/
//}

static void
lxl_dfss_storage_connect(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u)
{
	int 			   	  rc;
	lxl_dfss_addr_t  	 *addr;
	lxl_connection_t  	 *c;

	addr = lxl_queue_out(&r->addrs);
	if (addr == NULL) {
		lxl_dfss_storage_finalize_request(r, u, LXL_DFSS_UPSTREAM_FT_TRY_OVER);
		return;
	}

	u->peer.sockaddr = addr->addr.sockaddr;
	u->peer.socklen = addr->addr.socklen;
	u->peer.name = addr->addr.name;
	u->peer.get = lxl_event_get_peer;

	rc = lxl_event_connect_peer(&u->peer);
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss storage connect: %d", rc);
	if (rc == LXL_ERROR) {
		// lxl_dfss_finialize_request()
		return;
	}

	/*if (rc == LXL_BUSY) {
		lxl_log_error(LXL_LOG_ERROR, 0, "dfss no live storages");
		lxl_dfss_storage_next(r, u, LXL_DFSS_UPSTREAM_FT_NOLIVE);
		return;
	}

	if (rc == LXL_DECLINED) {
		lxl_dfss_storage_next(r, u, LXL_DFSS_UPSTREAM_FT_ERROR);
		return;
	}*/

	c = u->peer.connection;
	c->data = r;
	c->read->handler = lxl_dfss_storage_handler;
	c->write->handler = lxl_dfss_storage_handler;
	//u->read_event_handler = lxl_dfss_storage_process_response_header;
	u->read_event_handler = lxl_dfss_upstream_block_reading;
	u->write_event_handler = lxl_dfss_storage_send_request;

	if (u->request_send) {
		u->request_send = 0;
		lxl_dfss_storage_create_request(r, u);
	}

	if (rc == LXL_EAGAIN) {
		lxl_add_timer(c->write, 3000);
		return;
	}

	lxl_dfss_storage_send_request(r, u);
}

static void
lxl_dfss_storage_send_request(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u
)
{
	int 			   rc;
	ssize_t 		   n;
	lxl_buf_t		  *b;
	lxl_connection_t  *c;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss storage send request header");

	c = u->peer.connection;
	
	if (c->write->timedout) {
		return;
	}

	u->request_send = 1;
	b = u->header_buf;
	n = c->send(c, b->pos, b->last - b->pos);
	
	if (n == LXL_ERROR) {
		return;
	}

	if (n == LXL_EAGAIN) {
		goto again;
	}

	b->pos += n;
	if (b->pos != b->last) {
		goto again;
	}
	
	u->write_event_handler = lxl_dfss_storage_send_request_body;
	
	lxl_dfss_storage_send_request_body(r, u);

	/*if (c->write->timer_set) {
		lxl_del_timer(c->write);
	}


	if (rc == LXL_EAGAIN) {
		if (lxl_handle_write_event(c->write, 0) != 0) {
			return;
		}

		return;
	}

	lxl_add_timer(c->read, 3000);
	if (c->read->ready) {
		lxl_dfss_storage_process_header(r, u);
	}

	if (lxl_handle_write_event(c->write, 0) != 0) {
		// lxl_dfss_storage_finalize_request
		return;
	}

	return;*/

again:

	lxl_add_timer(c->write, 3000);

	if (lxl_handle_write_event(c->write, 0) != 0) {
		return;
	}

	return;
}

static void
lxl_dfss_storage_send_request_body(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u)
{
	int 			   		  rc;
	lxl_buf_t  		  		 *b;
	lxl_dfss_request_body_t  *rb;

	rb = r->body;
	b = &u->buffer;
	rb->rest = u->request_header.body_n;

	if (r->request_body_in_onebuf) {
		*(b) = *(rb->buf);
		b->pos = b->start;
		b->last = b->pos + rb->rest;
		/*b->start = r->body->buf->start;
		b->pos = r->body->buf->start;
		b->last = r->body->buf->last;
		b->end = r->body->buf->end;*/
	} else {
		//*(b) = *(r->body->buf); 
		b->start = r->body->buf->start;
		b->pos = b->start;
		b->last = b->start;
		b->end = r->body->buf->end;
	}

	u->write_event_handler = lxl_dfss_storage_send_request_body_handler;

	rc = lxl_dfss_storage_do_send_request_body(r, u);

	return rc;
}

static void
lxl_dfss_storage_send_request_body_handler(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u)
{
	int	rc;


	rc = lxl_dfss_storage_do_send_request_body(r, u);
	
	
	return;
}

static int
lxl_dfss_storage_do_send_request_body(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u)
{
	size_t 			   		  size;
	ssize_t			   		  n;
	lxl_buf_t		  		 *b;
	lxl_file_t				 *file;
	lxl_connection_t  		 *c;
	lxl_dfss_request_body_t  *rb;

	rb = r->body;
	file = &rb->file;
	b = &u->buffer;
	c = u->peer.connection;
	
	for (; ;) {
		for (; ;) {
			size = b->last - b->pos;
			if (size == 0) {
				lxl_reset_buf(b);
				size = b->end - b->last;
				if (rb->rest < (off_t) size) {
					size = rb->rest;
				}

				n = lxl_read_file1(file, b->last, size);
				if (n == -1) {
					return -1;
				}

				b->last += n;
			}
			
			n = c->send(c, b->pos, size);
			if (n == LXL_EAGAIN) {
			}

			if (n == LXL_ERROR) {
				return -1;
			}

			b->pos += n;
			rb->rest -= n;

			if (rb->rest == 0) {
				break;
			}

			if (b->pos < b->last) {
				break;
			}
		}

		lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss client response body rest %ld", rb->rest);
		if (rb->rest == 0) {
			break;
		}

		if (!c->write->ready) {
			lxl_add_timer(c->write, 60000);
			if (lxl_handle_write_event(c->write, 0) != 0) {
				return -1;
			}

			return LXL_EAGAIN;
		}
	}

	/*if (close(file->fd) == -1) {
		lxl_log_error(LXL_LOG_WARN, errno, "close() failed");
	} tongyi chu li */

	if (c->write->timer_set) {
		lxl_del_timer(c->write);
	}

	u->read_event_handler = lxl_dfss_storage_process_response_header;
	u->write_event_handler = lxl_dfss_upstream_request_empty_handler;

	//u->post_handler(r, u);
	c->read->timedout = 0;
	lxl_add_timer(c->read, 5000);

	return LXL_OK;
}


static void
lxl_dfss_storage_process_response_header(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u)
{
	size_t			   nread;
	ssize_t			   n;
	lxl_buf_t 		  *b;
	lxl_event_t       *rev;
	lxl_connection_t  *c;

	c = u->peer.connection;
	rev = c->read;

	if (rev->timedout) {
		lxl_log_error(LXL_LOG_ERROR, 0, "client timed out");
		// next
		lxl_dfss_storage_finalize_request(r, u, LXL_DFS_REQUEST_TIMEOUT);
		return;
	}

	b = u->header_buf;
	lxl_reset_buf(b);

	n = c->recv(c, b->last, b->end - b->last);
	if (n == LXL_EAGAIN) {
		return;
	}

	if (n == LXL_ERROR) {
		// next
		return;
	}

	if (n == 0) {
		lxl_log_error(LXL_LOG_INFO, 0, "storage client closed connection");
		// next
		return;
	}

	b->last += n;
	nread = b->last - b->pos;
	if (nread >= sizeof(lxl_dfs_response_header_t)) {
		lxl_dfss_upstream_parse_response(r, u, b);
		
		lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss storage process response: %s, %u, %hu", 
					r->rid, u->response_header.body_n, u->response_header.rcode);

		lxl_dfss_storage_next(r, u, u->response_header.rcode);
	}
	
	return;
}

static void
lxl_dfss_storage_dummy_handler(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u)
{
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss storage dummy handler");

	return;
}

static void
lxl_dfss_storage_finalize_request(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u, lxl_int_t rc)
{
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss storage finalize request, %s, %ld", r->rid, rc);

	if (u->peer.connection) {
		lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "close dfss storage connetion: %d", u->peer.connection->fd);
		
		/* connection pool is null */
		lxl_close_connection(u->peer.connection);
	}

	if (r->handler) {
		r->handler(r);
		return;
	}

	lxl_dfss_finalize_request(r, rc);
}

static void
lxl_dfss_storage_next(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u, lxl_uint_t ft_type)
{
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss next storage, %lu", ft_type);

	if (u->peer.connection) {
		lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "close dfss storage connection: %d", u->peer.connection->fd);

#if 0
		if (u->peer.connetion->pool) {
			lxl_destroy_pool(u->peer.connetion->pool);
		}
#endif

		lxl_close_connection(u->peer.connection);
		u->peer.connection = NULL;
	}

	lxl_dfss_storage_connect(r, u);
}

static void *
lxl_dfss_storage_create_conf(lxl_conf_t *cf)
{
	lxl_dfss_storage_srv_conf_t  *conf;

	conf = lxl_pcalloc(cf->pool, sizeof(lxl_dfss_storage_srv_conf_t));
	if (conf == NULL) {
		return NULL;
	}

	conf->upstream.connect_timeout = LXL_CONF_UNSET_MSEC;
	conf->upstream.send_timeout = LXL_CONF_UNSET_MSEC;
	conf->upstream.read_timeout = LXL_CONF_UNSET_MSEC;
	conf->upstream.send_timeout = LXL_CONF_UNSET_SIZE;

	return conf;
}

static char *
lxl_dfss_storage_init_conf(lxl_conf_t *cf, void *conf)
{
	lxl_dfss_storage_srv_conf_t  *sscf = conf;

	lxl_conf_init_msec_value(sscf->upstream.connect_timeout, 6000);
	lxl_conf_init_msec_value(sscf->upstream.send_timeout, 10000);
	lxl_conf_init_msec_value(sscf->upstream.read_timeout, 10000);
	lxl_conf_init_size_value(sscf->upstream.send_lowat, 0);

	return LXL_CONF_OK;
}

static char *
lxl_dfss_storage_pass(lxl_conf_t *cf, lxl_command_t *cmd, void *conf)
{
	lxl_dfss_storage_srv_conf_t  *sscf = conf;

	lxl_str_t  *value;
	lxl_url_t   u;

	if (sscf->upstream.upstream) {
		return "is duplicate";
	}

	memset(&u, 0x00, sizeof(lxl_url_t));
	value = lxl_array_elts(cf->args);
	u.host = value[1];
	sscf->upstream.upstream = lxl_dfss_upstream_add(cf, &u, LXL_DFSS_UPSTREAM_STORAGE);
	if (sscf->upstream.upstream == NULL) {
		return LXL_CONF_ERROR;
	}

	return LXL_CONF_OK;
}

static char *
lxl_dfss_storage_lowat_check(lxl_conf_t *cf, void *post, void *data)
{
	lxl_conf_log_error(LXL_LOG_WARN, cf, 0, "storage_send_lowat is not suport, ignored");

	return LXL_CONF_OK;
}
