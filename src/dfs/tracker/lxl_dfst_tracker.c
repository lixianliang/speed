
/*
 * Copyright (C) xianliang.li
 * use in tracker data recovery
 */


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_dfst.h>


typedef struct {
	lxl_dfst_upstream_conf_t  upstream;
} lxl_dfst_tracker_srv_conf_t;


static void lxl_dfst_tracker_init_request(lxl_dfst_request_t *r, lxl_dfst_upstream_t *u);
static int	lxl_dfst_tracker_create_request(lxl_dfst_request_t *r, lxl_dfst_upstream_t *u);
static void	lxl_dfst_tracker_udp_handler(lxl_event_t *rev);
static void	lxl_dfst_tracker_process_udp_response_header(lxl_dfst_request_t *r, lxl_dfst_upstream_t *u);
static void lxl_dfst_tracker_process_udp_response_body(lxl_dfst_request_t *r, lxl_dfst_upstream_t *u);
static void lxl_dfst_tracker_next_udp(lxl_dfst_request_t *r, lxl_dfst_upstream_t *u, lxl_uint_t ft_type);

static void lxl_dfst_tracker_handler(lxl_event_t *ev);

static void lxl_dfst_tracker_connect(lxl_dfst_request_t *r, lxl_dfst_upstream_t *u);
static void lxl_dfst_tracker_send_request(lxl_dfst_request_t *r, lxl_dfst_upstream_t *u);
static void lxl_dfst_tracker_process_header(lxl_dfst_request_t *r, lxl_dfst_upstream_t *u);

static void lxl_dfst_tracker_dummy_handler(lxl_dfst_request_t *r, lxl_dfst_upstream_t *u);
static void lxl_dfst_tracker_next(lxl_dfst_request_t *r, lxl_dfst_upstream_t *u, lxl_uint_t ft_type);
static void lxl_dfst_tracker_finalize_request(lxl_dfst_request_t *r, lxl_dfst_upstream_t *u, lxl_int_t rc);

static void *lxl_dfst_tracker_create_conf(lxl_conf_t *cf);
static char *lxl_dfst_tracker_init_conf(lxl_conf_t *cf, void *conf);
static char *lxl_dfst_tracker_pass(lxl_conf_t *cf, lxl_command_t *cmd, void *conf);
static char *lxl_dfst_tracker_lowat_check(lxl_conf_t *cf, void *post, void *data);

static lxl_conf_post_t  lxl_dfst_tracker_lowat_post = { lxl_dfst_tracker_lowat_check };

lxl_module_t  lxl_dfst_tracker_module;

static lxl_command_t  lxl_dfst_tracker_commands[] = {
	
	{ lxl_string("tracker_pass"),
	  LXL_DFST_SRV_CONF|LXL_CONF_TAKE1,
	  lxl_dfst_tracker_pass,
	  LXL_DFST_SRV_CONF_OFFSET,
	  0,
	  NULL },

	{ lxl_string("tracker_connect_timeout"),
	  LXL_DFST_MAIN_CONF|LXL_DFST_SRV_CONF|LXL_CONF_TAKE1,
	  lxl_conf_set_msec_slot,
	  LXL_DFST_SRV_CONF_OFFSET,
	  offsetof(lxl_dfst_tracker_srv_conf_t, upstream.connect_timeout),
	  NULL },

	{ lxl_string("tracker_read_timeout"),
	  LXL_DFST_MAIN_CONF|LXL_DFST_SRV_CONF|LXL_CONF_TAKE1,
	  lxl_conf_set_msec_slot,
	  LXL_DFST_SRV_CONF_OFFSET,
	  offsetof(lxl_dfst_tracker_srv_conf_t, upstream.read_timeout),
	  NULL },

	{ lxl_string("tracker_send_lowat"),
	  LXL_DFST_MAIN_CONF|LXL_DFST_SRV_CONF|LXL_CONF_TAKE1,
	  lxl_conf_set_size_slot,
	  LXL_DFST_SRV_CONF_OFFSET,
	  offsetof(lxl_dfst_tracker_srv_conf_t, upstream.send_lowat),
	  &lxl_dfst_tracker_lowat_post },

	lxl_null_command
};

lxl_dfst_module_t  lxl_dfst_tracker_module_ctx = {
	NULL,
	NULL,
	lxl_dfst_tracker_create_conf,
	lxl_dfst_tracker_init_conf
};

