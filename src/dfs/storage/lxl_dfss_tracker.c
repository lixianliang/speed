
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_dfss.h>


typedef struct {
	lxl_dfss_upstream_conf_t  upstream;
} lxl_dfss_tracker_srv_conf_t;


//static void	lxl_dfss_tracker_init(lxl_dfss_request_t *r);
static void lxl_dfss_tracker_init_request(lxl_dfss_request_t *r);
static int 	lxl_dfss_tracker_create_request(lxl_dfss_request_t *r);

static void lxl_dfss_tracker_handler(lxl_event_t *ev);

static void lxl_dfss_tracker_connect(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u);
static void lxl_dfss_tracker_send_request(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u);
static void lxl_dfss_tracker_process_header(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u);
static ssize_t lxl_dfss_tracker_read_response_header(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u);
//static int	lxl_dfss_tracker_parse_response(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u, char *data);
static void lxl_dfss_tracker_process_response(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u);
static int	lxl_dfss_read_tracker_response_body(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u);
static void lxl_dfss_read_tracker_response_body_handler(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u);
static int	lxl_dfss_do_read_tracker_response_body(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u);
static int	lxl_dfss_tracker_report_fid_response_body(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u);
static int	lxl_dfss_tracker_sync_fid_response_body(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u);
static int	lxl_dfss_tracker_sync_fid_handle_response(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u);
static int	lxl_dfss_tracker_dummy_response_body(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u);

static void lxl_dfss_tracker_dummy_handler(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u);
static void lxl_dfss_tracker_next(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u, lxl_uint_t ft_type);
static void lxl_dfss_tracker_finalize_request(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u, lxl_int_t rc);

//static void	lxl_dfss_tracker_set_keepalive(lxl_dfss_request_t *r)

static void *lxl_dfss_tracker_create_conf(lxl_conf_t *cf);
static char *lxl_dfss_tracker_init_conf(lxl_conf_t *cf, void *conf);
static char *lxl_dfss_tracker_pass(lxl_conf_t *cf, lxl_command_t *cmd, void *conf);
static char *lxl_dfss_tracker_lowat_check(lxl_conf_t *cf, void *post, void *data);

static lxl_conf_post_t  lxl_dfss_tracker_lowat_post = { lxl_dfss_tracker_lowat_check };

lxl_module_t  lxl_dfss_tracker_module;

static lxl_command_t  lxl_dfss_tracker_commands[] = {
	
	{ lxl_string("tracker_pass"),
 	  LXL_DFSS_SRV_CONF|LXL_CONF_TAKE1,
	  lxl_dfss_tracker_pass,
	  LXL_DFSS_SRV_CONF_OFFSET,
	  0,
	  NULL },
	
	{ lxl_string("tracker_connect_timeout"),
	  LXL_DFSS_MAIN_CONF|LXL_DFSS_SRV_CONF|LXL_CONF_TAKE1,
	  lxl_conf_set_msec_slot,
	  LXL_DFSS_SRV_CONF_OFFSET,
  	  offsetof(lxl_dfss_tracker_srv_conf_t, upstream.connect_timeout),
	  NULL },
		
	{ lxl_string("tracker_send_timeout"),
	  LXL_DFSS_MAIN_CONF|LXL_DFSS_SRV_CONF|LXL_CONF_TAKE1,
	  lxl_conf_set_msec_slot,
	  LXL_DFSS_SRV_CONF_OFFSET,
	  offsetof(lxl_dfss_tracker_srv_conf_t, upstream.send_timeout),
	  NULL },

	{ lxl_string("tracker_read_timeout"),
	  LXL_DFSS_MAIN_CONF|LXL_DFSS_SRV_CONF|LXL_CONF_TAKE1,
	  lxl_conf_set_msec_slot,
	  LXL_DFSS_SRV_CONF_OFFSET,
	  offsetof(lxl_dfss_tracker_srv_conf_t, upstream.read_timeout),
	  NULL },

	{ lxl_string("tracker_send_lowat"),
	  LXL_DFSS_MAIN_CONF|LXL_DFSS_SRV_CONF|LXL_CONF_TAKE1,
	  lxl_conf_set_size_slot,
	  LXL_DFSS_SRV_CONF_OFFSET,
	  offsetof(lxl_dfss_tracker_srv_conf_t, upstream.send_lowat),
	  &lxl_dfss_tracker_lowat_post }, 
	
	lxl_null_command
};

