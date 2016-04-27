
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_config.h>
#include <lxl_core.h>
//#include <lxl_files.h>
#include <lxl_dfss.h>


static void lxl_dfss_read_client_request_body_handler(lxl_dfss_request_t *r);
static int 	lxl_dfss_do_read_client_request_body(lxl_dfss_request_t *r);

//static int	lxl_dfss_process_request_body(lxl_dfss_request_t *r);
// static int lxl_dfss_generate_file(lxl_dfss_request_t *r);
static int	lxl_dfss_upload_request_body(lxl_dfss_request_t *r);
static int 	lxl_dfss_download_request_body(lxl_dfss_request_t *r);
// push过来的  posh_handler 可以去掉? 其他
static int	lxl_dfss_sync_push_fid_part_request_body(lxl_dfss_request_t *r);

static void	lxl_dfss_send_body_handler(lxl_dfss_request_t *r);
static int	lxl_dfss_do_send_body(lxl_dfss_request_t *r);


int 
lxl_dfss_read_client_request_body(lxl_dfss_request_t *r, lxl_dfss_client_body_handler_pt post_handler)
{
	// discard body
	int 					  rc;
	size_t 					  n, len1, len2, size, rest, preread;
	uint16_t 				  dir;
	char 					  fid[64], filename[64], *base_path = "data";
	//size_t 					  size, preread, old_size;
	lxl_buf_t  				 *b, *buf;
	lxl_file_t 				 *file;
	lxl_dfss_request_body_t  *rb;
	
	rb = lxl_pcalloc(r->pool, sizeof(lxl_dfss_request_body_t));
	if (rb == NULL) {
		return LXL_DFS_SERVER_ERROR;
	}

	rb->file.fd = -1;
	rb->post_handler = post_handler;
	r->body = rb;

	if (r->request_header.qtype == LXL_DFS_UPLOAD || r->request_header.qtype == LXL_DFS_UPLOAD_SC) {
		rest = rb->rest = r->request_header.body_n;
		rb->body_handler = lxl_dfss_upload_request_body;

		file = &rb->file;
		dir = lxl_dfss_dir_seed % LXL_DFSS_DIR_COUNT;
		/* 2+16+8+4+2=32 */
		len1 = (size_t) snprintf(fid, sizeof(fid), "%02x%s%08lx%04x01",
				lxl_dfss_area_id, lxl_dfss_uid, lxl_current_sec, lxl_dfss_fid_seed);

		len2 = (size_t) snprintf(filename, sizeof(filename), "%s/%02x/%02x%s%08lx%04x00", 
				base_path, dir, lxl_dfss_area_id, lxl_dfss_uid, lxl_current_sec, lxl_dfss_fid_seed);

		rb->fid.data = lxl_pnalloc(r->pool, len1 + 1);
		if (rb->fid.data == NULL) {
			return LXL_DFS_SERVER_ERROR;
		}

		lxl_str_memcpy(&rb->fid, fid, len1);

		file->name.data = lxl_pnalloc(r->pool,  len2 + 1);
		if (file->name.data == NULL) {
			return LXL_DFS_SERVER_ERROR;
		}

		lxl_str_memcpy(&file->name, filename, len2);

		++lxl_dfss_dir_seed;
		++lxl_dfss_fid_seed;

		file->fd = open(filename, O_CREAT|O_EXCL|O_WRONLY, 0644);
		if (file->fd == -1) {
			lxl_log_error(LXL_LOG_ALERT, errno, "dfss upload generate temp file %s failed", filename);
			return LXL_DFS_SERVER_ERROR;
		}

		if (r->request_header.body_n > 512) {
			if (ftruncate(file->fd, rb->rest) == -1) {
				lxl_log_error(LXL_LOG_ERROR, errno, "ftruncate() failed");
			}
		}

		lxl_log_error(LXL_LOG_INFO, 0, "dfss genrate temp file %s", file->name.data);
	} else if (r->request_header.qtype == LXL_DFS_DOWNLOAD || r->request_header.qtype == LXL_DFS_STORAGE_SYNC_PULL) {
		rest = rb->rest = r->request_header.body_n;
		rb->body_handler = lxl_dfss_download_request_body;
	} else {
		/* LXL_DFS_STORAGE_SYNC_PUSH */
		//rb->rest = r->request_header.body_n + r->request_header.flen;
		rest = r->request_header.body_n + r->request_header.flen;
		rb->rest = r->request_header.flen;
		rb->body_handler = lxl_dfss_sync_push_fid_part_request_body;
	}

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss request rest %ld", rb->rest);

	size = 8192;
	size += size >> 2;
	if (rest < size) {
		size = rest;
		r->request_body_in_onebuf = 1;
	} else {
		size = 8192;
	}

	rb->buf = lxl_create_temp_buf(r->pool, size);
	if (rb->buf == NULL) {
		//rc = LXL_DFS_SERVER_ERROR;
        //goto done;
		return LXL_DFS_SERVER_ERROR;
	}

	b = r->header_buf;
	preread = b->last - b->pos;
	if (preread) {
		/* there is the pre-read part of the request body */

		n = lxl_min(preread, rb->rest);
		lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss client request body preread %lu, %lu", preread, n);

		memcpy(rb->buf->pos, b->pos, n);
		b->pos += n;
		rb->buf->last += n;
		rb->rest -= n;
		/*if (preread < rb->rest) {
			r->header_buf->pos += preread;
			rb->buf->last += preread;
			memcpy(rb->buf->pos, r->header_buf->pos, preread);
			rb->rest -= preread;
		} else {
			r->header_buf->pos += rb->rest;
			rb->buf->last += rb->rest;
			memcpy(rb->buf->pos, r->header_buf->pos, rb->rest);
			rb->rest = 0;
		}*/
	}

	if (rb->rest == 0) {
		rc = rb->body_handler(r);
		if (rc != LXL_OK) {
			return LXL_DFS_SERVER_ERROR;
		}

		rb->post_handler(r);

		return LXL_OK;
	}

	r->read_event_handler = lxl_dfss_read_client_request_body_handler;
	r->write_event_handler = lxl_dfss_request_empty_handler;

	rc = lxl_dfss_do_read_client_request_body(r);
	
	return rc;
/*done:
	
	return rc;
	if (rc == LXL_EAGAIN) {
		return LXL_OK;
	} else {
		return rc;
	}*/
}

