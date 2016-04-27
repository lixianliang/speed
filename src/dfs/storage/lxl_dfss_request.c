
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_dfss.h>
#include <lxl_core.h>>
//#include <lxl_dfss_request.h>


//static lxl_dfss_request_t *lxl_dfss_create_request(lxl_connection_t *c);

static void 	lxl_dfss_wait_request_handler(lxl_event_t *rev);
static void 	lxl_dfss_empty_handler(lxl_event_t *ev);
static void 	lxl_dfss_process_request_header(lxl_event_t *rev);

static ssize_t 	lxl_dfss_read_request_header(lxl_dfss_request_t *r);
static void 	lxl_dfss_process_request(lxl_dfss_request_t *r);

static void		lxl_dfss_request_handler(lxl_event_t *ev);

static void     lxl_dfss_report_state_done_handle_request(lxl_dfss_request_t *r);
static void		lxl_dfss_upload_done_handle_request(lxl_dfss_request_t *r);
static void		lxl_dfss_upload_sync_handle_request(lxl_dfss_request_t *r);
static void		lxl_dfss_upload_sync_done_handle_request(lxl_dfss_request_t *r);
static void		lxl_dfss_upload_sc_sync_handle_request(lxl_dfss_request_t *r);
static void		lxl_dfss_upload_sc_sync_done_handle_request(lxl_dfss_request_t *r);
//static void		lxl_dfss_sync_handle_request(lxl_dfss_request_t *r);
//static void		lxl_dfss_sync_done_handle_request(lxl_dfss_request_t *r);
static void 	lxl_dfss_download_handle_request(lxl_dfss_request_t *r);
static void 	lxl_dfss_download_done_handle_request(lxl_dfss_request_t *r);
//static void		lxl_dfss_sync_push_handle_request(lxl_dfss_request_t *r);
static void		lxl_dfss_sync_push_done_handle_request(lxl_dfss_request_t *r);

static void 	lxl_dfss_send_header(lxl_dfss_request_t *r);
static void 	lxl_dfss_writer(lxl_dfss_request_t *r);
//static int		lxl_dfss_send_body(lxl_dfss_request_t *r);
// filename open_file create_temp_buf file_read
// lxl_dfs_send_header
// lxl_dfs_send_body
static int		lxl_dfss_send_special_response(lxl_dfss_request_t *r, lxl_int_t rc);
static void 	lxl_dfss_terminate_request(lxl_dfss_request_t *r, lxl_int_t rc);

static void 	lxl_dfss_close_request(lxl_dfss_request_t *r, lxl_int_t rc);
static void 	lxl_dfss_free_request(lxl_dfss_request_t *r, lxl_int_t rc);
static void 	lxl_dfss_close_connection(lxl_connection_t *c);


void    
lxl_dfss_init_connection(lxl_connection_t *c)
{
	lxl_event_t  *rev;

	rev = c->read;
	rev->handler = lxl_dfss_wait_request_handler;
	c->write->handler = lxl_dfss_empty_handler;
	lxl_add_timer(rev, 10000);
	if (lxl_handle_read_event(rev) != 0) {
		lxl_dfss_close_connection(c);
	}

	return ;
}