lxl_dfss_module_t  lxl_dfss_tracker_module_ctx = {
	NULL,
	NULL,
	lxl_dfss_tracker_create_conf,
	lxl_dfss_tracker_init_conf
};

lxl_module_t  lxl_dfss_tracker_module ={
	0,
	0,
	(void *) &lxl_dfss_tracker_module_ctx,
	lxl_dfss_tracker_commands,
	LXL_DFSS_MODULE,
	NULL,
	NULL
};


/*int
lxl_dfss_tracker_create(lxl_dfss_request_t *r)
{
	lxl_dfss_upstream_t *u;

	u = r->tracker;
	u = lxl_pcalloc(r->pool, sizeof(lxl_dfss_upstream_t));
	if (u == NULL) {
		return -1;
	}

	r->tracker = u;
	
	return 0;
}*/

/*int
lxl_dfss_upstream_tracker_handler(lxl_dfss_request_t *r)
{
	// upstream_create
	// upstream_init
	// upstream_init_request
	lxl_dfss_upstream_t *u;
	
	u = r->tracker;
	if (u == NULL) {
		u = lxl_pcalloc(r->pool, sizeof(lxl_dfss_upstream_t));
		if (u == NULL) {
			return -1;
		}
		
		r->tracker = u;
	}

	lxl_dfss_tracker_init(r);

	return 0;
}*/

//static 
void 
lxl_dfss_tracker_init(lxl_dfss_request_t *r)
{
	lxl_dfss_upstream_t 		 *u;
	lxl_dfss_tracker_srv_conf_t  *tscf;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss init tracker request, %s", r->rid);
	
	u = r->tracker;
	// upstream_cleanup()
	if (u == NULL) {
		u = lxl_pcalloc(r->pool, sizeof(lxl_dfss_upstream_t));
		if (u == NULL) {
			// finalize
			return;
		}

		u->header_buf = lxl_create_temp_buf(r->pool, 64);
		if (u->header_buf == NULL) {
			// finalize
			return;
		}
		
		r->tracker = u;
	}

	tscf = lxl_dfss_get_module_srv_conf(r, lxl_dfss_tracker_module);
	u->conf = &tscf->upstream;

	lxl_dfss_tracker_init_request(r);
}

static void
lxl_dfss_tracker_init_request(lxl_dfss_request_t *r)
{
	//直接get upstream module 获得配置信息
	lxl_dfss_upstream_t *u;
	lxl_dfss_upstream_srv_conf_t	*uscf;
	//lxl_dfss_upstream_main_conf_t 	*umcf;

	lxl_dfss_tracker_create_request(r);
	// peer tianjia
	
	u = r->tracker;
	uscf= u->conf->upstream;

	if (uscf == NULL) {
		lxl_log_error(LXL_LOG_ALERT, 0, "no upstream configuration");
		// lxl_dfss_upstream
		return;
	}

	if(uscf->peer.init(r, uscf, u) != 0) {
		//lxl_dfss_upstream_finalize_request(r, u, LXL_DFSS_INTERNAL_SERVER_ERROR);
	}

	lxl_dfss_tracker_connect(r, u);
}

