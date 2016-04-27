
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_dfst.h>


#define LXL_DFST_REPORT_STATE_UDP_LEN	26


static lxl_dfst_request_t * lxl_dfst_create_request(lxl_connection_t *c);

static lxl_int_t			lxl_dfst_parse_udp_request(lxl_dfst_request_t *r, lxl_buf_t *b);

static void		lxl_dfst_storage_report_state_handle_udp_request(lxl_dfst_request_t *r);
static void		lxl_dfst_storage_report_fid_handle_udp_request(lxl_dfst_request_t *r);

static void		lxl_dfst_wait_request_handler(lxl_event_t *rev);
static void		lxl_dfst_empty_handler(lxl_event_t *ev);
static void		lxl_dfst_process_request_header(lxl_event_t *rev);

static ssize_t 	lxl_dfst_read_request_header(lxl_dfst_request_t *r);
static void 	lxl_dfst_process_request(lxl_dfst_request_t *r);

static void		lxl_dfst_request_handler(lxl_event_t *ev);

static void		lxl_dfst_upload_handle_request(lxl_dfst_request_t *r);
static void		lxl_dfst_download_handle_request(lxl_dfst_request_t *r);
static void		lxl_dfst_delete_handle_request(lxl_dfst_request_t *r);
static void 	lxl_dfst_storage_register_request(lxl_dfst_request_t *r);
static void		lxl_dfst_storage_report_state_handle_request(lxl_dfst_request_t *r);
static void		lxl_dfst_storage_report_fid_handle_request(lxl_dfst_request_t *r);
//static void		lxl_dfst_storage_sync_handle_request(lxl_dfst_request_t *r);
//static void	    lxl_dfst_storage_sync_fid_handle_request(lxl_dfst_request_t *r);
//static void		lxl_dfst_tracker_sync_fid_handle_request(lxl_dfst_request_t *r);
//static int		lxl_dfst_sync_fid(lxl_dfst_request_t *r, lxl_dfs_idc_info_t *info);

//static void		lxl_dfst_send_sync_fid_response(lxl_dfst_request_t *r);
static void		lxl_dfst_send_response(lxl_dfst_request_t *r);
static void		lxl_dfst_send_header(lxl_dfst_request_t *r);
static void     lxl_dfst_package_header(lxl_dfst_request_t *r);

static void		lxl_dfst_writer_udp(lxl_dfst_request_t *r);
static void 	lxl_dfst_writer(lxl_dfst_request_t *r);

//static void		lxl_dfst_set_keepalive(lxl_dfst_reqeust_t *r);
//static void		lxl_dfst_keepalive_handler(lxl_event_t *ev);
static void		lxl_dfst_close_request(lxl_dfst_request_t *r, lxl_int_t rc);
static void		lxl_dfst_free_request(lxl_dfst_request_t *r, lxl_int_t rc);
static void 	lxl_dfst_close_connection(lxl_connection_t *c);


void    
lxl_dfst_init_udp_connection(lxl_connection_t *c)
{
	size_t					   size = 64;
	lxl_int_t			 	   rc;
	lxl_buf_t				  *b;
	lxl_dfst_port_t	          *port;
	lxl_dfst_request_t  	  *r;
	lxl_dfs_request_header_t  *header;
	
	c->data = r = lxl_dfst_create_request(c);
	if (r == NULL) {
		lxl_dfst_close_connection(c);
		return;
	}

	port = c->listening->servers;
	r->main_conf = port->addr.ctx->main_conf;
	r->srv_conf = port->addr.ctx->srv_conf;

	r->header_buf = b = lxl_create_temp_buf(r->pool, size);
	if (b == NULL) {
		lxl_dfst_close_connection(c);
		return;
	}

	rc = lxl_dfst_parse_udp_request(r, c->buffer);
	if (rc != LXL_OK) {
		lxl_dfst_finalize_request(r, rc);
		return;
	}

	header = &r->request_header;

	lxl_log_error(LXL_LOG_INFO, 0, "dfst request: %s, 0x%08x, 0x%04x", r->rid, header->body_n, header->qtype);

	/*if (header->qtype == LXL_DFS_STORAGE_REPORT_STATE) {
		lxl_dfst_storage_report_state_handle_udp_request(r);
	} else if (header->qtype == LXL_DFS_STORAGE_REPORT_FID){
		lxl_dfst_storage_report_fid_handle_udp_request(r);
	} else if (header->qtype == LXL_DFS_DOWNLOAD) {
	} else if (header->qtype == LXL_DFS_DELETE) {
	} else if (STAT)*/

	switch (header->qtype) {
	case LXL_DFS_STORAGE_REPORT_STATE:
		lxl_dfst_storage_report_state_handle_udp_request(r);
		break;

	case LXL_DFS_STORAGE_REPORT_FID:
		lxl_dfst_storage_report_fid_handle_udp_request(r);
		break;

	case LXL_DFS_DOWNLOAD:
		
		break;
	
	case LXL_DFS_DELETE:
	
		break;

	default:	/* LXL_DFS_STAT */

		break;
	}
}

