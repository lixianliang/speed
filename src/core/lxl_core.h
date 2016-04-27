
/*
 * Copyright (C) xianliang.li
 */


#ifndef LXL_CORE_H_INCLUDE
#define LXL_CORE_H_INCLUDE


#define LXL_PREFIX   		""
#define LXL_CONF_PATH		"conf/speed.conf"


#define LXL_OK 				0
#define LXL_ERROR 			-1
#define LXL_EAGAIN 			-2
#define LXL_BUSY			-3
#define LXL_DONE			-4
#define LXL_DECLINED		-5


typedef struct lxl_module_s				lxl_module_t;
typedef struct lxl_command_s			lxl_command_t;
typedef struct lxl_conf_s				lxl_conf_t;
typedef struct lxl_log_s 				lxl_log_t;
typedef struct lxl_pool_s   			lxl_pool_t;
typedef struct lxl_file_s				lxl_file_t;
typedef struct lxl_chain_s				lxl_chain_t;
typedef struct lxl_event_s 				lxl_event_t;
typedef struct lxl_connection_s 		lxl_connection_t;
//typedef struct lxl_peer_connection_s    lxl_peer_connection_t;
typedef struct lxl_cycle_s				lxl_cycle_t;


#include <lxl_string.h>
#include <lxl_file.h>
#include <lxl_palloc.h>
#include <lxl_buf.h>
#include <lxl_alloc.h>
#include <lxl_array.h>
#include <lxl_conf_file.h>
#include <lxl_log.h>
#include <lxl_list.h>
#include <lxl_slist.h>
#include <lxl_queue.h>
#include <lxl_stack.h>
#include <lxl_rbtree.h>
#include <lxl_hash.h>
#include <lxl_inet.h>
#include <lxl_times.h>
#include <lxl_cycle.h>
#include <lxl_connection.h>
#include <lxl_os.h>
//#include <lxl_util.h>
#include <lxl_files.h>
#include <lxl_socket.h>
#include <lxl_process.h>


#define LF					10
#define CR					13			
#define CRLF				"\x0d\x0a"

#define lxl_abs(value)       (((value) >= 0) ? (value) : - (value))
#define lxl_max(val1, val2)  ((val1 < val2) ? (val2) : (val1))
#define lxl_min(val1, val2)  ((val1 > val2) ? (val2) : (val1))


#endif	/* LXL_CORE_H_INCLUDE */