static int
lxl_dfss_tracker_create_request(lxl_dfss_request_t *r)
{
	// request type === use request header buf
	size_t 				  	   size, old_size;
	lxl_str_t				  *fid;
	lxl_buf_t 			 	  *b;
	lxl_dfss_upstream_t  	  *u;
	lxl_dfs_request_header_t  *header;
	
	u = r->tracker;
	header = &u->request_header;
	//header->qtype = htons(8);

	switch (r->request_header.qtype) {
	case LXL_DFS_STORAGE_REPORT_STATE:
		size = 64;
		header->qtype = LXL_DFS_STORAGE_REPORT_STATE;
		break;
	
	case LXL_DFS_UPLOAD:
	case LXL_DFS_UPLOAD_SC:
		size = 64;
		/*if (!r->sync) {
			header->qtype = LXL_DFS_STORAGE_REPORT_FID;
		} else {
			header->qtype = LXL_DFS_STORAGE_SYNC_PUSH;
		}*/
		header->qtype = LXL_DFS_STORAGE_REPORT_FID;
		break;

	default:
		lxl_log_error(LXL_LOG_ERROR, 0, "dfss tracker Unknow request type 0x%04x", r->request_header.qtype);
		return -1;	
	}

	/*switch (request_header->qtype) {
	case LXL_DFS_STORAGE_REPORT_STATE:
		size =  24;
		break;

	case LXL_DFS_UPLOAD_STRONG:
	case LXL_DFS_STORAGE_SYNC_PUSH:
	//case LXL_DFS_STORAGE_SYNC_FID_STRONG:
		size = 20;	
		break;

	case LXL_DFS_STORAGE_REPORT_FID:
		size = 4096;
		break;

	case LXL_DFS_STORAGE_SYNC_FID:
		size = 65536;
		break;

	default:

		lxl_log_error(LXL_LOG_ERROR, 0, "dfss tracker Unknow request type 0x%04x", request_header->qtype);
		return -1;
	}*/

	b = &u->buffer;
	if (b->start == NULL) {
		b->start = lxl_pnalloc(r->pool, size);
		if (b->start == NULL) {
			return -1;
		}

		b->pos = b->start;
		b->last = b->start;
		b->end = b->start + size;
	} else {
		old_size = b->end - b->start;	
		if (old_size < size) {
			if (lxl_pfree(r->pool, b->start) == 0) {
				lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss pfree %lu, pnalloc %lu", old_size, size);
			}

			b->start= lxl_pnalloc(r->pool, size);
			if (b->start == NULL) {
				return -1;
			}

			b->pos = b->start;
			b->last = b->start;
			b->end = b->start + size;
		} else {
			b->pos = b->start;
			b->last = b->start;
		}
	}

	switch (header->qtype) {
	case LXL_DFS_STORAGE_REPORT_STATE:
		*((uint32_t *) b->last) = htonl(16);
		b->last += 4;
		*((uint16_t *) b->last) = 0;
		b->last += 2;
		*((uint16_t *) b->last) = htons(header->qtype);
		b->last += 2;

		*((uint16_t *) b->last) = htons(lxl_dfss_idc_id);
		b->last += 2;
		*((uint16_t *) b->last) = lxl_dfss_ip_port.port;
		b->last += 2;
		*((uint32_t *) b->last) = lxl_dfss_ip_port.ip;
		b->last += 4;

		*((uint16_t *) b->last) = htons(lxl_dfss_loadavg5);
		b->last += 2;
		*((uint16_t *) b->last) = htons(lxl_dfss_cpu_idle);
		b->last += 2;
		*((uint32_t *) b->last) = htonl(lxl_dfss_disk_free_mb);
		b->last += 4;
		break;

	case LXL_DFS_STORAGE_REPORT_FID:
	case LXL_DFS_STORAGE_REPORT_SYNC_FID:
		fid = &r->body->fid;
		*((uint32_t *) b->last) = htonl(8 + fid->len);
		b->last += 4;
		*((uint16_t *) b->last) = 0;
		b->last += 2;
		*((uint16_t *) b->last) = htons(header->qtype);
		b->last += 2;

		*((uint16_t *) b->last) = htons(lxl_dfss_idc_id);
		b->last += 2;
		*((uint16_t *) b->last) = lxl_dfss_ip_port.port;
		b->last += 2;
		*((uint32_t *) b->last) = lxl_dfss_ip_port.ip;
		b->last += 4;
		memcpy(b->last, fid->data, fid->len);
		b->last += fid->len;
		break;

	//case LXL_DFS_UPLOAD_STRONG:
	case LXL_DFS_STORAGE_SYNC_PUSH:
		*((uint32_t *) b->last) = htonl(8);
		b->last += 4;
		*((uint16_t *) b->last) = 0;
		b->last += 2;
		*((uint16_t *) b->last) = htons(header->qtype);
		b->last += 2;

		*((uint16_t *) b->last) = htons(lxl_dfss_idc_id);
		b->last += 2;
		*((uint16_t *) b->last) = lxl_dfss_ip_port.port;
		b->last += 2;
		*((uint32_t *) b->last) = lxl_dfss_ip_port.ip;
		b->last += 4;
		break;

	/*case LXL_DFS_STORAGE_SYNC_FID:
		*((uint32_t *) b->last) = htonl(10);
		b->last += 4;
		*((uint16_t *) b->last) = 0;
		b->last += 2;
		*((uint16_t *) b->last) = htons(header->qtype);
		b->last += 2;

		*((uint16_t *) b->last) = htons(lxl_dfss_group_id);
		b->last += 2;
		*((uint16_t *) b->last) = htons(lxl_dfss_idc_id);
		b->last += 2;
		*((uint32_t *) b->last) = lxl_dfss_ip_port.ip;
		b->last += 4;
		*((uint16_t *) b->last) = lxl_dfss_ip_port.port;
		b->last += 2;
		break;*/

	default:
		break;
	}

	return 0;
}