static lxl_int_t 
lxl_dfst_parse_udp_request(lxl_dfst_request_t *r, lxl_buf_t *b)
{
	size_t 					   n;
	lxl_str_t				  *fid;
	lxl_dfs_fid_info_t		  *finfo;
	lxl_dfs_storage_state_t   *sstate;
	lxl_dfs_request_header_t  *header;
	
	n = b->last - b->pos;

	//lxl_log_debug(LXL_LOG_DEBUG, 0, "dfst parse udp request %lu", n);

	snprintf(r->rid, sizeof(r->rid), "%08lx%08x", lxl_current_sec, lxl_dfst_request_seed);
	++lxl_dfst_request_seed;
	
	if (n < sizeof(lxl_dfs_request_header_t)) {
		return LXL_ERROR;
	}

	header = &r->request_header;
	header->body_n = ntohl(*((uint32_t *) b->pos));
	b->pos += 6;
	header->qtype = ntohs(*((uint16_t *) b->pos));
	b->pos += 2;

	r->response_header.body_n = header->body_n;

	/* 8 + 24 */
	if (n < LXL_DFS_MIN_UDP_LENGTH) {
		return LXL_DFS_BAD_REQUEST;
	}

	/*if (n >= 64) {
		return LXL_DFS_REQUEST_TOO_LONG;
	}*/

	if (header->qtype == LXL_DFS_STORAGE_REPORT_STATE) {
		/* body 16 */
		sstate = &r->u.storage_state;
		sstate->idc_id = ntohs(*((uint16_t *) b->pos));
		b->pos += 2;
		sstate->port = *((uint16_t *) b->pos);
		b->pos += 2;
		sstate->ip = *((uint32_t *) b->pos);
		b->pos + 4;
		sstate->loadavg = ntohs(*((uint16_t *) b->pos));
		b->pos += 2;
		sstate->cpu_idle = ntohs(*((uint16_t *) b->pos));   
		b->pos += 2;
		sstate->disk_free_mb = ntohl(*((uint32_t *) b->pos));
	} else if (header->qtype == LXL_DFS_STORAGE_REPORT_FID) {
		/* body 10 + fid */
		finfo = &r->u.fid_info;
		finfo->idc_id = ntohs(*((uint16_t *) b->pos));
		b->pos += 2;
		finfo->port = *((uint16_t *) b->pos);
		b->pos += 2;
		finfo->ip = *((uint32_t *) b->pos);
		b->pos += 4;
		
		n -= 16;
		finfo->fid.data = lxl_palloc(r->pool, n + 1);
		if (finfo->fid.data == NULL) {
			return LXL_DFS_SERVER_ERROR;
		}

		lxl_str_memcpy(&finfo->fid, b->pos, n);
	} else if (header->qtype == LXL_DFS_DOWNLOAD || header->qtype == LXL_DFS_DELETE /*|| header->qtype == LXL_DFS_STAT */) {
		/* body fid */
		fid = &r->u.fid;
		n -= 8;
		fid->data = lxl_palloc(r->pool, n + 1);
		if (fid->data == NULL) {
			return LXL_DFS_SERVER_ERROR;
		}

		lxl_str_memcpy(fid, b->pos, n);
	} else {
		return LXL_DFS_NOT_IMPLEMENTED;
	}
	
	return LXL_OK;
}

static void
lxl_dfst_storage_report_state_handle_udp_request(lxl_dfst_request_t *r)
{
//	lxl_int_t				  rc;
	lxl_dfst_idc_t 	   		 *idc;
	lxl_dfst_storage_t       *storage;
	lxl_dfs_ip_port_t         ip_port;
	lxl_dfs_storage_state_t  *sstate;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst storage report state handle udp request");

//	rc = LXL_OK;
	sstate = &r->u.storage_state;
	if (lxl_dfst_storage_add(sstate, LXL_DFST_BINLOG) == -1) {
		lxl_dfst_finalize_request(r, LXL_DFS_SERVER_ERROR);
		return;
	}
/*	ip_port.port = sstate->port;
	ip_port.ip = sstate->ip;

	idc = lxl_dfst_idc_find(info->idc_id);
	if (!idc) {
		idc = lxl_dfst_idc_add(info->idc_id);
		if (!idc) {
			rc = LXL_DFS_SERVER_ERROR;
			goto failed;
		}
	}

	storage = lxl_dfst_storage_find(idc, &ip_port);
	if (storage) {
		idc->sort = 0;
		storage->loadavg = info->loadavg;
		storage->cpu_idle = info->cpu_idle;
		//storage->cpu_usage = info.cpu_usage;
		storage->disk_free_mb = info->disk_free_mb;
	} else {
		storage = lxl_dfst_storage_add(idc, info);
		if (storage == NULL) {
			rc = LXL_DFS_SERVER_ERROR;
		 	goto failed;
		}
	}*/

	lxl_dfst_send_header(r);

	return;

//failed:
//
//	lxl_dfst_finalize_request(r, rc);
}

static void
lxl_dfst_storage_report_fid_handle_udp_request(lxl_dfst_request_t *r)
{
	lxl_int_t 			 rc;
	lxl_uint_t			 i, nelts;
	lxl_str_t			*fid;
	//lxl_slist_t			*slist;
	lxl_dfst_fid_t		**value, *dfst_fid;
	//lxl_dfs_fid_t		*dfs_fid;
	//lxl_dfs_ip_port_t   *ip_port;
	lxl_dfs_fid_t		*dfs_fid;
	lxl_dfs_storage_t   *storage;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst storage report fid handle udp request");

	rc = LXL_OK;
	info = &r->u.fid_info;

	//for (slist = lxl_slist_head(&info->fid_slist);
	//	slist != lxl_slist_sentinel(); slist = lxl_slist_next(slist)) {
		//dfs_fid = lxl_slist_data(slist, lxl_dfs_fid_t, slist);
		fid = &info->fid;
		value = (lxl_dfst_fid_t **) lxl_hash1_addfind(&lxl_dfst_fid_hash, fid->data, fid->len);
		if (value == NULL) {
			rc = LXL_DFS_SERVER_ERROR;
			goto failed;
		}
	
		if (*value == NULL) {
			dfst_fid = lxl_alloc(sizeof(lxl_dfst_fid_t));
			if (dfst_fid == NULL) {
				rc = LXL_DFS_SERVER_ERROR;
				goto failed;
			}

			*value = dfst_fid;
			if (lxl_array1_init(&dfst_fid->storages, 3, sizeof(lxl_dfs_storage_t)) == -1) {
				rc = LXL_DFS_SERVER_ERROR;
				goto failed;
			}

			//dfst_fid->idc_id = info->idc_id;
		} else {
			dfst_fid = *value;
		}

		nelts = lxl_array1_nelts(&dfst_fid->storages);
		for (i = 0; i < nelts; ++i) {
			storage = lxl_array1_data(&dfst_fid->storages, lxl_dfs_storage_t, i);
			if (storage->ip == info->ip && storage->port == info->port) {
				lxl_log_error(LXL_LOG_INFO, 0, "storage report ip:%u port:%hu fid:%s exist", info->ip, info->port, dfs_fid->fid.data);
				return;
			}
		}

		storage = lxl_array1_push(&dfst_fid->storages);
		if (storage == NULL) {
			rc = LXL_DFS_SERVER_ERROR;
			goto failed;
		}

		storage->idc_id = info->idc_id;
		storage->ip = info->ip;
		storage->port = info->port;
		
		lxl_log_error(LXL_LOG_INFO, 0, "stroage report ip:%u port:%hu fid:%s", info->ip, info->port, dfs_fid->fid.data);
	//}

	lxl_dfst_send_header(r);
	
	return;

failed:

	lxl_dfst_finalize_request(r, rc);
}


