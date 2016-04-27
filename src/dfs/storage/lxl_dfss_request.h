
/*
 * Copyright (C) xianliang.li
 */


#ifndef LXL_DFSS_REQUEST_H_INCLUDE
#define LXL_DFSS_REQUEST_H_INCLUDE


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_dfss.h>
//#include <lxl_dfss_upstream.h>


typedef int	 (*lxl_dfss_handler_pt) (lxl_dfss_request_t *r);
typedef void (*lxl_dfss_event_handler_pt) (lxl_dfss_request_t *r);
//typedef void (*lxl_dfss_client_body_handler_pt) (lxl_dfss_request_t *r);

/* 4byte 1bytecmd 1bytestatus body */
/*typedef struct {
	uint32_t  body_n;
	uint16_t  qtype;
	uint8_t   flen;
	uint8_t   rcode;
} lxl_dfss_request_header_t;*/

typedef void (*lxl_dfss_client_body_handler_pt) (lxl_dfss_request_t *r);
typedef int	 (*lxl_dfss_body_handler_pt)		(lxl_dfss_request_t *r);

typedef struct {
	lxl_str_t						 fid;
	lxl_file_t  					 file;
	// lxl_str_t fid;
	off_t 		 					 rest;
	/* lxl_temp_file_t */
	lxl_buf_t 						*buf;
	//lxl_dfss_client_body_handler_pt body_handler;
	lxl_dfss_body_handler_pt 		 body_handler;
	lxl_dfss_client_body_handler_pt  post_handler;
} lxl_dfss_request_body_t;

struct lxl_dfss_request_s {
	lxl_connection_t 		   *connection;

	void				      **main_conf;
	void			          **srv_conf;

	lxl_dfss_event_handler_pt	read_event_handler;
	lxl_dfss_event_handler_pt	write_event_handler;

	lxl_dfss_upstream_t 	   *tracker;
	lxl_dfss_upstream_t 	   *storage;
	lxl_array_t 			    storage_array;	/* lxl_dfs_addr_t */
	lxl_queue_t					addrs;			/* lxl_dfss_addr_t */

	lxl_pool_t 				   *pool;

	//lxl_buf_t				  header_buf;
	lxl_buf_t				   *header_buf;
	lxl_dfs_request_header_t 	request_header;
	lxl_dfs_response_header_t	response_header;
	lxl_dfss_request_body_t	   *body;

	unsigned					request_body_in_onebuf:1;
	unsigned					request_closed_connection:1;
	unsigned 					first_qtype:1;
	unsigned 					new_request:1;
	unsigned 					complete:1;
	unsigned 					onebuf:1;
	unsigned 					keepalive:1;
	unsigned 					sync:1;
	//unsigned  strong_sync:1
	//unsigned 	weak_sync:1;
	//lxl_dfss_handler_pt		  content_handler;
	//lxl_chain_t *out;	 response
	// 一般的response 为header + body(几十byte)
	// 一个连接就一个request 用c->pool http 一个连接多个request r->pool
	lxl_buf_t 				   *out;
	//lxl_array
	char 					 	rid[24];

	lxl_dfss_handler_pt			handler;
};


#endif /* LXL_DFSS_REQUEST_H_INCLUDE */