lxl_module_t  lxl_dfst_tracker_module = {
	0,
	0,
	(void *) &lxl_dfst_tracker_module_ctx,
	lxl_dfst_tracker_commands,
	LXL_DFST_MODULE,
	NULL,
	NULL
};

void
lxl_dfst_tracker_udp_init(lxl_dfst_request_t *r)
{
	/*lxl_dfst_upstream_t  		 *u;
	lxl_dfst_tracker_srv_conf_t  *sscf;
	
	u = lxl_pcalloc(r->pool, sizeof(lxl_dfst_upstream_t));
	if (u == NULL) {
		return;
	}

	r->tracker = u;

	if (lxl_queue_init(&u->sockaddrs, r->pool, 2, sizeof(lxl_dfs_sockaddr_t)) == -1) {
		return;
	}

	sscf = lxl_dfst_get_module_srv_conf(r, lxl_dfst_tracker_module);
	u->conf = &sscf->upstream;
	
	lxl_dfst_tracker_init_request(r);*/
}

void
lxl_dfst_tracker_init(lxl_dfst_request_t *r)
{
	lxl_dfst_upstream_t 		 *u;
	lxl_dfst_tracker_srv_conf_t  *sscf;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst init tracker request, %s", r->rid);

	u = lxl_pcalloc(r->pool, sizeof(lxl_dfst_upstream_t));
	if (u == NULL) {
		return;
	}

	r->tracker = u;

	if (lxl_queue_init(&u->addrs, r->pool, 2, sizeof(lxl_addr_t)) == -1) {
		// finalize
		return;
	}

	sscf = lxl_dfst_get_module_srv_conf(r, lxl_dfst_tracker_module);
	u->conf = &sscf->upstream;

	lxl_dfst_tracker_init_request(r, u);
}

static void
lxl_dfst_tracker_init_request(lxl_dfst_request_t *r, lxl_dfst_upstream_t *u)
{
	lxl_uint_t					   i, nelts;
	//lxl_dfs_sockaddr_t			  *elt;
	lxl_addr_t					  *elt;
	//lxl_dfst_upstream_t			  *u;
	lxl_dfst_upstream_server_t    *server;
	lxl_dfst_upstream_srv_conf_t  *uscf;

	lxl_dfst_tracker_create_request(r, u);

	uscf = u->conf->upstream;

	if (uscf == NULL) {
		lxl_log_error(LXL_LOG_ALERT, 0, "no upstream configuration");
		return;
	}

	/*if (r->request_header.qtype == LXL_DFS_TRACKER_SYNC_FID) {
		if (uscf->peer.init(r, uscf, u) != 0) {
			// lxl_dfst
		}

		lxl_dfst_tracker_connect(r, u);
	} else {*/
		nelts = lxl_array_nelts(uscf->servers);
		for (i = 0; i < nelts; ++i) {
			server = lxl_array_data(uscf->servers, lxl_dfst_upstream_server_t, i);
			elt = lxl_queue_in(&u->addrs);
			if (elt == NULL) {
				return;
			}

			elt->sockaddr = (struct sockaddr *) server->sockaddr;
			elt->socklen = server->socklen;
			snprintf(elt->name, sizeof(elt->name), "%s", server->name);
			//elt->name = server->name;
		}

		if (lxl_event_udp_connect_peer(&u->peer) == -1) {
			return;
		}

		u->peer.connection->data = r;
		u->peer.connection->read->handler = lxl_dfst_tracker_udp_handler;

		lxl_dfst_tracker_next_udp(r, u, LXL_OK);
	//}
}