lxl_dfst_init_connection(lxl_connection_t *c)
{
	lxl_event_t	 *rev;
	
	rev = c->read;
	rev->handler = lxl_dfst_wait_request_handler;
	c->write->handler = lxl_dfst_empty_handler;
	lxl_add_timer(rev, 10000);
	if (lxl_handle_read_event(rev) !=  0) {
		lxl_dfst_close_connection(c);
	}

	return ;
}

static void
lxl_dfst_wait_request_handler(lxl_event_t *rev)
{
	size_t				 size;
	ssize_t				 n;
	lxl_buf_t		    *b;
	lxl_connection_t    *c;
	lxl_dfst_port_t	    *port;
	lxl_dfst_request_t  *r;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst wait request handler");

	c = rev->data;
	port = c->listening->servers;
	if (rev->timedout) {
		lxl_log_error(LXL_LOG_ERROR, 0, "client timed out");
		lxl_dfst_close_connection(c);
		return;
	}

	c->data = r = lxl_dfst_create_request(c);
	if (r == NULL) {
		lxl_dfst_close_connection(c);
		return;
	}

	r->main_conf = port->addr.ctx->main_conf;
	r->srv_conf = port->addr.ctx->srv_conf;

	/* ke yi xian zhi dfst de requset da xiao */
	size = 64;
	r->header_buf = b = lxl_create_temp_buf(r->pool, 64);
	if (b == NULL) {
		lxl_dfst_close_request(r, LXL_DFS_SERVER_ERROR);
		return;
	}

	n = c->recv(c, b->last, size);
	if (n == LXL_EAGAIN) {
		/*if (!rev->timer_set) {
			lxl_add_timer(rev, 10 * 10000);
		}*/

		rev->handler = lxl_dfst_process_request_header;
		if (lxl_handle_read_event(rev) != 0) {
			lxl_dfst_close_request(r, LXL_DFS_SERVER_ERROR);
			return;
		}
	}

	if (n == LXL_ERROR) {
		lxl_dfst_close_connection(c);
		return;
	}

	if (n == 0) {
		lxl_log_error(LXL_LOG_INFO, 0, "client closed connection");
		lxl_dfst_close_connection(c);
		return;
	}

	b->last += n;

	rev->handler = lxl_dfst_process_request_header;
	lxl_dfst_process_request_header(rev);

	return;
}

static void		
lxl_dfst_empty_handler(lxl_event_t *ev)
{
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst empty handler");

	return;
}

void 
lxl_dfst_request_empty_handler(lxl_dfst_request_t *r)
{
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst request empty handler");

	return;
}

static lxl_dfst_request_t *
lxl_dfst_create_request(lxl_connection_t *c)
{
	lxl_pool_t			*pool;
	lxl_dfst_request_t  *r;

	pool = lxl_create_pool(512);
	if (pool == NULL) {
		return NULL;
	}
	
	r = lxl_pcalloc(pool, sizeof(lxl_dfst_request_t));
	if (r == NULL) {
		lxl_destroy_pool(pool);
		return NULL;
	}

	/* 
 	* set by lxl_pcalloc
 	* first_qtype = 0
 	* new_request = 0
	* response_header = 0
 	*/
	r->pool = pool;
	r->connection = c;
	r->response_header.rcode = LXL_DFS_OK;

	return r;
}

static void
lxl_dfst_process_request_header(lxl_event_t *rev)
{
	size_t 				 n;
	lxl_buf_t 			*b;
	lxl_connection_t  	*c;
	lxl_dfst_request_t  *r;

	c = rev->data;
	r = c->data;
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst process request header");
	
	if (rev->timedout) {
		lxl_log_error(LXL_LOG_ERROR, 0, "client timed out");
		c->timedout = 1;
		lxl_dfst_close_request(r, LXL_DFS_REQUEST_TIMEOUT);
		return;
	}

	b = r->header_buf;
	if (b->last - b->pos < sizeof(lxl_dfs_request_header_t)) {
		n = lxl_dfst_read_request_header(r);
		if (n == LXL_EAGAIN || n == LXL_ERROR) {
			return;
		}

		if (n == 0) {
			lxl_log_error(LXL_LOG_INFO, 0, "dfst client closed connection");
			lxl_dfst_close_connection(r->connection);
			return;
		}
	}

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst read byte:%lu", b->last - b->pos);
	if (b->last - b->pos >= sizeof(lxl_dfs_request_header_t)) {
		if (lxl_dfst_parse_request(r) == -1) {
			lxl_dfst_finalize_request(r, r->response_header.rcode);
			return;
		}

		lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst process request: %s, %u, 0x%04x", 
						r->rid, r->request_header.body_n, r->request_header.qtype);
		//b->last = b->pos;
		lxl_dfst_process_request(r);
	}
}

static ssize_t
lxl_dfst_read_request_header(lxl_dfst_request_t *r)
{
	ssize_t 		   n, rc;
	lxl_buf_t		  *b;
	lxl_event_t		  *rev;
	lxl_connection_t  *c;

	c = r->connection;
	rev = c->read;
	b = r->header_buf;

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
		lxl_dfst_close_request(r, 0);
		rc = LXL_ERROR;
		break;

	default:
		b->last += n;
		rc = n;
	}

	return rc;
}