static void 
lxl_dfss_wait_request_handler(lxl_event_t *rev) 
{
	size_t 				 size;
	ssize_t 			 n;
	lxl_buf_t 			*b;
	lxl_connection_t    *c;
	lxl_dfss_port_t	    *port;
	lxl_dfss_request_t  *r;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss wait request handler");
	
	c = rev->data;
	port = c->listening->servers;

	if (rev->timedout) {
		lxl_log_error(LXL_LOG_ERROR, 0, "client timed out");
		lxl_dfss_close_connection(c);
		return;
	}

	c->data = r = lxl_dfss_create_request(c);
	if (r == NULL) {
		lxl_dfss_close_connection(c);
		return;
	}

	r->main_conf = port->addr.ctx->main_conf;
	r->srv_conf = port->addr.ctx->srv_conf;
	
	/* header */
	//size = sizeof(lxl_dfs_request_header_t);
	size = 64;
	r->header_buf = b = lxl_create_temp_buf(r->pool, size);
	if (b == NULL) {
		lxl_dfss_close_request(r, LXL_DFS_SERVER_ERROR);
		return;
	}
	
	n = c->recv(c, b->last, size);
	if (n == LXL_EAGAIN) {
		/*if (!rev->timer_set) {
			lxl_add_timer(rev, 10 * 1000);
		}*/
	
		rev->handler = lxl_dfss_process_request_header;
		if (lxl_handle_read_event(rev) != 0) {
			lxl_dfss_close_request(r, LXL_DFS_SERVER_ERROR);
			return;
		}
	}

	if (n == LXL_ERROR) {
		lxl_dfss_close_connection(c);
		return;
	}

	if (n == 0) {
		lxl_log_error(LXL_LOG_INFO, 0, "client closed connection");
		lxl_dfss_close_connection(c);
		return;
	}

	b->last += n;

	rev->handler = lxl_dfss_process_request_header;
	lxl_dfss_process_request_header(rev);

	return;
}

void
lxl_dfss_report_state_handler(lxl_event_t *ev)
{
	lxl_connection_t  	*c;
	lxl_dfss_port_t 	*port;
	lxl_dfss_request_t  *r;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss report state handler");

    lxl_dfss_update_system_state();
	
	c = ev->data;
	port = c->listening->servers;

	c->data = r = lxl_dfss_create_request(c);
	if (r == NULL) {
		/* not dfss colse connection */
		return;
	}

	r->main_conf = port->addr.ctx->main_conf;
	r->srv_conf = port->addr.ctx->srv_conf;
	r->request_header.qtype = LXL_DFS_STORAGE_REPORT_STATE;
	r->handler = lxl_dfss_report_state_done_handle_request;

	lxl_dfss_tracker_init(r);
}

void
lxl_dfss_report_fid_handler(lxl_event_t *ev)
{
	lxl_connection_t    *c;
	lxl_dfss_port_t     *port;
	lxl_dfss_request_t  *r;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss report fid handler");
	
	c = ev->data;
	port = c->listening->servers;
	
	c->data = r = lxl_dfss_create_request(c);
	if (r == NULL) {
		/* not dfss close connection */
		return;
	}

	r->main_conf = port->addr.ctx->main_conf;
	r->srv_conf = port->addr.ctx->srv_conf;
	r->request_header.qtype = LXL_DFS_STORAGE_REPORT_FID;

	lxl_dfss_tracker_init(r);
}

void
lxl_dfss_sync_fid_handler(lxl_event_t *ev)
{
	lxl_connection_t	*c;
	lxl_dfss_port_t     *port;
	lxl_dfss_request_t  *r;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss sync fid handler");

	/* c = ev->data;
	port = c->listening->servers;

	c->data = r = lxl_dfss_create_request(c);
	if (r == NULL) {
		return;
	}

	r->main_conf = port->addr.ctx->main_conf;
	r->srv_conf = port->addr.ctx->srv_conf;
	r->request_header.qtype = LXL_DFS_STORAGE_SYNC_FID;

	lxl_dfss_tracker_init(r);*/
}

void
lxl_dfss_sync_push_handler(lxl_event_t *ev)
{
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss sync push handler");
}

void 
lxl_dfss_sync_pull_handler(lxl_event_t *ev)
{
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss sync pull handler");
}

static void
lxl_dfss_empty_handler(lxl_event_t *ev) 
{
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss empty handler");
	
	return;
}

void 
lxl_dfss_request_empty_handler(lxl_dfss_request_t *r)
{
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss request empty handler");
	
	return;
}

