
/*
 * Copyright (C) xianliang.li
 */


#ifndef LXL_DFST_UPSTREAM_ROUND_ROBIN_H_INCLUDE
#define LXL_DFST_UPSTREAM_ROUND_ROBIN_H_INCLUDE


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_dfst.h>


typedef struct {
	struct sockaddr  *sockaddr;
	socklen_t		  socklen;
	char 			 *name;

	lxl_uint_t		  fails;
	lxl_uint_t 		  max_fails;
	lxl_uint_t		  down;
	time_t			  accessed;
	time_t			  checked;
	time_t			  fail_timeout;
} lxl_dfst_upstream_rr_peer_t;

typedef struct {
	lxl_uint_t					 number;
	unsigned					 single:1;

	lxl_dfst_upstream_rr_peer_t  peer[1];
} lxl_dfst_upstream_rr_peers_t;

typedef struct {
	lxl_dfst_upstream_rr_peers_t  *peers;
	lxl_uint_t					   current;
	lxl_uint_t					   tries;
	lxl_uint_t					   data;	
} lxl_dfst_upstream_rr_peer_data_t;


int		lxl_dfst_upstream_init_round_robin_storage(lxl_dfst_request_t *r, lxl_dfst_upstream_srv_conf_t *us);
int		lxl_dfst_upstream_init_round_robin_peer(lxl_dfst_request_t *r, lxl_dfst_upstream_srv_conf_t *us, lxl_dfst_upstream_t *u);
int		lxl_dfst_upstream_get_round_robin_peer(lxl_peer_connection_t *pc, void *data);
void 	lxl_dfst_upstream_free_round_robin_peer(lxl_peer_connection_t *pc, void *data, lxl_uint_t state);


#endif	/* LXL_DFST_UPSTREAM_ROUND_ROBIN_H_INCLUDE */