static void
lxl_dfst_process_request(lxl_dfst_request_t *r)
{
	int				   rc;
	lxl_connection_t  *c;

	c = r->connection;
	if (c->read->timer_set) {
		lxl_del_timer(c->read);
	}

	c->read->handler = lxl_dfst_request_handler;
	c->write->handler = lxl_dfst_request_handler;
	r->read_event_handler = lxl_dfst_block_reading;

	rc = LXL_OK;
	
	switch (r->request_header.qtype) {
	case LXL_DFS_UPLOAD:
	case LXL_DFS_UPLOAD_SC:
		//lxl_dfst_upload_handle_request(r);
		rc = lxl_dfst_read_client_request_body(r, lxl_dfst_upload_handle_request);
		break;

	case LXL_DFS_DOWNLOAD:
		rc = lxl_dfst_read_client_request_body(r, lxl_dfst_download_handle_request);
		break;

	case LXL_DFS_DELETE:
		rc = lxl_dfst_read_client_request_body(r, lxl_dfst_delete_handle_request);
		break;

	case LXL_DFS_STORAGE_REPORT_STATE:
		r->handler = lxl_dfst_storage_report_state_handle_request;
		//rc = lxl_dfst_read_client_request_body(r, lxl_dfst_storage_report_state_handle_request);
		rc = lxl_dfst_read_client_request_body(r, lxl_dfst_tracker_init);
		break;

	case LXL_DFS_STORAGE_REPORT_FID:
		r->handler = lxl_dfst_storage_report_fid_handle_request;
		//rc = lxl_dfst_read_client_request_body(r, lxl_dfst_storage_report_fid_handle_request);
		rc = lxl_dfst_read_client_request_body(r, lxl_dfst_tracker_init);
		break;

	/*case LXL_DFS_STORAGE_SYNC_FID:
		rc = lxl_dfst_read_client_request_body(r, lxl_dfst_storage_sync_handle_request);
		break;*/

	/*case LXL_DFS_STORAGE_SYNC_FID:
		rc = lxl_dfst_read_client_request_body(r, lxl_dfst_storage_sync_fid_handle_request);
		break;*/

	default:
		lxl_log_error(LXL_LOG_ERROR, 0, "dfst Unknow request qtype: 0x%04X", r->request_header.qtype);
		rc = LXL_DFS_NOT_IMPLEMENTED;
		//lxl_dfst_finalize_request(r, LXL_DFS_NOT_IMPLEMENTED);
		break;
	}

	if (rc > LXL_DFS_OK) {
		lxl_dfst_finalize_request(r, rc);
	}
}

static void
lxl_dfst_request_handler(lxl_event_t *ev)
{
	lxl_connection_t   	*c;
	lxl_dfst_request_t  *r;

	c = ev->data;
	r = c->data;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst run request %s", r->rid);

	if (ev->write) {
		r->write_event_handler(r);
	} else {
		r->read_event_handler(r);
	}
}

static void
lxl_dfst_upload_handle_request(lxl_dfst_request_t *r)
{
	lxl_uint_t			 i, len, nelts, max_disk_free;
	lxl_list_t  	    *list1, *list2, *target_list;
	lxl_dfs_ip_port_t 	*ip_port;
	lxl_dfst_idc_t  	*idc;
	lxl_dfst_storage_t	*storage;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst upload handle request");

	idc = lxl_dfst_idc_find(&r->u.idc);
	if (idc == NULL) {
		lxl_dfst_finalize_request(r, LXL_DFS_NOT_FIND_IDC);
		return;
	}

	nelts = lxl_array1_nelts(&idc->storages);
	if (nelts == 0) {
		lxl_dfst_finalize_request(r, LXL_DFS_NOT_FIND_STORAGE);
		return;
	}

	/* sort kusu zhiyao xuan 3ge  4ge seed */
	if (lxl_array_init(&r->storages, r->pool, 3, sizeof(lxl_dfs_ip_port_t)) != 0) {
		lxl_dfst_finalize_request(r, LXL_DFS_SERVER_ERROR);
		return;
	}

	for (i = 0; i < nelts; ++i) {
		if (i == 3) {
			break;
		}

		storage = lxl_array1_data(&idc->storages, lxl_dfst_storage_t, i);
		ip_port = lxl_array_push(&r->storages);
		if (ip_port == NULL) {
			lxl_dfst_finalize_request(r, LXL_DFS_SERVER_ERROR);
			return;
		}

		ip_port->port = storage->port;
		ip_port->ip = storage->ip;
	}
	/*max_disk_free = 0;
	target_list = NULL;
	for (list1 = lxl_list_head(&lxl_dfst_idc_list);
		list1 != lxl_list_sentinel(&lxl_dfst_idc_list); list1 = lxl_list_next(list1)) {
		idc = lxl_list_data(list1, lxl_dfst_idc_t, list);
		if (idc->number < 3) {
			continue;
		}

	 	list2 = lxl_list_head(&idc->list);
		if (list2 != lxl_list_sentinel(&idc->list)) {
			storage = lxl_list_data(list2, lxl_dfst_storage_t, list);
			if (storage->disk_free_mb > max_disk_free) {
				target_list = list2;
			}
		}
	}

	if (target_list == NULL) {
		lxl_dfst_finalize_request(r, LXL_DFS_NOT_FIND_STORAGE);
		return;
	}

	if (lxl_array_init(&r->storages, r->pool, 3, sizeof(lxl_dfs_ip_port_t)) != 0) {
		lxl_dfst_finalize_request(r, LXL_DFS_SERVER_ERROR);
		return;
	}

	i = 0;
	for (list2 = lxl_list_head(target_list);
		list2 != lxl_list_sentinel(target_list); list2 = lxl_list_next(list2)) {
		storage = lxl_list_data(list2, lxl_dfst_storage_t, list);
		ip_port = lxl_array_push(&r->storages);
		if (ip_port == NULL) {
			lxl_dfst_finalize_request(r, LXL_DFS_SERVER_ERROR);
			return;
		}

		ip_port->ip = storage->ip;
		ip_port->port = storage->port;

		++i;
		if (i == 3) {
			break;
		}
	}*/

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "find storage");

	lxl_dfst_send_response(r);
}