lxl_dfss_request_t *
lxl_dfss_create_request(lxl_connection_t *c) 
{
	lxl_pool_t 			*pool;
	lxl_dfss_request_t  *r;

	pool = lxl_create_pool(512);
	if (pool == NULL) {
		return NULL;
	}
	
	r = lxl_pcalloc(pool, sizeof(lxl_dfss_request_t));
	if (r == NULL) {
		lxl_destroy_pool(pool);
		return NULL;
	}

	/* 
 	* pcalloc set
 	* first_qtype = 0
 	* new_request = 0
	* r->response_header {0, 0, 0}
 	*/
	r->pool = pool;
	r->connection = c;
	r->response_header.rcode = LXL_DFS_OK;

	/* 2*N N = 2 or N = 3 */
	if (lxl_queue_init(&r->addrs, r->pool, 8, sizeof(lxl_dfss_addr_t)) == -1) {
		return NULL;
	}

	return r;
}

static void 
lxl_dfss_process_request_header(lxl_event_t *rev) 
{
	//int rc;
	size_t				 nread;
	ssize_t 			 n;
	lxl_buf_t 			*b;
	lxl_connection_t 	*c;
	lxl_dfss_request_t  *r;
	
	c = rev->data;
	r = c->data;
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss process request header");

	if (rev->timedout) {
		lxl_log_error(LXL_LOG_ERROR, 0, "client timed out");
		c->timedout = 1;
		lxl_dfss_close_request(r, LXL_DFS_REQUEST_TIMEOUT);
		return;
	}

	b = r->header_buf;
	nread = b->last - b->pos;
	//lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss read byte:%lu", b->last - b->pos);
	if (nread < sizeof(lxl_dfs_request_header_t)) {
		n = lxl_dfss_read_request_header(r);
		if (n == LXL_EAGAIN || n == LXL_ERROR) {
			return;
		}

		nread = b->last - b->pos;
	}

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss read byte:%lu", nread);
	if (nread >= sizeof(lxl_dfs_request_header_t)) {
		if (lxl_dfss_parse_request(r, b->pos) == -1) {
			lxl_dfss_finalize_request(r, r->response_header.rcode);
			return;
		}

		lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss process request: %s, %u, %hu, 0x%04x", 
					r->rid, r->request_header.body_n, r->request_header.flen, r->request_header.qtype);
		// b->pos = b->start;
		// b->last = b->start;
		//b->last = b->pos;
		b->pos += 8;
		lxl_dfss_process_request(r);
	}
}

static ssize_t 
lxl_dfss_read_request_header(lxl_dfss_request_t *r) 
{
	ssize_t 		   n, rc;
	lxl_event_t 	  *rev;
	lxl_connection_t  *c;

	c = r->connection;
	rev = c->read;

	if (rev->ready) {
		n = c->recv(c, r->header_buf->last, r->header_buf->end - r->header_buf->last);
	} else {
		n = LXL_EAGAIN;
	} 

	switch (n) {
	case LXL_EAGAIN:
		rc = LXL_EAGAIN;
		//  lxl_add_timer;
		if (lxl_handle_read_event(rev) != 0) {
			lxl_dfss_close_request(r, LXL_DFS_SERVER_ERROR);
		}
		break;

	case 0:
		lxl_log_error(LXL_LOG_INFO, 0, "client prematurely closed connection");
	case LXL_ERROR:
		// base request
		c->error = 1;
		//lxl_dfss_finalize_request(r, LXL_ERROR);
		lxl_dfss_close_request(r, 0);
		rc = LXL_ERROR;
		break;

	default:
		r->header_buf->last += n;
		rc = n;
	}

	return rc;
} 

