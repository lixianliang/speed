
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_files.h>
#include <lxl_dfss.h>


static void	lxl_dfss_send_client_response_body_handler(lxl_dfss_request_t *r);
static int 	lxl_dfss_do_send_client_response_body(lxl_dfss_request_t *r);


int 
lxl_dfss_send_client_response_body(lxl_dfss_request_t *r, lxl_dfss_client_body_handler_pt post_handler)
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
		size = rb->rest;
	} else {
		size = 8192;
	}

	rb->buf = lxl_create_temp_buf(r->pool, size);
	if (rb->buf == NULL) {
		return -1;
	}

	/*if (b == NULL) {
		b = lxl_create_temp_buf(r->pool, size);
		if (b == NULL) {
			return -1;
		}

		rb->buf = b;
	} else {
		old_size = b->end - b->start;
		if (old_size < size) {
			if (lxl_pfree(r->pool, b->start) == 0) {
				lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss pfree %lu, palloc %lu", old_size, size);
			}

			b->data = lxl_palloc(r->connection->pool, size);
			if (b->data == NULL) {
				return -1;
			}

			b->start = b->data;
			b->pos = b->start;
			b->last = b->start;
			b->end = b->start + size;
		} else {
			b->pos = b->start;
			b->last = b->start;
		}
	}*/

	r->read_event_handler = lxl_dfss_request_empty_handler;
	r->write_event_handler = lxl_dfss_send_client_response_body_handler;

	rc = lxl_dfss_do_send_client_response_body(r);

	return rc;
}

static void	
lxl_dfss_send_client_response_body_handler(lxl_dfss_request_t *r)
{
	int rc;

	if (r->connection->write->timedout) {
		r->connection->timedout = 1;
		// lxl_fin
		return;
	}

	rc = lxl_dfss_do_send_client_response_body(r);
	if (rc) {
	}

	return;
}

static int 	
lxl_dfss_do_send_client_response_body(lxl_dfss_request_t *r)
{
	size_t 					  size;
	ssize_t 				  n;
	lxl_buf_t 				 *b;
	lxl_file_t 				 *file;
	lxl_connection_t 		 *c;
	lxl_dfss_request_body_t  *rb;

	c = r->connection;
	rb = r->body;
	b = rb->buf;
	file = &rb->file;

	for (; ;) {
		for (; ;) {
			size = b->last - b->pos;
			if (size == 0) {
				b->pos = b->start;
				b->last = b->start;
				size = b->end - b->last;
				if (rb->rest < (off_t) size) {
					size = rb->rest;
				}

				n = lxl_read_file1(file, b->last, size);
				if (n == -1) {
					//lxl_finalize
					return -1;
				}

				b->last += n;
			}

			//size = b->last - b->pos;
			n = c->send(c, b->pos, size);
			if (n == LXL_EAGAIN) {
				break;
			}

			if (n == LXL_ERROR) {
				return -1;
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

	/*if (close(file->fd) == -1) {
		lxl_log_error(LXL_LOG_WARN, errno, "close() failed");
	}*/

	if (c->write->timer_set) {
		lxl_del_timer(c->write);
	}
	
	rb->post_handler(r);

	return LXL_OK;
}