static void
lxl_dfst_download_handle_request(lxl_dfst_request_t *r)
{
	lxl_uint_t 			 i, n;
	lxl_str_t   	    *fid;
	lxl_list_t   	    *storage_list, *list;
//	lxl_dfs_ip_port_t   *ip_port;
	lxl_dfs_storage_t  *ip_port, *elt;
	lxl_dfst_fid_t      *dfst_fid;
	lxl_dfst_storage_t  *storage;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst download handle request");

	fid = &r->u.fid;
	
	dfst_fid = lxl_hash1_find(&lxl_dfst_fid_hash, fid->data, fid->len);
	if (dfst_fid == NULL) {
		lxl_log_error(LXL_LOG_INFO, 0, "dfst download fid %s not find", fid->data);
		lxl_dfst_finalize_request(r, LXL_DFS_NOT_FIND);
		return;
	}

	n = lxl_array1_nelts(&dfst_fid->storages);
	if (n == 0) {
		lxl_dfst_finalize_request(r, LXL_DFS_NOT_FIND);
		return;
	}

	if (lxl_array_init(&r->storages, r->pool, 3, sizeof(lxl_dfs_storage_t)) != 0) {
		goto failed;
	}

	for (i = 0; i< n; ++i) {
		ip_port = lxl_array1_data(&dfst_fid->storages, lxl_dfs_storage_t, i);
		elt = lxl_array_push(&r->storages);
		if (elt == NULL) {
			goto failed;
		}

		elt->port = ip_port->port;
		elt->ip = ip_port->ip;
	}
	
	/*storage_list = lxl_hash1_find(&lxl_dfst_fid_hash, fid->data, fid->len);
	if (storage_list == NULL) {
		lxl_log_error(LXL_LOG_INFO, 0, "dfst download fid %s not find", fid->data);
		lxl_dfst_finalize_request(r, LXL_DFS_NOT_FIND);
		return;
	}

	if (lxl_array_init(&r->storages, r->pool, 3, sizeof(lxl_dfs_ip_port_t)) != 0) {
		goto failed;
	}

	for (list = lxl_list_head(storage_list); list != lxl_list_sentinel(storage_list); list = lxl_list_next(list)) {
		storage = lxl_list_data(list, lxl_dfst_storage_t, list);
		ip_port = lxl_array_push(&r->storages);
		if (ip_port == NULL) {
			goto failed;
		}

		ip_port->ip = storage->ip;
		ip_port->port = storage->port;
	}*/

	lxl_dfst_send_response(r);

	return;

failed:

	lxl_dfst_finalize_request(r, LXL_DFS_SERVER_ERROR);
}

static void
lxl_dfst_delete_handle_request(lxl_dfst_request_t *r)
{
	lxl_str_t       *fid;
	lxl_list_t      *storage_list;
	lxl_dfst_fid_t  *dfst_fid;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst delete handle request");

	fid = &r->u.fid;
	dfst_fid = lxl_hash1_del(&lxl_dfst_fid_hash, fid->data, fid->len);
	if (dfst_fid == NULL) {
		lxl_log_error(LXL_LOG_INFO, 0, "fid %s not in dfst fid hash", fid->data);
	} else {
		lxl_array1_destroy(&dfst_fid->storages);
		lxl_free(dfst_fid);
	}

	lxl_dfst_send_header(r);
}

/*static void
lxl_dfst_storage_register_request(lxl_dfst_request_t *r)
{
	lxl_list_t 		     *list1, *list2;
	lxl_dfst_idc_t     *idc;
	lxl_dfs_idc_info_t  info;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst storage register handle request");
	
	info = r->u.idc_info;
	if (lxl_dfst_idc_storage_add(&info) == -1) {
		lxl_log_error(LXL_LOG_INFO, 0, "idc storage register failed, idc id:%hu ip:%u port:%hu", info.idc_id, info.ip, info.port);
		lxl_dfst_finalize_request(r, LXL_DFS_GROUP_STORAGE_EXIST);
		return;
	}

	lxl_dfst_send_header(r);
}*/

static void
lxl_dfst_storage_report_state_handle_request(lxl_dfst_request_t *r)
{
	lxl_int_t				  rc;
	lxl_dfst_idc_t 	         *idc;
	lxl_dfst_storage_t       *storage;
	lxl_dfs_ip_port_t         ip_port;
	lxl_dfs_storage_state_t  *sstate;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst storage report state handle request, %lu", r->number);

	rc = LXL_OK;

	if (r->number < 1) {
		rc = LXL_DFS_BAD_GATEWAY;
		goto failed;
	}

	sstate = &r->u.storage_state;
	if (lxl_dfst_storage_add(sstate, LXL_DFST_BINLOG) == -1) {
		rc = LXL_DFS_SERVER_ERROR;
     	goto failed;
	}

	/*ip_port.ip = info->ip;
	ip_port.port = info->port;

	idc = lxl_dfst_idc_find(info->idc_id);
	if (!idc) {
		idc = lxl_dfst_idc_add(info->idc_id);
		if (!idc) {
			rc = LXL_DFS_SERVER_ERROR;
			goto failed;
		}
	}

	storage = lxl_dfst_storage_find(idc, &ip_port);
	if (storage) {
		idc->sort = 0;
		storage->loadavg = info->loadavg;
		storage->cpu_idle = info->cpu_idle;
		storage->disk_free_mb = info->disk_free_mb;
	} else {
		storage = lxl_dfst_storage_add(idc, info);
		if (storage == NULL) {
			rc = LXL_DFS_SERVER_ERROR;
		 	goto failed;
		}
	}*/

	lxl_dfst_send_header(r);

	return;

failed:

	lxl_dfst_finalize_request(r, rc);
}