static int
lxl_dfst_tracker_create_request(lxl_dfst_request_t *r, lxl_dfst_upstream_t *u)
{
	size_t					   size;
	lxl_buf_t				  *b, *b1, *b2;
	//lxl_dfs_storage_info_t    *storage_info;
	lxl_dfs_request_header_t  *reqh;
	
	b = r->header_buf;
	u->request_header = r->request_header;
	reqh = &u->request_header;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst tracker request qtype 0x%04x", reqh->qtype);

	size = 64;

	b1 = u->request_buf = lxl_create_temp_buf(r->pool, size);
	if (b1 == NULL) {
		return -1;
	}

	/*if (reqh->qtype == LXL_DFS_TRACKER_SYNC_FID) {
		size = 65535;
	}*/

	b2 = u->response_buf = lxl_create_temp_buf(r->pool, size);
	if (b2 == NULL) {
		return -1;
	}

	switch (reqh->qtype) {
	case LXL_DFS_STORAGE_REPORT_STATE:
	case LXL_DFS_STORAGE_REPORT_FID:
	case LXL_DFS_DOWNLOAD:
	case LXL_DFS_DELETE:
	//case LXL_DFS_STAT:
		size = b->last - b->start;
		memcpy(b1->last, b->start, size);
		reqh->body_n = lxl_dfst_request_seed++;
		*((uint32_t *) b1->last) = htonl(reqh->body_n);
		size = lxl_max(size, LXL_DFS_MIN_UDP_LENGTH);
		b1->last += size;
		break;
	//default:	/* LXL_DFS_TRACKER_SYNC_FID */
	//	break;
	}

	return 0;
}

static void
lxl_dfst_tracker_udp_handler(lxl_event_t *ev)
{
	ssize_t				  n;
	lxl_buf_t			 *b;
	lxl_connection_t  	 *c;
	lxl_dfst_request_t   *r;
	lxl_dfst_upstream_t  *u;

	c = ev->data;
	r = c->data;
	u = r->tracker;

	b = u->response_buf;
	lxl_reset_buf(b);

	if (c->read->timedout) {
		lxl_dfst_tracker_next_udp(r, u, LXL_DFST_UPSTREAM_FT_TIMEOUT);
		return;
	}

	n = c->recv(c, b->pos, b->end - b->pos);

	if (n < 0) {
		//lxl_dfst_tracker_finalize_request(r, u, LXL_DFS_SERVER_ERROR);
		lxl_dfst_tracker_next_udp(r, u, LXL_DFS_SERVER_ERROR);
		return;
	}

	b->last = b->pos + n;

	lxl_dfst_tracker_process_udp_response_header(r, u);
}

static void
lxl_dfst_tracker_process_udp_response_header(lxl_dfst_request_t *r, lxl_dfst_upstream_t *u)
{
	int 						rc;
	char 					   *err = "";
	size_t						n;
	lxl_uint_t				    ft_type = LXL_DFS_OK;
	lxl_buf_t  				   *b;
	lxl_dfs_request_header_t   *reqh;
	lxl_dfs_response_header_t  *resh;
	
	b = u->response_buf;
	n = b->last - b->pos;

	if (n < 8) {
		ft_type = LXL_DFST_UPSTREAM_FT_INVALID_RESPONSE;
		err = "dfst tracker short response";

		goto invalid;
	}
		
	reqh = &u->request_header;
	resh = &u->response_header;

	resh->body_n =  ntohl(*((uint32_t *) b->pos));
	b->pos += 6;
	resh->rcode = ntohs(*((uint16_t *) b->pos));
	b->pos += 2;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst tracker process udp response");

	if (resh->body_n != reqh->body_n) {
#if  1
		lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "request id %u, response id %u", reqh->body_n, resh->body_n);
#endif 
		ft_type = LXL_DFST_UPSTREAM_FT_INVALID_PACKAGE;
		err = "dfst tarcker request id not equal response id";
		goto invalid;
	}

	/*if (r->rcode != LXL_DFS_OK) {
		err = "dfst tracker response rcode not LXL_DFS_OK";
		ft_type = r->rcode;

		goto invalid;
	}*/

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "tracker response rcode %hu", resh->rcode);

	if (reqh->qtype == LXL_DFS_DELETE
		|| reqh->qtype == LXL_DFS_STORAGE_REPORT_STATE
		|| reqh->qtype == LXL_DFS_STORAGE_REPORT_FID) {
		lxl_dfst_tracker_next_udp(r, u, resh->rcode);
	} else {
		/* LXL_DFS_DOWNLOAD LXL_DFS_STAT */
		lxl_dfst_tracker_process_udp_response_body(r, u);
	}


	return;

invalid:

	lxl_log_error(LXL_LOG_ERROR, 0, err);
	
	lxl_dfst_tracker_next_udp(r, u, ft_type);
	
	return;
}

