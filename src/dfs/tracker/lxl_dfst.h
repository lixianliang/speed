
/*
 * Copyright (C) xianliang.li
 */


#ifndef LXL_DFST_H_INCLUDE
#define LXL_DFST_H_INCLUDE


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_inet.h>
#include <lxl_hash.h>
#include <lxl_array.h>
#include <lxl_queue.h>
#include <lxl_string.h>
#include <lxl_log.h>
#include <lxl_buf.h>
#include <lxl_connection.h>
#include <lxl_conf_file.h>
#include <lxl_event.h>
#include <lxl_event_timer.h>
#include <lxl_event_connect.h>


typedef struct lxl_dfst_request_s   lxl_dfst_request_t;
typedef struct lxl_dfst_upstream_s  lxl_dfst_upstream_t;


#include <lxl_dfs.h>
#include <lxl_dfst_core_module.h>
#include <lxl_dfst_request.h>
#include <lxl_dfst_data.h>
//#include <lxl_dfst_idc.h>
#include <lxl_dfst_upstream.h>
#include <lxl_dfst_upstream_round_robin.h>


void	lxl_dfst_init_connection(lxl_connection_t *c);
void 	lxl_dfst_init_udp_connection(lxl_connection_t *c);

void	lxl_dfst_finalize_request(lxl_dfst_request_t *r, lxl_int_t rc);

void 	lxl_dfst_request_empty_handler(lxl_dfst_request_t *r);
void	lxl_dfst_block_reading(lxl_dfst_request_t *r);

int		lxl_dfst_parse_request(lxl_dfst_request_t *r);
int		lxl_dfst_read_client_request_body(lxl_dfst_request_t *r, lxl_dfst_client_body_handler_pt post_handler);
void	lxl_dfst_tracker_init(lxl_dfst_request_t *r);


extern uint32_t	   	 lxl_dfst_request_seed;

extern lxl_hash1_t   lxl_dfst_fid_hash;
extern lxl_list_t  	 lxl_dfst_group_list;
extern lxl_list_t  	 lxl_dfst_idc_list;
extern lxl_array1_t  lxl_dfst_idc_array;
extern lxl_file_t	 lxl_dfst_storages_binlog_file;
extern lxl_file_t 	 lxl_dfst_fids_binlog_file;



#endif /* LXL_DFST_H_INCLUDE */