static void 
lxl_dfss_tracker_handler(lxl_event_t *ev)
{
	lxl_connection_t 	 *c;
	lxl_dfss_request_t 	 *r;
	lxl_dfss_upstream_t  *u;

	c = ev->data;
	r = c->data;
	u = r->tracker;
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss tracker request");
	
	if (ev->write) {
		u->write_event_handler(r, u);
	} else {
		u->read_event_handler(r, u);
	}

	//posted_request;
}

static void 
lxl_dfss_tracker_connect(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u)
{
	int 			   rc;
	lxl_connection_t  *c;
	
	rc = lxl_event_connect_peer(&u->peer);

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss tracker connect: %d", rc);

	if (rc == LXL_ERROR) {
		//lxl_dfss_tracker_finialize_request(r, u, )
		return;
	}

	if (rc == LXL_BUSY) {
		lxl_log_error(LXL_LOG_ERROR, 0, "dfss no live trackers");
		lxl_dfss_tracker_next(r, u, LXL_DFSS_UPSTREAM_FT_NOLIVE);
		return;
	}

	if (rc == LXL_DECLINED) {
		lxl_dfss_tracker_next(r, u, LXL_DFSS_UPSTREAM_FT_ERROR);
		return;
	}
		
	c = u->peer.connection;
	c->data = r;
	c->write->handler = lxl_dfss_tracker_handler;
	c->read->handler = lxl_dfss_tracker_handler;
	u->write_event_handler = lxl_dfss_tracker_send_request;
	u->read_event_handler = lxl_dfss_tracker_process_header;

	if (rc == LXL_EAGAIN) {
		lxl_add_timer(c->write, 3000);
		return;
	}
	
	lxl_dfss_tracker_send_request(r, u);
}

static void 
lxl_dfss_tracker_send_request(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u) 
{
	int 			   rc;
	ssize_t			   n;
	lxl_buf_t		  *b;
	lxl_connection_t  *c;

	// 处理timeout  or  send_request_handler
	c = u->peer.connection;

	if (c->write->timedout) {
		lxl_dfss_tracker_next(r, u, LXL_DFSS_UPSTREAM_FT_TIMEOUT);
		return;
	}

	b = &u->buffer;
	n = c->send(c, b->pos, b->last - b->pos);

#if 1
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "send tracker %ld", n);
#endif 
	
	if (n == LXL_ERROR) {
		lxl_dfss_tracker_next(r, u, LXL_DFSS_UPSTREAM_FT_ERROR);
		return;
	}

	if (n == LXL_EAGAIN) {
		goto again;
	}


	b->pos += n;
	if (b->pos != b->last) {
		goto again;
	}

	if (c->write->timer_set) {
		lxl_del_timer(c->write);
	}

	u->write_event_handler = lxl_dfss_tracker_dummy_handler;
	if (lxl_handle_write_event(c->write, 0) != 0) {
		//lxl_dfss_tracker_finalize_request(r, u, LXL_DFSS_INTERNAL_SERVER_ERROR);
		return;
	}

//	b->pos = b->start;
//	b->last = b->start;
	lxl_reset_buf(b);
	lxl_add_timer(c->read, 10000);	/* da yi xie */
	/*if (c->read->ready) {
		lxl_dfss_tracker_process_header(r, u);
		return;
	}*/

	return;

again:

	lxl_add_timer(c->write, 3000);

	if (lxl_handle_write_event(c->write, 0) != 0) {
		return;
	}

	return;
}