static void
lxl_dfst_tracker_process_udp_response_body(lxl_dfst_request_t *r, lxl_dfst_upstream_t *u)
{
	lxl_uint_t		    nelts;
	lxl_buf_t  		   *b;
	lxl_dfs_ip_port_t  *elt;
	
	b = u->response_buf;
	while (b->pos + 6 <= b->last) {
		elt = lxl_array_push(&r->storages);
		if (elt == NULL) {
			lxl_dfst_tracker_finalize_request(r, u, LXL_ERROR);
			return;
		}

		elt->ip = (uint32_t *)  b->pos;
		b->pos += 4;
		elt->port = (uint16_t *) b->pos;
		b->pos += 2;
	}

	nelts = lxl_array_nelts(&r->storages);
	if (nelts > 0) {
		lxl_dfst_tracker_finalize_request(r, u, LXL_DFS_OK);
	} else {
		lxl_dfst_tracker_next_udp(r, u, LXL_DFST_UPSTREAM_FT_ERROR);
	}
}

static void
lxl_dfst_tracker_next_udp(lxl_dfst_request_t *r, lxl_dfst_upstream_t *u, lxl_uint_t ft_type)
{
	size_t				 len;
	ssize_t 			 n;
	struct sockaddr_in  *sin;
	lxl_buf_t 			   *b;
	lxl_addr_t			 *addr;
	lxl_connection_t     *c;
	//lxl_dfs_sockaddr_t  *sa;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst next tracker udp, %lu", ft_type);

	if (ft_type == LXL_DFS_OK) {
		++r->number;
	}
		
	c = u->peer.connection;
	if (ft_type == LXL_DFST_UPSTREAM_FT_TIMEOUT && u->tries < 1) {
		/* two */
		++u->tries;
	} else {
		addr = lxl_queue_out(&u->addrs);
		if (addr == NULL) {
			lxl_dfst_tracker_finalize_request(r, u, LXL_DFST_UPSTREAM_FT_TRY_OVER);
			return;
		}

		c->sockaddr = *(addr->sockaddr);
		c->socklen  = addr->socklen;
		//u->peer.name = addr->name;
		u->tries = 0;

		//sin = (struct sockaddr_sin *) &c->sockaddr;
		lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "request to tracker %s", addr->name);
	}

	b = u->request_buf;
	len = b->last - b->pos;

	/* sendto network problem */
	n = c->send(c, b->pos, len);
	if (n < 0) {
		goto failed;
	}

	if ((size_t) n != len) {
		lxl_log_error(LXL_LOG_ERROR, errno, "send() imcomplete");
		goto failed;
	}

	if (!c->read->active) {
		if (lxl_add_event(c->read, LXL_READ_EVENT, LXL_CLEAR_EVENT) == -1) {
			goto failed;
		}
	}

	// timedout
	c->read->timedout = 0;
	lxl_add_timer(c->read, 5000);

	return;

failed:

	lxl_dfst_tracker_finalize_request(r, u, LXL_ERROR);

	return;
}

static void
lxl_dfst_tracker_handler(lxl_event_t *ev)
{
	lxl_connection_t 	 *c;
	lxl_dfst_request_t   *r;
	lxl_dfst_upstream_t  *u;

	c = ev->data;
	r = c->data;
	u = r->tracker;

	if (ev->write) {
		u->write_event_handler(r, u);
	} else {
		u->read_event_handler(r, u);
	}

	// posted_request;
}

static void
lxl_dfst_tracker_connect(lxl_dfst_request_t *r, lxl_dfst_upstream_t *u)
{
	int				   rc;
	lxl_connection_t  *c;

	rc = lxl_event_connect_peer(&u->peer);
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst tracker connect: %d", rc);
	if (rc == LXL_ERROR) {
		// finialize
		return;
	}

	if (rc == LXL_BUSY) {
		lxl_log_error(LXL_LOG_ERROR, 0, "dfst no live trackers");
		lxl_dfst_tracker_next(r, u, LXL_DFST_UPSTREAM_FT_NOLIVE);
		return;
	}

	if (rc == LXL_DECLINED) {
		lxl_dfst_tracker_next(r, u, LXL_DFST_UPSTREAM_FT_ERROR);
		return;
	}

	c = u->peer.connection;
	c->data = r;
	c->write->handler = lxl_dfst_tracker_handler;
	c->read->handler = lxl_dfst_tracker_handler;

	if (rc == LXL_EAGAIN) {
		lxl_add_timer(c->write, 3000);
		return;
	}

	lxl_dfst_tracker_send_request(r, u);
}