static void
lxl_dfst_storage_report_fid_handle_request(lxl_dfst_request_t *r)
{
	lxl_int_t 			 rc = LXL_OK;
	lxl_uint_t			 i, nelts;
	lxl_str_t			*fid;
	lxl_list_t			*list;
	//lxl_slist_t			*slist;
	//lxl_dfs_fid_t		*dfs_fid;
	lxl_dfs_storage_t   *storage;
	//lxl_dfs_storage_t  *ip_port;
	lxl_dfs_fid_t		*dfs_fid;
	lxl_dfst_fid_t		**value, *dfst_fid;
	lxl_dfst_idc_t		*idc;
	lxl_dfst_storage_t  *dfst_storage;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst storage report fid handle request, %lu", r->number);

	if (r->number < 1) {
		rc = LXL_DFS_BAD_GATEWAY;
		goto failed;
	}
	
	if (lxl_dfst_fid_add(&r->u.dfs_fid, LXL_DFST_BINLOG) == -1) {
		rc = LXL_DFS_SERVER_ERROR;
		goto failed;
	}

	//info = &r->u.fid_info;
	/*dfs_fid = &r->u.dfs_fid;

	//for (slist = lxl_slist_head(&info->fid_slist);
	//	slist != lxl_slist_sentinel(); slist = lxl_slist_next(slist)) {
		//dfs_fid = lxl_slist_data(slist, lxl_dfs_fid_t, slist);
		fid = &info->fid;
		value = (lxl_dfst_fid_t **) lxl_hash1_addfind(&lxl_dfst_fid_hash, fid->data, fid->len);
		if (value == NULL) {
			rc = LXL_DFS_SERVER_ERROR;
			goto failed;
		}
	
		if (*value == NULL) {
			dfst_fid = lxl_alloc(sizeof(lxl_dfst_fid_t));
			if (dfst_fid == NULL) {
				rc = LXL_DFS_SERVER_ERROR;
				goto failed;
			}

			*value = dfst_fid;
			if (lxl_array1_init(&dfst_fid->storages, 3, sizeof(lxl_dfs_storage_t)) == -1) {
				rc = LXL_DFS_SERVER_ERROR;
				goto failed;
			}

			//dfst_fid->idc_id = info->idc_id;
		} else {
			dfst_fid = *value;
		}

		nelts = lxl_array_nelts(&dfst_fid->storages);
		for (i = 0; i < nelts; ++i) {
			storage = lxl_array_data(&dfst_fid->storages, lxl_dfs_storage_t, i);
			if (storage->idc_id == info->idc_id && storage->ip == info->ip && storage->port == info->port) {
				lxl_log_error(LXL_LOG_INFO, 0, "storage report ip:%u port:%hu fid:%s exist", info->ip, info->port, fid->data);
				return;
			}
		}

		storage = lxl_array1_push(&dfst_fid->storages);
		if (storage == NULL) {
			rc = LXL_DFS_SERVER_ERROR;
			goto failed;
		}

		storage->idc_id = info->idc_id;
		storage->ip = info->ip;
		storage->port = info->port;
		
		lxl_log_error(LXL_LOG_INFO, 0, "stroage report ip:%u port:%hu fid:%s", info->ip, info->port, fid->data);*/
	//}

	//lxl_dfst_send_header(r);

	/* tongyige idc */
	if (lxl_array_init(&r->storages, r->pool, 4, sizeof(lxl_dfs_storage_t)) == -1) {
		rc = LXL_DFS_SERVER_ERROR;
		goto failed;
	}
	
	idc = lxl_dfst_idc_find(info->idc_id);
	if (idc == NULL) {
		rc = LXL_DFS_NOT_FIND_IDC;
		goto failed;
	}

	nelts = lxl_array1_nelts(&idc->storages);
	for (i = 0; i < nelts; ++i) {
		dfst_storage = lxl_array1_data(&idc->storages, lxl_dfst_storage_t, i);
		if (dfst_storage->port != info->port || dfst_storage->ip != info->ip) {
			storage = lxl_array_push(&r->storages);
			if (storage == NULL) {
				rc = LXL_DFS_SERVER_ERROR;
                goto failed;
			}

			storage->idc_id = idc->id;
			storage->port = dfst_storage->port;
			storage->ip = dfst_storage->ip;
		}
	}
	/*for (list = lxl_list_head(&idc->list);
		list != lxl_list_sentinel(&idc->list); list = lxl_list_next(list)) {
		storage = lxl_list_data(list, lxl_dfst_storage_t, list);
		if (storage->port != info->port || storage->ip != info->ip) {
			idc_info = lxl_array_push(&r->storages);
			if (idc_info == NULL) {
				rc = LXL_DFS_SERVER_ERROR;
				goto failed;
			}

			idc_info->idc_id = info->idc_id;
			idc_info->port = storage->port;
			idc_info->ip = storage->port;
		}
	}*/

	if (lxl_array_nelts(&r->storages) == 0) {
		rc = LXL_DFS_NOT_FIND_STORAGE;
		goto failed;
	}
	
	lxl_dfst_send_response(r);
	
	return;

failed:

	lxl_dfst_finalize_request(r, rc);
}

/*static void
lxl_dfst_storage_sync_handle_request(lxl_dfst_request_t *r)
{
	lxl_int_t			  rc, i;
	lxl_list_t 		     *list;
	lxl_dfs_ip_port_t	 *ip_port;
	lxl_dfst_idc_t     *idc;
	lxl_dfst_storage_t   *storage;
	lxl_dfs_idc_info_t  info;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst storage sync handle request");
	
	i = 0;
	rc = LXL_OK;
	info = r->u.idc_info;

	idc = lxl_dfst_idc_find(info.idc_id);
	if (!idc) {
		rc = LXL_DFS_NOT_FIND_GROUP;
		goto failed;
	}

	if (lxl_array_init(&r->storages, r->pool, 3, sizeof(lxl_dfst_storage_t)) == -1) {
		rc = LXL_DFS_SERVER_ERROR;
		goto failed;
	}

	for (list = lxl_list_head(&idc->list); 
		list != lxl_list_sentinel(&idc->list); list = lxl_list_next(list)) {
		storage = lxl_list_data(list, lxl_dfst_storage_t, list);
		if (storage->ip != info.ip || storage->port != info.port) {
			ip_port = lxl_array_push(&r->storages);
			if (ip_port == NULL) {
				rc = LXL_DFS_SERVER_ERROR;
				goto failed;
			}

			ip_port->ip = storage->ip;
			ip_port->port = storage->port;

			++i;
			if (i == 4) {
				break;
			}
		}
	}

	i = lxl_array_nelts(&r->storages);
	if (i < 2) {
		rc = LXL_DFS_NOT_FIND_STORAGE;
		lxl_log_error(LXL_LOG_WARN, 0, "dfst sync not find enough storages");
		goto failed;
	}

	return;

failed:

	lxl_dfst_finalize_request(r, rc);
}*/