static void 
lxl_dfss_tracker_process_header(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u)
{
	size_t			   nread;
	ssize_t		       n;
	struct in_addr	   in;
	lxl_buf_t  		  *b;
	lxl_connection_t  *c;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss tracker process header");

	c = u->peer.connection;

	if (c->read->timedout) {
		lxl_dfss_tracker_next(r, u, LXL_DFSS_UPSTREAM_FT_TIMEOUT);
		return;
	}

	n = lxl_dfss_tracker_read_response_header(r, u);
	if (n == LXL_EAGAIN || n == LXL_ERROR) {
		return;
	}

	b = u->header_buf;
	nread = b->last - b->pos;
	if (nread >= sizeof(lxl_dfs_response_header_t)) {
		//lxl_dfss_tracker_parse_response(r, u, b);
		lxl_dfss_upstream_parse_response(r, u, b);

		// b->last = b->pos;
		//b->pos += 8;
		lxl_dfss_tracker_process_response(r, u);
	}

	return;

	in_addr_t addr;
	in_port_t port;
	addr = *((uint32_t *) (b->pos + 8));
	port = *((uint16_t *) (b->pos + 12));
	in.s_addr = addr;
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "addr: %s, port: %u", inet_ntoa(in), htons(port));
	
	char 							name[64];
	size_t							len;
	lxl_uint_t						i, nelts;
	socklen_t						socklen;
	struct sockaddr_in 			   	si;
	lxl_dfss_upstream_server_t	   *s;
	lxl_dfss_upstream_srv_conf_t   *uscf, **uscfp;
	lxl_dfss_upstream_main_conf_t  *umcf;

	uscf = NULL;
	umcf = lxl_dfss_get_module_main_conf(r, lxl_dfss_upstream_module);
	nelts = lxl_array_nelts(&umcf->upstreams);
	uscfp = lxl_array_elts(&umcf->upstreams);
	for (i = 0; i < nelts; ++i) {
		if (uscfp[i]->flags & LXL_DFSS_UPSTREAM_STORAGE) {
			uscf = uscfp[i];
			break;
		}
	}

	if (uscf == NULL) {
		lxl_log_error(LXL_LOG_ERROR, 0, "dfss no storage");
		// finalize
		return;
	}

	if (uscf->servers == NULL) {
		uscf->servers = lxl_array_create(r->pool, 9, sizeof(lxl_dfss_upstream_server_t));
		if (uscf->servers == NULL) {
			// finalize
			return;
		}
	}

	s = lxl_array_push(uscf->servers);
	if (s == NULL) {
		// finalize
		return;
	}
	
	len = (size_t) snprintf(name, 64, "%s:%d", inet_ntoa(in), ntohs(port));
	socklen = sizeof(struct sockaddr_in);
	si.sin_family = AF_INET;
	si.sin_port = port;
	si.sin_addr = in;
	memset(s, 0x00, sizeof(lxl_dfss_upstream_server_t));
	memcpy(s->sockaddr, &si, socklen);
	s->socklen = socklen;
	s->fail_timeout = 10;
	s->name = lxl_palloc(r->pool, len + 1);
	if (s->name == NULL) {
		// finalize
		return;
	}

	memcpy(s->name, name, len + 1);
	if (lxl_dfss_upstream_init_round_robin_storage(r, uscf) != 0) {
		// finalize
		return ;
	}

	lxl_dfss_storage_init(r);

	return;
}

static ssize_t
lxl_dfss_tracker_read_response_header(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u)
{
	ssize_t   		   n, rc;
	lxl_buf_t		  *b;
	lxl_event_t 	  *rev;
	lxl_connection_t  *c;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss tracker read response header");

	c = u->peer.connection;
	rev = c->read;
	//b = &u->buffer;
	b = u->header_buf;

	if (rev->ready) {
		n = c->recv(c, b->last, b->end - b->last);
	} else {
		n = LXL_EAGAIN;
	}

	switch (n) {
	case LXL_EAGAIN:
		rc = LXL_EAGAIN; 
		break;

	case 0:
		lxl_log_error(LXL_LOG_INFO, 0, "client prematurely closed connection");
	case LXL_ERROR:
		c->error = 1;
		// lxl_dfss_tracker_close_request
		rc = LXL_ERROR;
		break;

	default:
		b->last += n;
		rc = n;
	}

	return rc;
}