static void
lxl_dfss_process_request(lxl_dfss_request_t *r) 
{
	lxl_int_t		   rc;
	lxl_connection_t  *c;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss process request");

	c = r->connection;
	if (c->read->timer_set) {
		lxl_del_timer(c->read);
	}
	
	c->read->handler = lxl_dfss_request_handler;
	c->write->handler = lxl_dfss_request_handler;
	r->read_event_handler = lxl_dfss_block_reading;

	//r->read_event_handler =  block_reading
	switch (r->request_header.qtype) {
	case LXL_DFS_UPLOAD:
		r->handler = lxl_dfss_upload_done_handle_request;
		rc = lxl_dfss_read_client_request_body(r, lxl_dfss_tracker_init);

		break;

	case LXL_DFS_UPLOAD_SC:
		r->handler  = lxl_dfss_upload_sc_sync_handle_request;
		rc = lxl_dfss_read_client_request_body(r, lxl_dfss_tracker_init);
	
		break;
	
	case LXL_DFS_STORAGE_SYNC_PUSH:
		rc = lxl_dfss_read_client_request_body(r, lxl_dfss_sync_push_done_handle_request);

		break;

	case LXL_DFS_DOWNLOAD:
		rc = lxl_dfss_read_client_request_body(r, lxl_dfss_download_handle_request);

		break;

	case LXL_DFS_DELETE:
		rc = LXL_DFS_NOT_IMPLEMENTED;
		break;

	default:
		lxl_log_error(LXL_LOG_ERROR, 0, "dfss Unknow request qtype: %04X", r->request_header.qtype);
		break;
	} 

	if (rc > LXL_DFS_OK) {
		lxl_dfss_finalize_request(r, rc);
	}
}

static void		
lxl_dfss_request_handler(lxl_event_t *ev)
{
	lxl_connection_t *c;
	lxl_dfss_request_t *r;
	
	c = ev->data;
	r = c->data;
	
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss run request %s", r->rid);

	if (ev->write) {
		r->write_event_handler(r);
	} else {
		r->read_event_handler(r);
	}

	//lxl_dfss_run_posted_request(c);
}

/*static void
lxl_dfss_run_posted_request(lxl_connection_t *) 
{
}*/

static void
lxl_dfss_report_state_done_handle_request(lxl_dfss_request_t *r)
{
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss report stat done");

	lxl_add_timer(&lxl_dfss_report_state_event, 300*1000);
	//lxl_dfss_finalize_request(r, LXL_OK);

	lxl_dfss_free_request(r, LXL_OK);
}

static void		
lxl_dfss_upload_done_handle_request(lxl_dfss_request_t *r)
{
	size_t 					  len, size;
	ssize_t 				  n;
	lxl_str_t				 *fid, *name;
	lxl_buf_t 				 *b;
	lxl_file_t 				 *file;
	lxl_connection_t  		 *c;
	lxl_dfss_fid_value_t	 *value;

	c = r->connection;
	fid = &r->body->fid;
	file = &r->body->file;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss upload %s => %s", fid->data, file->name.data);

	value = lxl_alloc(sizeof(lxl_dfss_fid_value_t));
	if (value == NULL) {
		lxl_dfss_finalize_request(r, LXL_DFS_SERVER_ERROR);
		return;
	}
	
	value->size = r->request_header.body_n;

	value->name.data = lxl_alloc(file->name.len + 1);
	if (value->name.data == NULL) {
		lxl_dfss_finalize_request(r, LXL_DFS_SERVER_ERROR);
		return;
	}

	lxl_str_memcpy(&value->name, file->name.data, file->name.len);

	if (lxl_hash1_add(&lxl_dfss_fid_hash, fid->data, fid->len, value) == -1) {
		lxl_dfss_finalize_request(r, LXL_DFS_SERVER_ERROR);
		return;
	}
	
	b = r->out = r->header_buf;
	lxl_reset_buf(b);

	r->response_header.body_n = fid->len;
	*((uint32_t *) b->last) = htonl(r->response_header.body_n);
	b->last += 4;
	*((uint16_t *) b->last) = htons(r->response_header.flen);
	b->last += 2;
	*((uint16_t *) b->last) = htons(r->response_header.rcode);
	b->last += 2;
	memcpy(b->last, fid->data, fid->len);
	b->last += fid->len;

	/* tracker */
	/*if (lxl_dfss_upstream_tracker_handler(r, lxl_dfss_sync_handle_request) != 0) {
		
	}*/
	
	n = c->send(c, b->pos, b->last - b->pos);
	if (n == -1) {
		//lxl_dfss_finalize
		lxl_dfss_finalize_request(r, LXL_DFS_SERVER_ERROR);
		return;
	}

	b->pos += n;
	if (b->pos < b->last) {
		c->read->handler = lxl_dfss_request_handler;
		c->write->handler = lxl_dfss_request_handler;
		r->read_event_handler = lxl_dfss_request_empty_handler;
		r->write_event_handler = lxl_dfss_writer;
		lxl_add_timer(c->write, 10 * 1000);
		if (lxl_handle_write_event(c->write, 0) != 0) {
			lxl_dfss_finalize_request(r, LXL_DFS_SERVER_ERROR);
			return;
		}
	}
	
	r->request_closed_connection = 1;
	lxl_dfss_close_connection(c);

	lxl_dfss_upload_sync_handle_request(r);

#if 0
	//lxl_dfss_process_request_header(rev);
#endif
}

