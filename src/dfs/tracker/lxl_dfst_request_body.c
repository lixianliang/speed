
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_dfst.h>


static void lxl_dfst_read_client_request_body_handler(lxl_dfst_request_t *r);
static int	lxl_dfst_do_read_client_request_body(lxl_dfst_request_t *r);

static int	lxl_dfst_upload_request_body(lxl_dfst_request_t *r);
static int	lxl_dfst_one_fid_request_body(lxl_dfst_request_t *r);
static int	lxl_dfst_storage_sync_fid_request_body(lxl_dfst_request_t *r);
static int	lxl_dfst_storage_report_fid_request_body(lxl_dfst_request_t *r);
static int	lxl_dfst_storage_report_state_request_body(lxl_dfst_request_t *r);


int
lxl_dfst_read_client_request_body(lxl_dfst_request_t *r, lxl_dfst_client_body_handler_pt post_handler)
{
	int						  rc;
	size_t					  size, preread;
	lxl_buf_t  			     *b, *buf;
	lxl_dfst_request_body_t  *rb;

	rb = lxl_pcalloc(r->pool, sizeof(lxl_dfst_request_body_t));
	if (rb == NULL) {
		return LXL_DFS_SERVER_ERROR;
	}

	r->body = rb;
	
	switch (r->request_header.qtype) {
		case LXL_DFS_UPLOAD:
		case LXL_DFS_UPLOAD_SC:
			 rb->body_handler = lxl_dfst_upload_request_body;
			 break;
		case LXL_DFS_DOWNLOAD:
		case LXL_DFS_DELETE:
			rb->body_handler = lxl_dfst_one_fid_request_body;
			break;

		/*case LXL_DFS_STORAGE_SYNC_FID:
			rb->body_handler = lxl_dfst_storage_sync_fid_request_body;
			break;*/

		case LXL_DFS_STORAGE_REPORT_STATE:
			rb->body_handler = lxl_dfst_storage_report_state_request_body;
			break;

		case LXL_DFS_STORAGE_REPORT_FID:
			//lxl_slist_init(&r->u.fid_info.fid_slist);
			rb->body_handler = lxl_dfst_storage_report_fid_request_body;
			break;

		default:
			rb->body_handler = lxl_dfst_storage_report_state_request_body;
			break;
	}

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst request body length %ld", r->request_header.body_n);

	if (r->request_header.body_n == 0) {
		return LXL_DFS_BAD_REQUEST;
	}

	rb->post_handler = post_handler;
	rb->rest = r->request_header.body_n;	/* set rb->rest */
	buf = r->header_buf;

	preread = buf->last - buf->pos;
	if (preread) {
		/* there is the pre-read part of the request body */

		lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst client request body preread %lu", preread);

		if (preread < rb->rest) {
			rb->rest -= preread;
		} else {
			rb->rest = 0;
		}

		/* is very small */
		if (rb->rest > 0 && rb->rest < (buf->end - buf->last)) {
			/* the whole request body may be placed in r->header_buf */

			b = lxl_alloc_buf(r->pool);
			if (b == NULL) {
				return LXL_DFS_SERVER_ERROR;
			}

			b->start = buf->pos;
			b->pos = buf->pos;
			b->last = buf->last;
			b->end = buf->end;

			r->read_event_handler = lxl_dfst_read_client_request_body_handler;
			r->write_event_handler = lxl_dfst_request_empty_handler;
			rb->buf = b;
			//rc = LXL_OK;
			
			return LXL_OK;
		}
	}

	if (rb->rest == 0) {
		/* the whole request body was pre-read */

		rb->buf = buf;
		rc = rb->body_handler(r);
		if (rc != LXL_OK) {
			return rc;
		}

		rb->post_handler(r);

		return LXL_OK;	/* rc */
	}

	size = 8192;
	size += size >> 2;

	if (r->request_header.body_n < size) {
		size =  r->request_header.body_n;
	} else {
		size = 8192;
	}

	rb->buf = lxl_create_temp_buf(r->pool, size);
	if (rb->buf == NULL) {
		return LXL_DFS_SERVER_ERROR;
	}

	if (preread) {
		memcpy(rb->buf->pos, buf->pos, preread);
		rb->buf->pos += preread;
	}

	r->read_event_handler = lxl_dfst_read_client_request_body_handler;
	r->write_event_handler = lxl_dfst_request_empty_handler;

	rc = lxl_dfst_do_read_client_request_body(r);

	/*if (rc == LXL_EAGAIN) {
		return LXL_OK;
	} else {
		return rc;
	}*/

//	return (rc == LXL_EAGAIN ? LXL_OK : rc);
	return rc;
}