/*static int
lxl_dfss_tracker_parse_response(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u, char *data)
{
	u->response_header.body_n = ntohl(*((uint32_t *) data));
	u->response_header.flen = ntohl(*((uint16_t *) (data + 4)));
	u->response_header.rcode = ntohs(*((uint16_t *) (data + 6)));

	return 0;
}*/

static void 
lxl_dfss_tracker_process_response(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u)
{
	lxl_connection_t  		   *c;
	lxl_dfs_request_header_t   *reqh;
	lxl_dfs_response_header_t  *resh;
	
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss tracker process response");

	c = u->peer.connection;
	reqh = &u->request_header;
	resh = &u->response_header;

	if (c->read->timer_set) {
		lxl_del_timer(c->read);
	}

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "tracker response: %u, %hu, %hu", resh->body_n, resh->flen, resh->rcode);

	if (resh->rcode != LXL_DFS_OK) {
		lxl_dfss_tracker_finalize_request(r, u, resh->rcode);
		return;
	}

	switch (reqh->qtype) {
	case LXL_DFS_STORAGE_REPORT_FID:
		u->body_handler = lxl_dfss_tracker_report_fid_response_body;
		break;
	default :
		u->body_handler = lxl_dfss_tracker_dummy_response_body;
	}

	if (reqh->qtype == LXL_DFS_STORAGE_REPORT_STATE
		/*|| reqh->qtype == LXL_DFS_STORAGE_REPORT_FID*/) {
		lxl_dfss_tracker_finalize_request(r, u, LXL_DFS_OK);
		return;
	}

	/*if (reqh->qtype == LXL_DFS_STORAGE_SYNC_FID && resh->body_n == 0) {
		sync fid complet 
		lxl_dfss_tracker_finalize_request(r, u, LXL_DFS_OK);
		lxl_dfss_sync_pull();
		return;
	}*/

	/*if (resh->body_n == 0) {
		if (reqh->qtype == LXL_DFS_STORAGE_SYNC_FID) {
			// sync fid over
		} else {
			// lxl_dfss_tracker
		}

		return;
	}*/

	/*if (u->request_header.qtype != LXL_DFS_UPLOAD_STRONG 
		&& u->request_header.qtype != LXL_DFS_STORAGE_SYNC_PUSH) {
		// lxl_dfss_tracker_finalize();
		return;
	}*/

	//read body
	lxl_dfss_read_tracker_response_body(r, u);
}

static int
lxl_dfss_read_tracker_response_body(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u)
{
	int			rc;
	size_t      preread, rest, old_size;
	lxl_buf_t  *b;

	/*rest = u->response_header.body_n;
	if (rest == 0) {
		// wan
		return LXL_OK;
	}*/
	
	u->rest = u->response_header.body_n;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss tracker response body length %ld", u->rest);

	preread = u->header_buf->last - u->header_buf->pos;
	if (preread) {
		/* there is the pre-read part of the response body */
		
		lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss tracker response body preread %lu", preread);

		if (preread < u->rest) {
			u->rest -= preread;
		} else {
			u->rest = 0;
		}
	}

	if (u->rest == 0) {
		/*u->buffer.start = u->header_buf->pos;
		u->buffer.pos = u->header_buf->pos;
		u->buffer.last = u->header_buf->last;
		u->buffer.end = u->header_buf->end;*/
		u->buffer = *(u->header_buf);

		rc = u->body_handler(r, u);
		if (rc != LXL_OK) {
		}

		//u->post_handler(r);
		lxl_dfss_tracker_finalize_request(r, u, LXL_OK);

		return rc;
	}

	/*b = &u->buffer;
	old_size = b->end - b->start;
	if (old_size < u->rest) {
		b->start = lxl_palloc(r->pool, rest);
		if (b->start == NULL) {
			return -1;
		}

		b->pos = b->start;
		b->last = b->start;
		b->end = b->last + rest;
	}

	u->read_event_handler = lxl_dfss_read_tracker_response_body_handler;
	u->write_event_handler = lxl_dfss_tracker_dummy_handler;

	rc = lxl_dfss_do_read_tracker_response_body(r, u);*/

	return rc;
}