static void
lxl_dfss_upload_sync_handle_request(lxl_dfss_request_t *r)
{
	lxl_str_t			*fid;
	lxl_file_t			*file;

	fid = &r->body->fid;
	
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss upload sync push %s", fid->data);

	r->handler = lxl_dfss_upload_sync_done_handle_request;
	lxl_dfss_storage_init(r);
}

static void
lxl_dfss_upload_sync_done_handle_request(lxl_dfss_request_t *r)
{
	size_t	   			 n;
	lxl_str_t			*fid;
	lxl_buf_t			*b;
	lxl_connection_t	*c;

	fid = &r->body->fid;
	
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss upload sync push done %s", fid->data);

	lxl_dfss_finalize_request(r, LXL_OK);
}

static void
lxl_dfss_upload_sc_sync_handle_request(lxl_dfss_request_t *r)
{
	lxl_str_t			*fid;
	lxl_file_t			*file;

	fid = &r->body->fid;
	
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss upload sc sync push %s", fid->data);

	r->handler = lxl_dfss_upload_sc_sync_done_handle_request;
	lxl_dfss_storage_init(r);
}

static void
lxl_dfss_upload_sc_sync_done_handle_request(lxl_dfss_request_t *r)
{
	size_t	   			 n;
	lxl_str_t			*fid;
	lxl_buf_t			*b;
	lxl_connection_t	*c;

	fid = &r->body->fid;
	
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss upload sc sync push done %s", fid->data);

	c = r->connection;
	b = r->out = r->header_buf;
	lxl_reset_buf(b);

	r->response_header.body_n = fid->len;
	*((uint32_t *) b->last) = htonl(r->response_header.body_n);
	b->last += 4;
	*((uint16_t *) b->last) = htons(r->response_header.flen);
	b->last += 2;
	*((uint16_t *) b->last) = htons(r->response_header.rcode);
	b->last += 2;
	memcpy(b->last, fid->data, fid->len);
	b->last += fid->len;

	n = c->send(c, b->pos, b->last - b->pos);
	if (n == -1) {
		lxl_dfss_finalize_request(r, LXL_DFS_SERVER_ERROR);
		return;
	}

	b->pos += n;
	if (b->pos < b->last) {
		c->read->handler = lxl_dfss_request_handler;
		c->write->handler = lxl_dfss_request_handler;
		r->read_event_handler = lxl_dfss_request_empty_handler;
		r->write_event_handler = lxl_dfss_writer;
		lxl_add_timer(c->write, 10 * 1000);
		if (lxl_handle_write_event(c->write, 0) != 0) {
			lxl_dfss_finalize_request(r, LXL_DFS_SERVER_ERROR);
			return;
		}
	}

	lxl_dfss_finalize_request(r, LXL_OK);  
}