static void
lxl_dfst_tracker_send_request(lxl_dfst_request_t *r, lxl_dfst_upstream_t *u)
{
	int				   rc;
	lxl_connection_t  *c;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst tracker send request");

	c = u->peer.connection;
	if (rc == LXL_EAGAIN) {
		if (lxl_handle_write_event(c->write, 0) != 0) {
			return;
		}

		return;
	}

	lxl_add_timer(c->read, 3000);
	if (c->read->ready) {
		lxl_dfst_tracker_process_header(r, u);
	}

	if (lxl_handle_write_event(c->write, 0) != 0) {
		// finalize
		return;
	}
}

static void
lxl_dfst_tracker_process_header(lxl_dfst_request_t *r, lxl_dfst_upstream_t *u)
{
	return;
}

static void
lxl_dfst_tracker_dummy_handler(lxl_dfst_request_t *r, lxl_dfst_upstream_t *u)
{
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst tracker dummy handler");

	return;
}

static void
lxl_dfst_tracker_next(lxl_dfst_request_t *r, lxl_dfst_upstream_t *u, lxl_uint_t ft_type)
{
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst next tracker, %lu", ft_type);
}

static void lxl_dfst_tracker_finalize_request(lxl_dfst_request_t *r, lxl_dfst_upstream_t *u, lxl_int_t rc)
{
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst tracker finalize request: %s, %ld", r->rid, rc);

	if (u->peer.free && u->peer.sockaddr) {
		u->peer.free(&u->peer, u->peer.data, 0);
		u->peer.sockaddr = NULL;
	}

	if (u->peer.connection) {
		lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "close dfst tracker connection: %d", u->peer.connection->fd);
		lxl_close_connection(u->peer.connection);
	}

	if (rc == LXL_DFS_OK || rc == LXL_DFST_UPSTREAM_FT_TRY_OVER) {
		r->handler(r);
		return;
	}

	/*if ( 
			&& (u->request_header->type == LXL_DFS_STORAGE_REPORT_STATE
				|| u->request_header->type == LXL_DFS_STORAGE_REPORT_FID)) {

		 u->request_header->type == LXL_DFS_DELETE all success
	} else {
		lxl_dfst_finalize_request(r, rc);
	}*/

	lxl_dfst_finalize_request(r, LXL_DFS_SERVER_ERROR);

	// rc is chanage
}

static void *
lxl_dfst_tracker_create_conf(lxl_conf_t *cf)
{
	lxl_dfst_tracker_srv_conf_t  *conf;

	conf = lxl_pcalloc(cf->pool, sizeof(lxl_dfst_tracker_srv_conf_t));
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
lxl_dfst_tracker_init_conf(lxl_conf_t *cf, void *conf)
{
	lxl_dfst_tracker_srv_conf_t  *sscf = conf;

	lxl_conf_init_msec_value(sscf->upstream.connect_timeout, 6000);
	lxl_conf_init_msec_value(sscf->upstream.send_timeout, 10000);
	lxl_conf_init_msec_value(sscf->upstream.read_timeout, 10000);
	lxl_conf_init_size_value(sscf->upstream.send_lowat, 0);

	return LXL_CONF_OK;
}

static char *
lxl_dfst_tracker_pass(lxl_conf_t *cf, lxl_command_t *cmd, void *conf)
{
	lxl_dfst_tracker_srv_conf_t  *sscf = conf;

	lxl_str_t  *value;
	lxl_url_t   u;

	if (sscf->upstream.upstream) {
		return "is duplicate";
	}

	memset(&u, 0x00, sizeof(lxl_url_t));
	value = lxl_array_elts(cf->args);
	u.host = value[1];
	sscf->upstream.upstream = lxl_dfst_upstream_add(cf, &u, 0);
	if (sscf->upstream.upstream == NULL) {
		return LXL_CONF_ERROR;
	}

	return LXL_CONF_OK;
}

static char *
lxl_dfst_tracker_lowat_check(lxl_conf_t *cf, void *post, void *data)
{
	lxl_conf_log_error(LXL_LOG_WARN, cf, 0, "tracker_send_lowat is not supoort, ignored");

	return LXL_CONF_OK;
}
