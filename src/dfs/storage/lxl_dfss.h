
/*
 * Copyright (C) xianliang.li
 */


#ifndef LXL_DFSS_H_INCLUDE
#define LXL_DFSS_H_INCLUDE


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_log.h>
#include <lxl_buf.h>
#include <lxl_inet.h>
#include <lxl_queue.h>
#include <lxl_hash.h>
#include <lxl_array.h>
#include <lxl_string.h>
#include <lxl_connection.h>
#include <lxl_conf_file.h>
#include <lxl_event.h>
#include <lxl_event_timer.h>
#include <lxl_os.h>


#define LXL_DFSS_DIR_COUNT			256
#define LXL_DFSS_CHUNK_SETINEL		2560000


typedef struct lxl_dfss_request_s	lxl_dfss_request_t;
typedef struct lxl_dfss_upstream_s 	lxl_dfss_upstream_t;


#include <lxl_dfs.h>
#include <lxl_dfss_core_module.h>
#include <lxl_dfss_request.h>
#include <lxl_dfss_upstream.h>
#include <lxl_dfss_upstream_round_robin.h>


void	lxl_dfss_server_start(void);

void	lxl_dfss_init_connection(lxl_connection_t *c);
void	lxl_dfss_report_state_handler(lxl_event_t *ev);
void	lxl_dfss_sync_fid_handler(lxl_event_t *ev);
void	lxl_dfss_sync_push_handler(lxl_event_t *ev);
void	lxl_dfss_sync_pull_handler(lxl_event_t *ev);
void 	lxl_dfss_report_fid_handler(lxl_event_t *ev);
void 	lxl_dfss_request_empty_handler(lxl_dfss_request_t *r);
//void 	lxl_dfss_upstream_request_empty_handler(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u);
//void 	lxl_dfss_close_connetion(lxl_connection_t *c);

lxl_dfss_request_t *lxl_dfss_create_request(lxl_connection_t *c);
void 	lxl_dfss_finalize_request(lxl_dfss_request_t *r, lxl_int_t rc);
//void	lxl_dfss_free_request(lxl_dfss_request_t *r);


//typedef void (*lxl_dfss_client_body_handler_pt) (lxl_dfss_request_t *r);
//typedef int	 (*lxl_dfss_body_handler_pt)		(lxl_dfss_request_t *r);

int		lxl_dfss_parse_request(lxl_dfss_request_t *r, char *data);
int 	lxl_dfss_read_client_request_body(lxl_dfss_request_t *r, lxl_dfss_client_body_handler_pt post_handler);
int		lxl_dfss_send_body(lxl_dfss_request_t *r, lxl_dfss_client_body_handler_pt post_handler);

//int		lxl_dfss_upstream_tracker_handler(lxl_dfss_request_t *r);
void	lxl_dfss_block_reading(lxl_dfss_request_t *r);

void	lxl_dfss_tracker_init(lxl_dfss_request_t *r);
void 	lxl_dfss_storage_init(lxl_dfss_request_t *r);
//void	lxl_dfss_block_reading(lxl_dfss_request_t *r);

void    lxl_dfss_update_system_state(void);


extern uint16_t			   lxl_dfss_area_id;
extern uint16_t			   lxl_dfss_group_id;
extern uint16_t			   lxl_dfss_idc_id;
extern uint16_t 		   lxl_dfss_loadavg5;
extern uint16_t  		   lxl_dfss_cpu_usage;
extern uint16_t			   lxl_dfss_cpu_idle;
extern uint32_t			   lxl_dfss_disk_free_mb;

extern uint16_t			   lxl_dfss_fid_seed;
extern uint32_t			   lxl_dfss_id;
extern uint32_t			   lxl_dfss_dir_seed;
extern uint32_t 		   lxl_dfss_file_seed;
extern uint32_t 		   lxl_dfss_request_seed;
extern lxl_uint_t 		   lxl_dfss_file_count;

extern char				   lxl_dfss_uid[24];

extern lxl_pool_t		  *lxl_dfss_temp_pool;
extern lxl_hash_t		   lxl_dfss_fid_phash;
extern lxl_hash1_t		   lxl_dfss_fid_hash;
extern lxl_list_t		   lxl_dfss_sync_pull_list;	/* restore data */
extern lxl_list_t 		   lxl_dfss_sync_push_list;
extern lxl_event_t		   lxl_dfss_report_state_event;
extern lxl_event_t         lxl_dfss_sync_fid_event;
extern lxl_event_t		   lxl_dfss_sync_push_event;
extern lxl_event_t		   lxl_dfss_sync_pull_event;
extern lxl_event_t 		   lxl_dfss_report_fid_event;
extern lxl_dfs_ip_port_t   lxl_dfss_ip_port;
extern lxl_dfss_cpu_use_t  lxl_dfss_cpu_use;


#endif /* LXL_DFSS_H_INCLUDE */