static void 	
lxl_dfss_download_handle_request(lxl_dfss_request_t *r)
{
	char 					   *p;
	size_t 						n;
	lxl_int_t 				    rc, body_n;
	lxl_str_t				   *fid;
	lxl_buf_t 				   *b;
	lxl_file_t 				   *file;
	lxl_event_t 			   *rev;
	lxl_connection_t 		   *c;
	lxl_dfs_response_header_t  *header;
	lxl_dfss_fid_value_t	   *value;
	//lxl_dfss_request_body_t    *rb;
	
	c = r->connection;
	if (r->complete) {
		r->complete = 0;
		rev = c->read;
		rev->handler = lxl_dfss_process_request_header;
		if (lxl_handle_read_event(rev) != 0) {
			// 
			return;
		}
	
		lxl_add_timer(rev, 10 * 1000);
		return;
	}

	//rb = r->body;
	fid = &r->body->fid;
	file = &r->body->file;

	value = lxl_hash1_find(&lxl_dfss_fid_hash, fid->data, fid->len);
	if (value == NULL) {
		rc = LXL_DFS_NOT_FIND_FID;
		//lxl_dfss_finalize_request(r, LXL_DFS_NOT_FIND_FID);
		//return;
		goto failed;
	}

	file->name.data = lxl_pnalloc(r->pool, value->name.len + 1);
	if (file->name.data == NULL) {
		rc = LXL_DFS_SERVER_ERROR;
		goto failed;
	}

	lxl_str_memcpy(&file->name, value->name.data, value->name.len);
	//file->info.st_size = value->size;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss download %s => %s, %lu", fid->data, file->name.data, file->info.st_size);

	file->fd = open(file->name.data, O_RDONLY);
	if (file->fd == -1) {
		lxl_log_error(LXL_LOG_ALERT, errno, "open(O_RDONLY) %s failed", file->name.data);
		rc = LXL_DFS_SERVER_ERROR;
		goto failed;
	}

	/* p = strrchr(file->name.data, '/');
	if (p == NULL) {
		return ;
	}

	hout->rcode = 1;
	p += 17;
	body_n = lxl_hextoi(p, 8);
	if (body_n == -1) {
		lxl_log_error(LXL_LOG_ERROR, 0, "dfss paring body_n lxl_hextoi(%s) failed", p);
		return;
	}

	hout->body_n = (uint32_t) body_n;
	hout->flen = 0;
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss download file body_n %ld", hout->body_n);*/
	// fen kai you li yu zheng kuai du or zheng kuai xie
	
	//header->body_n = file->info.st_size;
	b = r->out = r->header_buf;
	header = &r->response_header;
	header->body_n = value->size;
	b->pos = b->start;
	b->last = b->start;
	*((uint32_t *) b->last) = htonl(header->body_n);
	b->last += 4;
	*((uint16_t *) b->last) = htons(header->flen);
	b->last += 2;
	*((uint16_t *) b->last) = htons(header->rcode);
	b->last += 2;

	n = c->send(c, b->pos, b->last - b->pos);
	if (n == -1) {
		lxl_dfss_finalize_request(r, LXL_DFS_SERVER_ERROR);
		return;
	}

	b->pos += n;
	if (b->pos < b->last) {
		//c->read->handler
		c->write->handler = lxl_dfss_request_handler;
		//r->read_evnet_handle
		r->write_event_handler = lxl_dfss_writer;
		lxl_add_timer(c->write, 10 * 1000);
		if (lxl_handle_write_event(c->write, 0) != 0) {
			lxl_dfss_finalize_request(r, LXL_DFS_SERVER_ERROR);
			return;
		}
	}

//	b->pos = b->start;
//	b->last = b->start;
	rc = lxl_dfss_send_body(r, lxl_dfss_download_done_handle_request);
	if (rc > LXL_DFS_OK) {
		//
		return;
	}
	/*if (lxl_dfss_send_body(r, lxl_dfss_download_done_handle_request) > LXL_DFS_OK) {
		// lxl_finalize
		return;
	}*/

	return;

failed:

	lxl_dfss_finalize_request(r, rc);

	return;
}

static void 	
lxl_dfss_download_done_handle_request(lxl_dfss_request_t *r)
{
	lxl_connection_t *c;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss download done request");

	lxl_dfss_finalize_request(r, LXL_DFS_OK);

/*	c = r->connection;
	c->read->handler = lxl_dfss_process_request_header;
	lxl_add_timer(c->read, 10000);
	//c->write->handler = lxl_dfss_request_handler;
	//c->write_event_handler = lxl_dfss_request_empty_handler;
	if (lxl_handle_read_event(c->read) != 0) {
		lxl_dfss_finalize_request(r, LXL_DFS_SERVER_ERROR);
		return;
	}*/
}