static void
lxl_dfss_read_tracker_response_body_handler(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u)
{
	int  rc;

	if (u->peer.connection->read->timedout) {
		// finalize
		return;
	}

	rc = lxl_dfss_do_read_tracker_response_body(r, u);
	if (rc > LXL_DFS_OK) {
		// finalize
	}
}

static int
lxl_dfss_do_read_tracker_response_body(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u)
{
	int				   rc;
   	size_t			   n, size;
	lxl_buf_t		  *b;
	lxl_connection_t  *c;
	//lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss do read tracker response body");

	rc = LXL_OK;
	c = u->peer.connection;
	size = b->end - b->last;
	n = c->recv(c, b->last, size);

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss tracker response body recv %ld", n);

	if (n == LXL_EAGAIN) {
		//break;
	}

	if (n == 0) {
		lxl_log_error(LXL_LOG_INFO, 0, "tracker prematurely close connection");
	}

	if (n == 0 || n == LXL_ERROR) {
		// c->error = 1
		//return LXL_DFS_BAD_RESPONSE;
		return LXL_DFSS_UPSTREAM_FT_ERROR;
	}

	b->last += n;
	if (n == size) {
		rc = lxl_dfss_tracker_sync_fid_response_body(r, u);
		if (rc == 0) {
			lxl_dfss_tracker_sync_fid_handle_response(r, u);
		}
	} else {
		lxl_add_timer(c->read, 60000);
	}

	return rc;
}

static int
lxl_dfss_tracker_report_fid_response_body(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u)
{
	size_t 				n;
	struct sockaddr_in  sin;
	lxl_uint_t  		i, j;
	lxl_buf_t  		   *b;
	//lxl_addr_t  	   *elt;
	lxl_dfss_addr_t    *elt;

	b = &u->buffer;
	n = b->last - b->pos;

	/* idc id must is sort */
	j = n / 8;	/* idc_id port ip */
	for (i = 0; i < j; ++i) {
		elt = lxl_queue_in(&r->addrs);
		if (elt == NULL) {
			return LXL_DFS_SERVER_ERROR;
		}

		sin.sin_family = AF_INET;
		sin.sin_port = *((uint16_t *) (b->pos + 2));
		sin.sin_addr.s_addr = *((uint32_t *) (b->pos + 4));


		elt->idc_id = ntohs(*((uint16_t *) (b->pos)));
		elt->addr.socklen = sizeof(struct sockaddr_in);
		elt->addr.sockaddr = lxl_palloc(r->pool, sizeof(struct sockaddr_in));
		if (elt->addr.sockaddr == NULL) {
			return LXL_DFS_SERVER_ERROR;
		}

		memcpy(elt->addr.sockaddr, &sin, elt->addr.socklen);
		snprintf(elt->addr.name, sizeof(elt->addr.name), "%s:%hu", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
#if 1
		lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "storage %hu %s", elt->idc_id, elt->addr.name);
#endif 

		b->pos += 8;
	}

	return LXL_OK;
}

static int 
lxl_dfss_tracker_sync_fid_response_body(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u)
{
	size_t			n;
	char 	   	    sep = '\0', *start;
	lxl_str_t  	   *fid;
	lxl_buf_t  	   *b;
	lxl_dfs_fid_t  *dfs_fid;

	b = &u->buffer;

	for (; ;) {
		if (b->pos == b->last) {
			break;
		}
		
		if (*(b->pos) == sep) {
			++(b->pos);
		}

		start = b->pos;
		do {
			++b->pos;
		} while (*(b->pos) != sep);

		n = b->pos - start;
		
		fid = lxl_hash_find(&lxl_dfss_fid_phash, start, n);
		if (fid) {
			lxl_log_error(LXL_LOG_INFO, 0, "storage sync fid %s exist", fid->data);
		} else {
			lxl_log_error(LXL_LOG_INFO, 0, "storage sync fid %s not exist", start);
			
		}

		dfs_fid = lxl_alloc(sizeof(lxl_dfs_fid_t));
		if (dfs_fid == NULL) {
			return -1;
		}

		dfs_fid->fid.data = lxl_alloc(n + 1);
		if (dfs_fid->fid.data == NULL) {
			return -1;
		}

		lxl_str_memcpy(&dfs_fid->fid, start, n);
		lxl_list_add_tail(&lxl_dfss_sync_pull_list, &dfs_fid->list);
	}
	
	return 0;
}

