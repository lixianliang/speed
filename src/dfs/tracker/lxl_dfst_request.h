
/*
 * Copyright (C) xianliang.li
 */


#ifndef LXL_DFST_REQUEST_H_INCLUDE
#define LXL_DFST_REQUEST_H_INCLUDE


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_dfst.h>


//typedef int	 (*lxl_dfst_handler_pt) (lxl_dfst_request_t *r);
typedef void (*lxl_dfst_event_handler_pt) (lxl_dfst_request_t *r);

/*typedef struct {
	uint32_t	body_n;
	uint16_t 	qtype;
	uint8_t		flen;
	uint8_t		rcode;
} lxl_dfst_request_header_t;*/

typedef void (* lxl_dfst_handler_pt)			 (lxl_dfst_request_t *r);
typedef int	 (* lxl_dfst_body_handler_pt)		 (lxl_dfst_request_t *r);
typedef void (* lxl_dfst_client_body_handler_pt) (lxl_dfst_request_t *r);
/*
typedef struct {
	lxl_buf_t		     *buf;
	lxl_queue_t			 *queue;		// lxl_str_t fid 
	lxl_dfs_idc_info_t  info;
} lxl_dfst_sync_fid_t;*/

typedef struct {
	unsigned 						 first_process_buf;
	off_t  							 rest;
	lxl_buf_t  						*buf;	/* buffer */
	lxl_dfst_body_handler_pt  		 body_handler;
	lxl_dfst_client_body_handler_pt  post_handler;
} lxl_dfst_request_body_t;

struct lxl_dfst_request_s {
	lxl_connection_t 		 	    *connection;
	
	void				     	   **main_conf;
	void				     	   **srv_conf;

	lxl_dfst_event_handler_pt  		 read_event_handler;
	lxl_dfst_event_handler_pt  		 write_event_handler;
	
	lxl_dfst_upstream_t				*tracker;
	lxl_dfst_upstream_t		  	    *storage;
	lxl_array_t				   	 	 storage_array;

	lxl_pool_t				  		*pool;

	//char 							*header;
	lxl_buf_t				  		*header_buf;
	//lxl_buf_t						 buffer;
	lxl_dfs_request_header_t   		 request_header;
	lxl_dfs_response_header_t  		 response_header;
	lxl_dfst_request_body_t	  		*body;

	union {
		lxl_str_t				   	 fid;
		lxl_dfs_fid_t			     dfs_fid;
		//lxl_dfs_idc_info_t	   	 idc_info;
		lxl_dfs_idc_t				 idc;
		lxl_dfs_storage_t			 storage;
		lxl_dfs_storage_state_t	   	 storage_state;
	} u;

	//lxl_dfst_sync_fid_t			    *sync_fid;

	lxl_array_t				   	 	 storages;		/* lxl_dfs_ip_port_t */

	lxl_buf_t				  		*out;

	char 							 rid[24];

	lxl_dfst_handler_pt				 handler;

	lxl_uint_t						 number;

	unsigned 						 loop_request;
};


#endif	/* LXL_DFST_REQUEST_H_INCLUDE */