/*static void
lxl_dfss_sync_push_start_handle_request(lxl_dfss_request_t *r)
{
	lxl_str_t			*fid;
	lxl_file_t			*file;

	fid = &r->body->fid;
	
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss sync push start %s", fid->data);

	r->sync = 1;
	r->handler = lxl_dfss_sync_push_handle_request;
	lxl_dfss_tracker_init(r);
}*/

static void		
lxl_dfss_sync_push_done_handle_request(lxl_dfss_request_t *r)
{
	size_t				   n;
	lxl_str_t			  *fid;
	lxl_buf_t			  *b;
	lxl_file_t 			  *file;
	lxl_connection_t	  *c;
	lxl_dfss_fid_value_t  *value;

	c = r->connection;
	fid = &r->body->fid;
	file = &r->body->file;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss sync push done %s => %s", fid->data, file->name.data);

	value = lxl_alloc(sizeof(lxl_dfss_fid_value_t));
	if (value == NULL) {
		lxl_dfss_finalize_request(r, LXL_DFS_SERVER_ERROR);
		return;
	}

	value->name.data = lxl_alloc(file->name.len + 1);
	if (value->name.data == NULL) {
		lxl_dfss_finalize_request(r, LXL_DFS_SERVER_ERROR);
		return;
	}

	lxl_str_memcpy(&value->name, file->name.data, file->name.len);

	value->size = r->request_header.body_n;

	if (lxl_hash1_add(&lxl_dfss_fid_hash, fid->data, fid->len, value) == -1) {
		lxl_dfss_finalize_request(r, LXL_DFS_SERVER_ERROR);
		return;
	}

	b = r->out = r->header_buf;
	lxl_reset_buf(b);
	
	*((uint32_t *) b->last) = htonl(r->response_header.body_n);
	 b->last += 4;  
	*((uint16_t *) b->last) = htons(r->response_header.flen);
	b->last += 2; 
	*((uint16_t *) b->last) = htons(r->response_header.rcode);
	b->last += 2;     

	n = c->send(c, b->pos, b->last - b->pos);
	if (n == -1) {
		lxl_dfss_finalize_request(r, LXL_DFS_SERVER_ERROR);
		return;
	}

	b->pos += n;
	if (b->pos < b->last) {
		c->read->handler = lxl_dfss_request_handler;
		c->write->handler = lxl_dfss_request_handler;
		r->read_event_handler = lxl_dfss_request_empty_handler;
		r->write_event_handler = lxl_dfss_writer;
		lxl_add_timer(c->write, 10 * 1000);
		if (lxl_handle_write_event(c->write, 0) != 0) {
			lxl_dfss_finalize_request(r, LXL_DFS_SERVER_ERROR);
			return;
		}
	}

	lxl_dfss_finalize_request(r, LXL_OK);
}

static void
lxl_dfss_send_header(lxl_dfss_request_t *r)
{
	lxl_buf_t  		  *b;
	lxl_event_t  	  *wev;
	lxl_connection_t  *c;

	c = r->connection;
	wev = c->write;
	b = r->out = r->header_buf;

	lxl_reset_buf(b);
//	b->pos = b->start;
//	b->last = b->start;

	*((uint32_t *) b->last) = htonl(r->response_header.body_n);
	b->last += 4;
	*((uint16_t *) b->last) = htons(r->response_header.flen);
	b->last += 2;
	*((uint16_t *) b->last) = htons(r->response_header.rcode);
	b->last += 2;
	
	r->write_event_handler = lxl_dfss_writer;

	lxl_dfss_writer(r);
}