static void
lxl_dfss_read_client_request_body_handler(lxl_dfss_request_t *r)
{
	int  rc;
	//lxl_dfss_request_body_t *rb;

	if (r->connection->read->timedout) {
		r->connection->timedout = 1;
		lxl_dfss_finalize_request(r, LXL_DFS_REQUEST_TIMEOUT);
		return;
	}

	/* rc is again or > lxl_dfs_ok */
	rc = lxl_dfss_do_read_client_request_body(r);
	if (rc > LXL_DFS_OK) {
		lxl_dfss_finalize_request(r, rc);
	}
}

static int
lxl_dfss_do_read_client_request_body(lxl_dfss_request_t *r) 
{
	//int 	i;
	off_t 	rest;
	size_t 	size;
	ssize_t n;
	lxl_buf_t *b;
	lxl_file_t *file;
	lxl_connection_t *c;
	lxl_dfss_request_body_t *rb;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss do read client request body");

	c = r->connection;
	rb = r->body;
	b = rb->buf;

	for (; ;) {
		for (; ;) {
			if (b->last == b->end) {
				//rb->rest -= n;
				if (rb->body_handler(r) == LXL_ERROR) {
					return LXL_DFS_SERVER_ERROR;
				}

				/*b->pos = b->start;
				b->last = b->start;
				b->last = b->pos;
				rb->rest -= n;*/
			}

			/* read size */
			size =  b->end - b->last;
			rest = rb->rest - (b->last - b->pos);
			if ((off_t) size > rest) {
				size = (size_t) rest;
			}

			n = c->recv(c, b->last, size);

			lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss client request body recv %ld", n);

			if (n == LXL_EAGAIN) {
				break;
			}

			if (n == 0) {
				lxl_log_error(LXL_LOG_INFO, 0, "client prematurely closed connection");
			}
			
			if (n == 0 || n == LXL_ERROR) {
				c->error = 1;
				goto failed;
			}
			
			b->last += n;

			if (n == rest) {
				/* done reading */
				rb->rest -= (b->last - b->pos);
			}

			if(rb->rest == 0) {
				break;
			}

			if (b->last < b->end) {
				break;
			}
		}

		lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss client request body rest %ld", rb->rest);

		if(rb->rest == 0) {
			break;
		}

		if (!c->read->ready) {
			lxl_add_timer(c->read, 60000);	/* nginx 60000 */
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

	//b->last = b->pos;

	r->read_event_handler =  lxl_dfss_block_reading;
	rb->post_handler(r);
	// lxl_dfss_request_handler

	return LXL_OK;

failed:

	file = &rb->file;
	if (close(file->fd) == -1) {
		lxl_log_error(LXL_LOG_ERROR, errno, "close() failed");
	}

	if (unlink(file->name.data) == -1) {
		lxl_log_error(LXL_LOG_ERROR, errno, "unlink(%s) failed", file->name.data);
	}

	return LXL_DFS_BAD_REQUEST;
}

static int
lxl_dfss_upload_request_body(lxl_dfss_request_t *r)
{
	//int 					  fd;
	size_t 					  n;
	//uint16_t 				  dir;
	//char 					  fid[64], filename[64], *base_path = "data";
	lxl_buf_t 				 *b;
	lxl_file_t 				 *file;
	lxl_dfss_request_body_t  *rb;

	rb = r->body;
	b = rb->buf;
	file = &rb->file;
	//if (file->name.data == NULL) {
		/*dir = lxl_dfss_dir_seed % LXL_DFSS_DIR_COUNT;
		// 16+8+4=28 
		len1 = (size_t) snprintf(fid, sizeof(fid), "%02x/%08lx%s%04x01",
				dir, lxl_current_sec, lxl_dfss_uid, lxl_dfss_fid_seed);

		len2 = (size_t) snprintf(filename, 64, "%s/%02x/%08lx%s%04x00", 
				base_path, dir, lxl_current_sec, lxl_dfss_uid, lxl_dfss_fid_seed);

		rb->fid.data = lxl_pnalloc(r->pool, len1 + 1);
		if (rb->fid.data == NULL) {
			return -1;
		}

		lxl_str_memcpy(&rb->fid, fid, len1);

		file->name.data = lxl_pnalloc(r->pool,  len2 + 1);
		if (file->name.data == NULL) {
			return -1;
		}
		
		lxl_str_memcpy(&file->name, filename, len2);

		++lxl_dfss_dir_seed;
		++lxl_dfss_fid_seed;


		fd = open(filename, O_CREAT|O_EXCL|O_WRONLY, 0644);
		if (fd == -1) {
			lxl_log_error(LXL_LOG_ALERT, errno, "dfss generate temp file %s failed", filename);
			return -1;
		}

		if (rb->rest > 4096) {
			if (ftruncate(fd, rb->rest) == -1) {
				lxl_log_error(LXL_LOG_ERROR, errno, "ftruncate() failed");
			}
		}

		file->fd = fd;
		//rb->file = file;
		r->new_request = 0;
		lxl_log_error(LXL_LOG_INFO, 0, "dfss genrate temp file %s", file->name.data);*/

#if (LXL_DEBUG)
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss body buf: start:%p pos:%p size:%lu", b->start, b->pos, b->last - b->pos);
#endif
	n = b->last - b->pos;
	if (lxl_write_file(file, b->pos, n) == -1) {
		lxl_log_error(LXL_LOG_ALERT, errno, "write() failed");
		goto failed;
	}

	/*b->pos = b->start;
	  b->last = b->start;*/
	b->last = b->pos;
	//rb->rest -= n;
	if (rb->rest == 0) {
		char new_name[128];
		memcpy(new_name, file->name.data, file->name.len);
		new_name[file->name.len] = '\0';
		new_name[file->name.len - 1] = '1';
		if (rename(file->name.data, new_name) == -1) {
			lxl_log_error(LXL_LOG_ERROR, errno, "rename(%s, %s) failed", file->name.data, new_name);
			goto failed;
		}

		//memcpy(file->name.data, new_name, len);
		file->name.data[file->name.len - 1] = '1';
		if (fsync(file->fd) == -1) {
			lxl_log_error(LXL_LOG_ALERT, errno, "fsync() failed");
			goto failed;
		}

		if (close(file->fd) == -1) {
			lxl_log_error(LXL_LOG_INFO, errno, "close() failed");
			/* return -1; igonre */
		}

		lxl_log_error(LXL_LOG_INFO, 0, "dfss upload file %s", new_name);
	}
	
	return LXL_OK;

failed:

	if (close(file->fd) == -1) {
		lxl_log_error(LXL_LOG_ERROR, errno, "close() failed");
	}

	if (unlink(file->name.data) == -1) {
		lxl_log_error(LXL_LOG_ERROR, errno, "unlink(%s) failed", file->name.data);
	}

	return LXL_ERROR;
}

static int
lxl_dfss_download_request_body(lxl_dfss_request_t *r)
{
	size_t 					  n;
	lxl_str_t				 *fid;
	lxl_buf_t 				 *b;
	lxl_file_t 				 *file;
	lxl_dfss_fid_value_t	 *value;
	lxl_dfss_request_body_t  *rb;

	rb = r->body;
	b = rb->buf;
	fid = &rb->fid;
	file = &rb->file;

	n = r->request_header.body_n;

	fid->data = lxl_pnalloc(r->pool, n + 1);
	if (fid->data == NULL) {
		return LXL_ERROR;
	}

	lxl_str_memcpy(fid, b->pos, n);

	/*value = lxl_hash1_find(&lxl_dfss_fid_hash, fid->data, fid->len);
	if (value == NULL) {
		lxl_log_error(LXL_LOG_ERROR, 0, "dfss download %s not find file", fid->data);
		return LXL_DFS_NOT_FIND_FID;
	}

	file->name.data = lxl_palloc(r->pool, value->name.len + 1);
	if (file->name.data == NULL) {
		return -1;
	}

	lxl_str_memcpy(&file->name, value->name.data, value->name.len);

	file->info.st_size = value->size;*/
	//r->respons_header.body_n = htonl(value->size);
	
	/*n = r->request_header.body_n;
	if (n > 63) {
		lxl_log_error(LXL_LOG_ERROR, 0, "dfss download fid is too long %u, max is 63", n);
		return -1;
	}

	memcpy(fid, b->pos, n);
	fid[n] = '\0';
	
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss %s download fid %s", r->rid, fid);
	p = strchr(fid, '/');
	if (p == NULL) {
		lxl_log_error(LXL_LOG_ERROR, 0, "dfss download fid %s invalid", fid);
		return -1;
	}

	*p = '\0';
	group_id = lxl_hextoi(fid, p - fid);
	if (group_id == -1) {
		lxl_log_error(LXL_LOG_ERROR, 0, "dfss groupid invalid, lxl_hextoi(%s) failed", fid);
		return -1;
	

	if ((lxl_int_t) lxl_dfss_group_id != group_id) {
		lxl_log_error(LXL_LOG_ERROR, 0, "dfss download group_id %ld, not same group_id %u", lxl_dfss_group_id);
		return -1;
	}

	//p = '/';
	n = (size_t) snprintf(filename, 128, "%s%s", base_path, p + 1);
	++n;
	if (file->name.data == NULL) {
		file->name.data = lxl_palloc(r->connection->pool, n);
		if (file->name.data == NULL) {
			return -1;
		}
	} else {
		if (file->name.len < n) {
			lxl_log_error(LXL_LOG_WARN, 0, "dfss download file len: prev:%lu now:%lu", file->name.len, n);
			file->name.data = lxl_palloc(r->connection->pool, n);
			if (file->name.data == NULL) {
				return -1;
			}
		}
	}

	file->name.len = n;
	memcpy(file->name.data, filename, n);*/
	
	return LXL_OK;
}

static int
lxl_dfss_sync_push_fid_part_request_body(lxl_dfss_request_t *r)
{
	int 					  rc;
	size_t 					  n, len, preread;
	uint16_t 				  dir;
	char 					  filename[64], *base_path = "data";
	lxl_buf_t 		         *b;
	lxl_file_t				 *file;
	lxl_dfss_request_body_t  *rb;

	rb = r->body;
	//b = rb->buf;
	file = &rb->file;
	
	n = rb->buf->last - rb->buf->pos;
	if (n != r->request_header.flen) {
		return LXL_ERROR;
	}

	rb->fid.data = lxl_pnalloc(r->pool, n + 1);
	if (rb->fid.data == NULL) {
		return LXL_ERROR;
	}

	lxl_str_memcpy(&rb->fid, rb->buf->pos, n);

	dir = lxl_dfss_dir_seed % LXL_DFSS_DIR_COUNT;
	len = (size_t) snprintf(filename, sizeof(filename), "%s/%02x/%s", base_path, dir, rb->fid.data);
	filename[len - 1] = '0';

	file->name.data = lxl_pnalloc(r->pool, len + 1);
	if (file->name.data == NULL) {
		return LXL_ERROR;
	}

	lxl_str_memcpy(&file->name, filename, len);

	++lxl_dfss_dir_seed;

	file->fd = open(filename, O_CREAT|O_EXCL|O_WRONLY, 0644);
	if (file->fd == -1) {
		lxl_log_error(LXL_LOG_ALERT, errno, "dfss sync push generate temp file %s failed", filename);
		return LXL_ERROR;
	}

	if (r->request_header.body_n > 512) {
		if (ftruncate(file->fd, r->request_header.body_n)) {
			lxl_log_error(LXL_LOG_ERROR, errno, "ftruncate() failed");
		}
	}

	lxl_log_error(LXL_LOG_INFO, 0, "dfss sync push generate temp file file %s", file->name.data);
	
	//b->pos += r->request_header.flen;
	rb->rest = r->request_header.body_n;
	rb->body_handler = lxl_dfss_upload_request_body;
	lxl_reset_buf(rb->buf);

	b = r->header_buf;
	preread = b->last - b->pos;
	if (preread) {
		n = lxl_min(preread, rb->rest);
		lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss sync push client request body preread %lu, %lu", preread, n);
		
		memcpy(rb->buf->pos, b->pos, n);
		rb->buf->last += n;
		rb->rest -= n;
	}

	if (rb->rest == 0) {
		rc = rb->body_handler(r);
		if (rc != LXL_OK) {
			return LXL_DFS_SERVER_ERROR;
		}

		return LXL_OK;
	}

	/*n -= r->request_header.flen;
	memmove(b->start, b->pos, n);
	b->pos = b->start;
	b->last = b->pos + n;*/

	rc = lxl_dfss_do_read_client_request_body(r);

	//return rc;
	if (rc == LXL_EAGAIN) {
		return LXL_OK;
	} else {
		return rc;
	}
}

/*static int	
lxl_dfss_sync_push_request_body(lxl_dfss_request_t *r)
{
	size_t 					  n;
	lxl_buf_t 				 *b;
	lxl_dfss_request_body_t  *rb;

	rb = r->body;
	b = rb->buf;
	file = &rb->file;
	
	return 0;
}*/

int 
lxl_dfss_send_body(lxl_dfss_request_t *r, lxl_dfss_client_body_handler_pt post_handler)
{
	int rc;
	size_t size, old_size;
	lxl_buf_t *b;
	lxl_dfss_request_body_t *rb;

	rb = r->body;
	rb->post_handler = post_handler;
	b = rb->buf;

	rb->rest = r->response_header.body_n;
	size = 8192;
	size += size >> 2;
	if (rb->rest < size) {
		size = (rb->rest / 1024 + 1) * 1024;
	} else {
		size = 8192;
	}

	if (b == NULL) {
		b = lxl_create_temp_buf(r->connection->pool, size);
		if (b == NULL) {
			return LXL_DFS_SERVER_ERROR;
		}

		rb->buf = b;
	} else {
		old_size = b->end - b->start;
		if (old_size < size) {
			if (lxl_pfree(r->connection->pool, b->start) == 0) {
				lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss pfree %lu, palloc %lu", old_size, size);
			}

			b->start= lxl_palloc(r->connection->pool, size);
			if (b->start == NULL) {
				return LXL_DFS_SERVER_ERROR;
			}

			b->pos = b->start;
			b->last = b->start;
			b->end = b->start + size;
		} else {
			b->pos = b->start;
			b->last = b->start;
		}
	}

	r->read_event_handler = lxl_dfss_request_empty_handler;
	r->write_event_handler = lxl_dfss_send_body_handler;

	rc = lxl_dfss_do_send_body(r);

	//return rc;
	if (rc == LXL_EAGAIN) {
		return LXL_OK;
	} else {
		return rc;
	}
}

static void	
lxl_dfss_send_body_handler(lxl_dfss_request_t *r)
{
	int rc;

	if (r->connection->write->timedout) {
		r->connection->timedout = 1;
		// lxl_fin
		return;
	}

	rc = lxl_dfss_do_send_body(r);
	if (rc) {
	}

	return;
}

static int 	
lxl_dfss_do_send_body(lxl_dfss_request_t *r)
{
	size_t size;
	ssize_t n;
	lxl_buf_t *b;
	lxl_file_t *file;
	lxl_connection_t *c;
	lxl_dfss_request_body_t *rb;

	c = r->connection;
	rb = r->body;
	b = rb->buf;
	file = &rb->file;

	for (; ;) {
		for (; ;) {
			size = b->last - b->pos;
			if (size == 0) {
				//b->pos = b->start;
				//b->last = b->start;
				lxl_reset_buf(b);
				size = b->end - b->last;
				if (rb->rest < (off_t) size) {
					size = rb->rest;
				}

				n = lxl_read_file1(file, b->pos, size);
				if (n == -1) {
					//lxl_finalize
					return LXL_DFS_SERVER_ERROR;
				}

				b->last += n;
			}

			//size = b->last - b->pos;
			n = c->send(c, b->pos, size);
			if (n == LXL_EAGAIN) {
				break;
			}

			if (n == LXL_ERROR) {
				return LXL_DFS_SERVER_ERROR;
				//goto failed;
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

	if (close(file->fd) == -1) {
		lxl_log_error(LXL_LOG_WARN, errno, "close() failed");
	}

	if (c->write->timer_set) {
		lxl_del_timer(c->write);
	}
	
	rb->post_handler(r);

	return LXL_OK;
}