static int
lxl_dfss_tracker_sync_fid_handle_response(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u)
{
	lxl_connection_t  *c;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss tracker storage sync fid handle response");
	
	c = u->peer.connection;
	c->write->handler = lxl_dfss_tracker_handler;
	c->read->handler = lxl_dfss_tracker_handler;
	u->write_event_handler = lxl_dfss_tracker_send_request;
	u->read_event_handler = lxl_dfss_tracker_process_header;

	lxl_dfss_tracker_send_request(r, u);

	return 0;
}

static int
lxl_dfss_tracker_dummy_response_body(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u)
{
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss tracker dummy response body");

	return LXL_OK;
}

static void 
lxl_dfss_tracker_dummy_handler(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u)
{
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss tracker dummy handler");
	return;
}

static void
lxl_dfss_tracker_next(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u, lxl_uint_t ft_type)
{
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss next tracker, %lu", ft_type);

	lxl_dfss_tracker_finalize_request(r, u, ft_type);
	
	//lxl_dfss_tracker_connect(r, u);
}

static void
lxl_dfss_tracker_finalize_request(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u, lxl_int_t rc)
{
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss tracker finalize request: %s, %ld", r->rid, rc);

	if (u->peer.free && u->peer.sockaddr) {
		u->peer.free(&u->peer, u->peer.data, 0);
		u->peer.sockaddr = NULL;
	}

	if (u->peer.connection) {
		lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "close dfss tracker connection: %d", u->peer.connection->fd);
		
		/* peer.connection pool is null */
		lxl_close_connection(u->peer.connection);
	}

	if (rc == LXL_OK || rc == LXL_DFS_OK) {
		if (r->handler) {
			r->handler(r);
			return;
		}
	}

	// u->peer.connection = NULL;

	// LXL_DECLINED

	/*if (r->handler) {
		r->handler(r);
	}*/
	lxl_dfss_finalize_request(r, rc);
}

static void *
lxl_dfss_tracker_create_conf(lxl_conf_t *cf)
{
	lxl_dfss_tracker_srv_conf_t  *conf;
	
	conf = lxl_pcalloc(cf->pool , sizeof(lxl_dfss_tracker_srv_conf_t));
	if (conf == NULL) {
		return NULL;
	}

	conf->upstream.connect_timeout = LXL_CONF_UNSET_MSEC;
	conf->upstream.send_timeout = LXL_CONF_UNSET_MSEC;
	conf->upstream.read_timeout = LXL_CONF_UNSET_MSEC;
	conf->upstream.send_lowat = LXL_CONF_UNSET_SIZE;

	return conf;
}

static char *
lxl_dfss_tracker_init_conf(lxl_conf_t *cf, void *conf)
{
	lxl_dfss_tracker_srv_conf_t  *tscf = conf;

	lxl_conf_init_msec_value(tscf->upstream.connect_timeout, 6000);
	lxl_conf_init_msec_value(tscf->upstream.send_timeout, 10000);
	lxl_conf_init_msec_value(tscf->upstream.read_timeout, 10000);
	lxl_conf_init_size_value(tscf->upstream.send_lowat, 0);

	return LXL_CONF_OK;
}

static char *
lxl_dfss_tracker_pass(lxl_conf_t *cf, lxl_command_t *cmd, void *conf)
{
	lxl_dfss_tracker_srv_conf_t  *tscf = conf;
	lxl_str_t 					 *value;
	lxl_url_t					  u;

	if (tscf->upstream.upstream) {
		return "is duplicate";
	}

	memset(&u, 0x00, sizeof(lxl_url_t));
	value = lxl_array_elts(cf->args);
	u.host = value[1];
	tscf->upstream.upstream = lxl_dfss_upstream_add(cf, &u, 0);
	if (tscf->upstream.upstream == NULL) {
		return LXL_CONF_ERROR;
	}

	return LXL_CONF_OK;
}

static char *
lxl_dfss_tracker_lowat_check(lxl_conf_t *cf, void *post, void *data)
{
	lxl_conf_log_error(LXL_LOG_WARN, cf, 0, "tracker_send_lowat is not suport, ignored");

	return LXL_CONF_OK;
}