static void 
lxl_dfss_writer(lxl_dfss_request_t *r)
{
	int				   rc;
	ssize_t			   n;
	lxl_buf_t		  *b;
	lxl_event_t  	  *wev;
	lxl_connection_t  *c;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss writer handler");

	c = r->connection;
	wev = c->write;
	b = r->out;

	if (wev->timedout) {
		// delayed
		lxl_log_error(LXL_LOG_INFO, 0, "client timed out");
		c->timedout = 1;
		lxl_dfss_finalize_request(r, LXL_DFS_REQUEST_TIMEOUT);
		return;
	}

	n = c->send(c, b->pos, b->last - b->pos);
	if (n == LXL_ERROR) {
		c->error = 1;
		lxl_dfss_close_request(r, 0);
		return;
	}

	if (n == LXL_EAGAIN) {
		goto done;
	}

	b->pos += n;
	if (b->pos < b->last) {
		goto done;
	}

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss writer done");

	lxl_dfss_finalize_request(r, LXL_OK);

	return;

done:

	if (!wev->timer_set) {
		lxl_add_timer(wev, 6 * 1000);
	}

	if (lxl_handle_write_event(wev, 0) != 0) {
		lxl_dfss_close_request(r, 0);
	}

	return;
}

void
lxl_dfss_block_reading(lxl_dfss_request_t *r)
{
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss request reading blocked %s", r->rid);
	
	return;
}

void		
lxl_dfss_finalize_request(lxl_dfss_request_t *r, lxl_int_t rc)
{
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss finalize request: %s, %ld", r->rid, rc);

	/*if (rc == LXL_DFS_OK) {
		lxl_dfss_close_request(r, 0);
		return;
	}*/
	
	if (rc == LXL_DFS_REQUEST_TIMEOUT || rc == LXL_DFS_CLIENT_CLOSE_REQUEST) {
		//lxl_dfss_terminate_request(r, rc);
		lxl_dfss_close_request(r, rc);
		return;
	}

	if (rc > LXL_DFS_OK) {
		lxl_dfss_finalize_request(r, lxl_dfss_send_special_response(r, rc));
		return;
	}

	// 这里面错，直接terminal_request
	//rc == LXL_DONE  ===> lxl_dfss_finalize_request return
	// reponse
	// write
	//r->response_header.rcode = rc;
	//lxl_dfss_send_header(r);
	//lxl_dfss_close_request(r);
/*	if (r->request_header.qtype == LXL_DFS_UPLOAD && rc == LXL_OK) {
		if (close(r->connection->fd) == -1) {
			lxl_log_error(LXL_LOG_ERROR, errno, "dfss close() %d failed", r->connection->fd);
		}

		lxl_dfss_upload_sync_handle_request(r);
		return;
	}*/

	lxl_dfss_close_request(r, rc);
}

static int
lxl_dfss_send_special_response(lxl_dfss_request_t *r, lxl_int_t rc)
{
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss send special response: %ld", rc);

	r->response_header.rcode = rc;
	lxl_dfss_send_header(r);

	return LXL_OK;
}

static void
lxl_dfss_terminate_request(lxl_dfss_request_t *r, lxl_int_t rc)
{
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss terminate request: %ld", rc);
}

/*void 
lxl_dfss_block_reading(lxl_dfss_request_t *r)
{
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss reading blocked");	
}*/

static void
lxl_dfss_close_request(lxl_dfss_request_t *r, lxl_int_t rc)
{
	unsigned 		   flags;
	lxl_connection_t  *c;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss close request");

	flags = r->request_closed_connection;
	c = r->connection;

	lxl_dfss_free_request(r, rc);

	if (!flags) {
		lxl_dfss_close_connection(c);
	}

	//return;
}

void
lxl_dfss_free_request(lxl_dfss_request_t *r, lxl_int_t rc)
{
	lxl_pool_t  *pool;

	/*if (r->pool == NULL) {
		lxl_log_error(LXL_LOG_ALERT, 0, "dfss request already closed");
		return;
	}*/

	pool = r->pool;
//	r->pool = NULL;
	lxl_destroy_pool(pool);
}

static void 
lxl_dfss_close_connection(lxl_connection_t *c) 
{
	lxl_pool_t  *pool;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss close connection");

	pool = c->pool;
	lxl_close_connection(c);	/* timer event */

	if (pool) {	/* udp */
		lxl_destroy_pool(pool);
	}
}