static void
lxl_dfst_read_client_request_body_handler(lxl_dfst_request_t *r)
{
	int  rc;

	if (r->connection->read->timedout) {
		r->connection->timedout = 1;
		lxl_dfst_finalize_request(r, LXL_DFS_REQUEST_TIMEOUT);
		return;
	}

	rc = lxl_dfst_do_read_client_request_body(r);
	if (rc > LXL_DFS_OK) {
		lxl_dfst_finalize_request(r, rc);
	}
}

static int
lxl_dfst_do_read_client_request_body(lxl_dfst_request_t *r)
{
	int		rc;
	off_t   rest;
	size_t  size;
	ssize_t n;
	lxl_buf_t *b;
	lxl_connection_t  *c;
	lxl_dfst_request_body_t  *rb;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst do read client request body");

	c = r->connection;
	rb = r->body;
	b = rb->buf;

	for (; ;) {
		for (; ;) {
			if (b->last == b->end) {
				rc = rb->body_handler(r);

				if (rc != LXL_OK) {
					return rc;
				}
			}

			size = b->end - b->last;
			rest = rb->rest - size;
			if ((off_t) size > rest) {
				size = rest;
			}

			n = c->recv(c, b->last, size);

			lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst client request body %ld", n);

			if (n == LXL_EAGAIN) {
				break;
			}

			if (n == 0) {
				lxl_log_error(LXL_LOG_INFO, 0, "client prematurely closed connection");
			}

			if (n == 0 || n == LXL_ERROR) {
				c->error = 1;
				return LXL_DFS_BAD_REQUEST;
			}

			b->last += n;

			if (n == rest) {
				rb->rest -= (b->last - b->pos);
			}

			if (rb->rest == 0) {
				break;
			}

			if (b->last < b->end) {
				break;
			}
		}
	
		lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst client request body rest %ld", rb->rest);

		if (rb->rest == 0) {
			break;
		}

		if (!c->read->ready) {
			lxl_add_timer(c->read, 60000);
			if (lxl_handle_read_event(c->read) != 0) {
				return LXL_DFS_SERVER_ERROR;
			}

			return LXL_EAGAIN;
		}
	}

	if (c->read->timer_set) {
		lxl_del_timer(c->read);
	}

	if (rb->body_handler(r) == LXL_ERROR) {
		return LXL_DFS_SERVER_ERROR;
	}

	r->read_event_handler = lxl_dfst_block_reading;
	rb->post_handler(r);

	return LXL_OK;
}

static int
lxl_dfst_upload_request_body(lxl_dfst_request_t *r)
{
	lxl_buf_t				 *b;
	lxl_dfs_idc_t			 *idc;
	lxl_dfst_request_body_t  *rb;

	rb = r->body;
	b = rb->buf;
	idc = &r->u.idc;
	 
	idc->idc_id =  ntohs(*((uint16_t *) b->pos));

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst upload: %hu", idc->idc_id);

	return LXL_OK;
}

static int
lxl_dfst_one_fid_request_body(lxl_dfst_request_t *r)
{
	size_t				 	  n;
	lxl_str_t				 *fid;
	lxl_dfst_request_body_t  *rb;

	rb = r->body;
	fid = &r->u.fid;
	n = rb->buf->last - rb->buf->pos;


	fid->data = lxl_palloc(r->pool, n + 1);
	if (fid->data == NULL) {
		return LXL_ERROR;
	}

	fid->len = n;
	memcpy(fid->data, rb->buf->pos, n);
	fid->data[n] = '\0';

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst one fid: %s", fid->data);

	return LXL_OK;
}