/*static void
lxl_dfst_storage_sync_fid_handle_request(lxl_dfst_request_t *r)
{
	lxl_int_t			   rc;
	lxl_dfs_idc_info_t  *info;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst storage sync fid handle request");

	info = &r->u.idc_info;

	rc = lxl_dfst_sync_fid(r, info);
	if (rc != LXL_OK) {
		goto failed;
	}

	lxl_dfst_send_sync_fid_response(r);
	
	return;

failed:

	lxl_dfst_finalize_request(r, LXL_DFS_SERVER_ERROR);

	return;
}

static void
lxl_dfst_tracker_sync_fid_handle_request(lxl_dfst_request_t *r)
{
	lxl_int_t  rc;
	lxl_dfs_idc_info_t  *info;
	
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst tracker sync fid handle request");

	info = &r->u.idc_info;

	rc = lxl_dfst_sync_fid(r, info);
	if (rc != LXL_OK) {
		lxl_dfst_finalize_request(r, rc);
		return;
	}

	return;
}

static int 
lxl_dfst_sync_fid(lxl_dfst_request_t *r, lxl_dfs_idc_info_t *info)
{
	lxl_uint_t  	   	   i, nelts, j, n;
	lxl_str_t		  	  *fid, *elt;
	lxl_hash_elt_t   	  *temp, *next;
	lxl_dfst_fid_t	   	  *dfst_fid;
	lxl_dfs_ip_port_t     *ip_port;
	lxl_dfs_idc_info_t  *ginfo;

	if (r->sync_fid == NULL) {
		r->sync_fid = lxl_palloc(r->pool, sizeof(lxl_dfst_sync_fid_t));
		if (r->sync_fid == NULL) {
			return LXL_DFS_SERVER_ERROR;
		}

		r->sync_fid->buf = lxl_create_temp_buf(r->pool, LXL_MB_SIZE);
		if (r->sync_fid->buf == NULL) {
			return LXL_DFS_SERVER_ERROR;
		}

		r->sync_fid->queue = lxl_queue_create(r->pool, 1000000, sizeof(lxl_str_t));
		if (r->sync_fid->queue == NULL) {
			return LXL_DFS_SERVER_ERROR;
		}

		r->sync_fid->info.idc_id = 0;
		r->sync_fid->info.ip = 0;
		r->sync_fid->info.port = 0;
	}

	ginfo = &r->sync_fid->info;

	if (info->idc_id == ginfo->idc_id && info->ip == ginfo->ip && info->port == ginfo->port) {
		return LXL_OK;
	}

	lxl_queue_clear(r->sync_fid->queue);

	nelts = lxl_dfst_fid_hash.nelts;
	for (i = 0; i < nelts; ++i) {
		temp = lxl_dfst_fid_hash.buckets[i];

		while (temp) {
			dfst_fid = (lxl_dfst_fid_t *) temp->value;
			if (dfst_fid->idc_id == info->idc_id) {
				n = lxl_array_nelts(&dfst_fid->storage_array);
				for (j = 0; j < n; ++j) {
					ip_port = lxl_array_data(&dfst_fid->storage_array, lxl_dfs_ip_port_t, j);
					if (ip_port->ip == info->ip && ip_port->port == info->port) {
						fid = lxl_palloc(r->pool, sizeof(lxl_str_t));
						if (fid == NULL) {
							return LXL_DFS_SERVER_ERROR;
						}
						
						fid->data = lxl_palloc(r->pool, temp->len + 1);
						if (fid->data == NULL) {
							return LXL_DFS_SERVER_ERROR;
						}

						lxl_str_memcpy(fid, temp->name, temp->len);
						elt = lxl_queue_in(r->sync_fid->queue);
						if (elt == NULL) {
							return LXL_DFS_SERVER_ERROR;
						}

						elt = fid;

						break;
					}
				}
			}
			
			temp = temp->next;
		}
	}
	
	return LXL_OK;
}*/

/*static void
lxl_dfst_send_sync_fid_response(lxl_dfst_request_t *r)
{
	char 	   			 *header;
	size_t				  n;
	lxl_str_t  			 *fid;
	lxl_buf_t  			 *b;
	lxl_dfst_sync_fid_t  *sync_fid;

	sync_fid = r->sync_fid;
	b = r->out = sync_fid->buf;

	if (lxl_queue_empty(sync_fid->queue)) {
		r->loop_request = 0;
	} else {
		r->loop_request = 1;
	}

	lxl_reset_buf(b);
	//r->header = b->pos;
	b->last += sizeof(lxl_dfs_response_header_t);
	n = 0;

	while ((fid = lxl_queue_out(sync_fid->queue)) != NULL) {
		if (b->end - b->pos < fid->len + 1) {
			break;
		}
		
		memcpy(b->last, fid->data, fid->len + 1);
		b->last += fid->len + 1;
		n += fid->len + 1;
	}

	r->response_header.body_n = n;

	lxl_dfst_package_header(r);

	r->write_event_handler = lxl_dfst_writer;

	lxl_dfst_writer(r);
}*/

static void
lxl_dfst_send_response(lxl_dfst_request_t *r)
{
	uint16_t			idc;
	uint32_t			n = 0;
	lxl_uint_t	  	    i, nelts;
	lxl_buf_t	 	   *b;
	lxl_event_t	 	   *wev;
	lxl_connection_t   *c;
//	lxl_dfs_ip_port_t  *ip_port;
	lxl_dfs_storage_t  *storage;

	c = r->connection;
	wev = c->write;
	b = r->out = r->header_buf;

	/*b = r->body->buf;
	if (b->end - b->start < n + 8) { 
		b = lxl_create_temp_buf(r->pool, n + 8);
		if (b == NULL) {
			lxl_dfst_finalize_request(r, LXL_DFS_SERVER_ERROR);
			return;
		}
	}

	r->out = b;*/
	//r->header = b->last;
	lxl_reset_buf(b);
	b->last += 8;

	//lxl_dfst_package_header(r);

	nelts = lxl_array_nelts(&r->storages);
	for (i = 0; i < nelts; ++i) {
		//ip_port = lxl_array_data(&r->storages, lxl_dfs_ip_port_t, i);
		storage = lxl_array_data(&r->storages, lxl_dfs_storage_t, i);
/*		memcpy(b->last, &ip_port->ip, 4);
		b->last += 4;
		memcpy(b->last, &ip_port->port, 2);
		b->last += 2;*/
		idc = htons(storage->idc_id);
		memcpy(b->last, &idc, 2);
		b->last += 2;
		memcpy(b->last, &storage->port, 2);
		b->last += 2;
		memcpy(b->last, &storage->ip, 4);
		b->last += 4;
		n += 8;
	}

	r->response_header.body_n = n;

	lxl_dfst_package_header(r);

	r->write_event_handler = lxl_dfst_writer;

	lxl_dfst_writer(r);
}

