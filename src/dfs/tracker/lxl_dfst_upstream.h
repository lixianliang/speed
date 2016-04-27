
/*
 * Copyright (C) xianliang.li
 */


#ifndef LXL_DFST_UPSTREAM_H_INCLUDE
#define LXL_DFST_UPSTREAM_H_INCLUDE


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_dfst.h>


#define LXL_DFST_UPSTREAM_FT_ERROR				600
#define LXL_DFST_UPSTREAM_FT_TIMEOUT			601
#define LXL_DFST_UPSTREAM_FT_NOLIVE				603
#define LXL_DFST_UPSTREAM_FT_INVALID_RESPONSE 	604
#define LXL_DFST_UPSTREAM_FT_INVALID_PACKAGE	605

#define LXL_DFST_UPSTREAM_FT_TRY_OVER			610

#define LXL_DFST_UPSTREAM_CREATE				0x0001
#define LXL_DFST_UPSTREAM_TRACKER				0x0010
#define LXL_DFST_UPSTREAM_STORAGE				0x0020


typedef struct lxl_dfst_upstream_srv_conf_s lxl_dfst_upstream_srv_conf_t;
typedef int	(*lxl_dfst_upstream_init_pt) (lxl_conf_t *conf, lxl_dfst_upstream_srv_conf_t *us);
typedef int (*lxl_dfst_upstream_init_peer_pt) (lxl_dfst_request_t *r, lxl_dfst_upstream_srv_conf_t *us, lxl_dfst_upstream_t *u);

typedef struct {
	lxl_array_t	 upstreams;
} lxl_dfst_upstream_main_conf_t;

typedef struct {
	lxl_dfst_upstream_init_pt		init_upstream;
	lxl_dfst_upstream_init_peer_pt	init;
	void						   *data;
} lxl_dfst_upstream_peer_t;

typedef struct {
	char 		sockaddr[LXL_SOCKADDRLEN];
	socklen_t	socklen;
	lxl_uint_t	max_fails;
	time_t		fail_timeout;
	lxl_uint_t	down;
	char	   *name;
} lxl_dfst_upstream_server_t;

struct lxl_dfst_upstream_srv_conf_s {
	lxl_dfst_upstream_peer_t  peer;
	void					**srv_conf;
	lxl_array_t			 *servers;		/* lxl_dfst_upstream_server_t */
	
	lxl_uint_t				  flags;
	lxl_str_t				  host;
	in_port_t				  port;
	in_port_t				  default_port;
	lxl_uint_t				  no_port;
};

typedef struct {
	lxl_dfst_upstream_srv_conf_t  *upstream;

	lxl_msec_t					   connect_timeout;
	lxl_msec_t					   send_timeout;
	lxl_msec_t					   read_timeout;
	
	size_t						   send_lowat;
} lxl_dfst_upstream_conf_t;

typedef void (*lxl_dfst_upstream_handler_pt) (lxl_dfst_request_t *r, lxl_dfst_upstream_t *u);

struct lxl_dfst_upstream_s {
	lxl_dfst_upstream_handler_pt  read_event_handler;
	lxl_dfst_upstream_handler_pt  write_event_handler;
	
	lxl_peer_connection_t		  peer;
	
	lxl_dfst_upstream_conf_t	 *conf;

	lxl_dfs_request_header_t	  request_header;
	lxl_dfs_response_header_t	  response_header;

//	lxl_buf_t					  buffer;
//	lxl_buf_t					 *out;
//
	lxl_buf_t					 *request_buf;
	lxl_buf_t					 *response_buf;

	lxl_queue_t 				  addrs;	/* tracker lxl_addr_t */

	lxl_uint_t					  tries;
	//lxl_uint_t					  number;		/* success of successful */
};


void	lxl_dfst_upstream_init(lxl_dfst_request_t *r);
lxl_dfst_upstream_srv_conf_t *lxl_dfst_upstream_add(lxl_conf_t *cf, lxl_url_t *u, lxl_uint_t flags);


extern lxl_module_t  lxl_dfst_upstream_module;


#endif	/* LXL_DFST_UPSTREAM_H_INCLUDE */