static int
lxl_dfst_storage_sync_fid_request_body(lxl_dfst_request_t *r)
{
	size_t					  n;
	struct in_addr			  addr;
	lxl_buf_t				 *b;
	lxl_dfs_storage_t	     *storage;
	lxl_dfst_request_body_t  *rb;

	rb = r->body;
	b = rb->buf;
	storage = &r->u.storage;

	storage->idc_id = ntohs(*((uint16_t *) b->pos));
	b->pos += 2;
	storage->ip = *((uint32_t *) b->pos);
	b->pos += 4;
	storage->port = *((uint16_t *) b->pos);

	addr.s_addr = storage->ip;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst storage sync fid: %hu, %hu, %s:%hu", 
					storage->idc_id, storage->idc_id, inet_ntoa(addr), ntohs(storage->port));

	return LXL_OK;
}

static int
lxl_dfst_storage_report_state_request_body(lxl_dfst_request_t *r)
{
	struct in_addr			  addr;
	lxl_buf_t				 *b;
	lxl_dfs_storage_state_t  *sstate;
	lxl_dfst_request_body_t  *rb;
	
	rb = r->body;
	b = rb->buf;
	sstate = &r->u.storage_state;

	sstate->idc_id = ntohs(*((uint16_t *) b->pos));
	b->pos += 2;

	/*sstate->storage_id = ntohs(*((uint16_t *) b->pos));
	b->pos += 2;*/
	sstate->port = *((uint16_t *) b->pos);
	b->pos += 2;
	sstate->ip = *((uint32_t *) b->pos);
	b->pos += 4;

	sstate->loadavg = ntohs(*(uint16_t *) b->pos);
	b->pos += 2;
	sstate->cpu_idle = ntohs(*((uint16_t *) b->pos));
	b->pos += 2;
	sstate->disk_free_mb = ntohl(*((uint32_t *) b->pos));

	addr.s_addr = sstate->ip;

	lxl_log_debug(LXL_LOG_DEBUG, 0, "dfst report state: %s, %hu, %s:%hu, %hu, %hu, %u",
					r->rid, sstate->idc_id, inet_ntoa(addr), ntohs(sstate->port), 
					sstate->loadavg, sstate->cpu_idle, sstate->disk_free_mb);
	
	return LXL_OK;
}

static int
lxl_dfst_storage_report_fid_request_body(lxl_dfst_request_t *r)
{
	char 					  sep = '\0', *start, *buf;
	size_t  				  n;
	struct in_addr 			  addr;
	lxl_buf_t 				 *b;
	lxl_dfs_fid_t			 *dfs_fid;
	//lxl_dfs_fid_info_t		 *info;
	lxl_dfst_request_body_t  *rb;

	rb = r->body;
	b = rb->buf;
	dfs_fid = &r->u.dfs_fid;

//	info->idc_id = ntohs(*((uint16_t *) b->pos));
//	b->pos += 2;
	dfs_fid->idc_id = ntohs(*((uint16_t *) b->pos));
	b->pos += 2;
	dfs_fid->port = *((uint16_t *) b->pos);
	b->pos += 2;
	dfs_fid->ip = *((uint32_t *) b->pos);
	b->pos += 4;

	n = b->last - b->pos;
	dfs_info->fid.data = lxl_palloc(r->pool, n + 1);
	if (dfs->fid.data == NULL) {
		return LXL_ERROR;
	}

	lxl_str_memcpy(&dfs_fid->fid, b->pos, n);

//	b->pos = b->start;
//	b->last = b->start;

	addr.s_addr = dfs_fid->ip;

	lxl_log_debug(LXL_LOG_DEBUG, 0, "dfst report fid: %s, %hu, %hu, %s:%hu, %s",
					r->rid, dfs_fid->idc_id, dfs_fid->idc_id, inet_ntoa(addr), ntohs(dfs_fid->port), dfs_fid->fid.data);
	
	return LXL_OK;
}