static void
lxl_dfst_send_header(lxl_dfst_request_t *r)
{
	lxl_buf_t 		  *b;
	lxl_event_t 	  *wev;
	lxl_connection_t  *c;

	c = r->connection;
	wev = c->write;
	b = r->out = r->header_buf;
//	b->pos = b->start;
//	b->last = b->start;
//	r->header = b->pos;

	lxl_reset_buf(b);
	b->last += sizeof(lxl_dfs_response_header_t);

	lxl_dfst_package_header(r);

	if (r->connection->udp) {
		lxl_dfst_writer_udp(r);
	} else {
		r->write_event_handler = lxl_dfst_writer;
		lxl_dfst_writer(r);
	}
}

static void
lxl_dfst_package_header(lxl_dfst_request_t *r)
{
	lxl_buf_t	*b;

	b = r->out;
	*((uint32_t *) b->pos) = htonl(r->response_header.body_n);
	//b->last += 4;
	*((uint16_t *) (b->pos + 4)) = htons(r->response_header.flen);
	//b->last += 2;
	*((uint16_t *) (b->pos + 6)) = htons(r->response_header.rcode);
	//b->last += 2;

	/**((uint32_t *) r->header) = htonl(r->response_header.body_n);
	r->header += 4;
	*((uint16_t *) r->header) = htons(r->response_header.flags);
	r->header += 2;
	*((uint16_t *) r->header) = htons(r->response_header.rcode);*/
	//b->header += 2;
}

static void
lxl_dfst_writer_udp(lxl_dfst_request_t *r)
{
	lxl_int_t		   rc;
	size_t 			   len;
	ssize_t  		   n;
	lxl_connection_t  *c;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst writer udp handler");
	
	rc = LXL_DFS_OK;
	c = r->connection;
	len = r->out->last - r->out->pos;
	n = c->send(c, r->out->pos, len);

	if (n == -1) {
		lxl_dfst_finalize_request(r, LXL_ERROR);
		return;
	}

	if ((size_t) n != len) {
		lxl_log_error(LXL_LOG_ERROR, errno, "send() incomplete");
		lxl_dfst_finalize_request(r, LXL_ERROR);
		return;
	}

	lxl_dfst_finalize_request(r, LXL_DFS_OK);
}

static void
lxl_dfst_writer(lxl_dfst_request_t *r)
{
	int 			   rc;
	ssize_t			   n;
	lxl_buf_t 		  *b;
	lxl_event_t		  *wev;
	lxl_connection_t  *c;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst writer handler");
	
	c = r->connection;
	wev = c->write;
	b = r->out;

	if (wev->timedout) {
		lxl_log_error(LXL_LOG_INFO, 0, "client timed out");
		c->timedout = 1;
		lxl_dfst_finalize_request(r, LXL_DFS_REQUEST_TIMEOUT);
		return;
	}

	n = c->send(c, b->pos, b->last - b->pos);
	if (n == LXL_ERROR) {
		c->error = 1;
		lxl_dfst_close_request(r, 0);
		return;
	}

	if (n == LXL_EAGAIN) {
		goto done;
	}

	b->pos += n;
	if (b->pos < b->last) {
		goto done;
	}

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst write done");

	lxl_dfst_finalize_request(r, LXL_OK);

	return;

done:

	if (!wev->timer_set) {
		lxl_add_timer(wev, 6 * 1000);
	}

	if (lxl_handle_write_event(wev, 0) != 0) {
		lxl_dfst_close_request(r, 0);
	}

	return;
}

void
lxl_dfst_block_reading(lxl_dfst_request_t *r)
{
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst request reading blocked %s", r->rid);

	return;
}

void
lxl_dfst_finalize_request(lxl_dfst_request_t *r, lxl_int_t rc)
{
	lxl_event_t  *rev;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst finalize request: %s, %ld", r->rid, rc);

	if (rc == LXL_ERROR) { /* client colse request */
		lxl_dfst_close_request(r, rc);
		return;
	}

	if (rc > LXL_DFS_OK) {
		lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "special_response not implement");
		//lxl_dfst_finalize_request(r, lxl_dfst_special_response)
		return;
	}

	/*if (r->loop_request) {
		lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "loop request");
		
		// timer
		r->connection->read->handler = lxl_dfst_process_request_header;
		r->connection->write->handler = lxl_dfst_empty_handler;
		return;
	}*/

	lxl_dfst_close_request(r, LXL_DFS_OK);
}

/*static void
lxl_dfst_set_keepalive(lxl_dfst_request_t *r)
{
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst set keepalive handler");
}

static void
lxl_dfst_keepalive_handler(lxl_event_t *ev)
{
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst keepalive handler");
}*/

static void
lxl_dfst_close_request(lxl_dfst_request_t *r, lxl_int_t rc)
{
	lxl_connection_t  *c;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst close request: %ld", rc);

	c = r->connection;
	lxl_dfst_free_request(r, rc);
	lxl_dfst_close_connection(c);
}

static void
lxl_dfst_free_request(lxl_dfst_request_t *r, lxl_int_t rc)
{
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst free request");

	lxl_destroy_pool(r->pool);
}

static void 
lxl_dfst_close_connection(lxl_connection_t *c)
{
	lxl_pool_t  *pool;
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst close connection");

	pool = c->pool;
	lxl_close_connection(c);

	if (pool) {
		lxl_destroy_pool(c->pool);
	}
}
