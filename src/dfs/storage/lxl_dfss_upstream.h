
/*
 * Copyright (C) xianliang.li
 */


#ifndef LXL_DFSS_UPSTREAM_H_INCLUDE
#define LXL_DFSS_UPSTREAM_H_INCLUDE


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_event_connect.h>
#include <lxl_dfss.h>


#define LXL_DFSS_UPSTREAM_FT_ERROR		0x00000002
#define LXL_DFSS_UPSTREAM_FT_TIMEOUT	0x00000004
#define LXL_DFSS_UPSTREAM_FT_NOLIVE		0x00000010

#define LXL_DFSS_UPSTREAM_FT_TRY_OVER   610

#define LXL_DFSS_UPSTREAM_CREATE		0x0001
#define LXL_DFSS_UPSTREAM_STORAGE		0x0010


typedef struct lxl_dfss_upstream_srv_conf_s lxl_dfss_upstream_srv_conf_t;
typedef int	(*lxl_dfss_upstream_init_pt) (lxl_conf_t *conf, lxl_dfss_upstream_srv_conf_t *us);
typedef int	(*lxl_dfss_upstream_init_peer_pt) (lxl_dfss_request_t *r, lxl_dfss_upstream_srv_conf_t *us, lxl_dfss_upstream_t *u);

typedef struct {
	lxl_array_t	 upstreams;		/* lxl_dfss_upstream_srv_conf_t */
} lxl_dfss_upstream_main_conf_t;

typedef struct {
	lxl_dfss_upstream_init_pt		init_upstream;
	lxl_dfss_upstream_init_peer_pt	init;
	void 						   *data;
} lxl_dfss_upstream_peer_t;

typedef struct {
	char		sockaddr[LXL_SOCKADDRLEN];
	socklen_t	socklen;
	lxl_uint_t  max_fails;
	time_t 		fail_timeout;
	lxl_uint_t  down;
	char 	   *name;
} lxl_dfss_upstream_server_t;

struct lxl_dfss_upstream_srv_conf_s {
	lxl_dfss_upstream_peer_t  peer;
	void 	  				**srv_conf;
	lxl_array_t			 	 *servers;

	lxl_uint_t				  flags;
	lxl_str_t				  host;
	in_port_t 				  port;
	in_port_t 				  default_port;
	lxl_uint_t 		          no_port;
};

typedef struct {
	lxl_dfss_upstream_srv_conf_t  *upstream;
	
	lxl_msec_t					   connect_timeout;
	lxl_msec_t					   send_timeout;
	lxl_msec_t					   read_timeout;

	size_t						   send_lowat;
} lxl_dfss_upstream_conf_t;

typedef void (*lxl_dfss_upstream_handler_pt) (lxl_dfss_request_t *r, lxl_dfss_upstream_t *u);
typedef int  (*lxl_dfss_upstream_body_handler_pt) (lxl_dfss_request_t *r, lxl_dfss_upstream_t *u);

struct lxl_dfss_upstream_s {
	off_t						  	   rest;

	lxl_dfss_upstream_handler_pt  	   read_event_handler;
	lxl_dfss_upstream_handler_pt  	   write_event_handler;
	lxl_dfss_upstream_body_handler_pt  body_handler;

	lxl_peer_connection_t 		  	   peer;

	lxl_dfss_upstream_conf_t   	 	  *conf;

	lxl_buf_t					      *header_buf;
	lxl_dfs_request_header_t 	  	   request_header;
	lxl_dfs_response_header_t     	   response_header;

	lxl_buf_t					  	   buffer;

	unsigned 						   request_send:1;
	
	//lxl_buf_t 		 	   	     	  *out;
/*	int		(*create_request)(lxl_dfss_request_t *r);
	int		(*reinit_request)(lxl_dfss_request_t *r);
	int		(*process_header)(lxl_dfss_request_t *r);
	void 	(*abort_request)(lxl_dfss_request_t *r);*/
};


//int 	lxl_dfss_upstream_create(lxl_dfss_request_t *r);
//void	lxl_dfss_upstream_init(lxl_dfss_request_t *r);

//void lxl_dfss_upstream_connect(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u);
//void lxl_dfss_upstream_next(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u, lxl_uint_t ft_type);
void 	lxl_dfss_upstream_init(lxl_dfss_request_t *r);
lxl_dfss_upstream_srv_conf_t *lxl_dfss_upstream_add(lxl_conf_t *cf, lxl_url_t *u, lxl_uint_t flags);

//void 	lxl_dfss_tracker_init(lxl_dfss_request_t *r);
void 	lxl_dfss_upstream_request_empty_handler(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u);
void	lxl_dfss_upstream_block_reading(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u);
void	lxl_dfss_upstream_parse_response(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u, lxl_buf_t *b);

extern lxl_module_t  lxl_dfss_upstream_module;


#endif	/* LXL_DFSS_UPSTREAM_H_INCLUDE */
