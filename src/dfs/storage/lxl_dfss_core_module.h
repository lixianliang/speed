
/*
 * Copyright (C) xianliang.li
 */


#ifndef	LXL_DFSS_CORE_MODULE_H_INCLUDE
#define LXL_DFSS_CORE_MODULE_H_INCLUDE


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_dfss.h>


#define LXL_DFSS_MODULE				0x44465353	/* DFSS */
#define LXL_DFSS_MAIN_CONF			0x02000000
#define LXL_DFSS_SRV_CONF			0x04000000
#define LXL_DFSS_UPS_CONF			0x10000000

#define LXL_DFSS_MAIN_CONF_OFFSET	offsetof(lxl_dfss_conf_ctx_t, main_conf)
#define LXL_DFSS_SRV_CONF_OFFSET	offsetof(lxl_dfss_conf_ctx_t, srv_conf)


typedef struct {
	size_t		size;
	lxl_str_t	name;
} lxl_dfss_fid_value_t;

typedef struct {
	int  					   use1;
	int  					   use2;
	int  					   use3;
	int  					   use4;
	int  					   use5;
	int  					   use6;
	int  					   use7;

	int 					   total;
} lxl_dfss_cpu_use_t;

typedef struct {
	uint16_t  	idc_id;
	lxl_addr_t  addr;
} lxl_dfss_addr_t;

typedef struct {
	void					 **main_conf;
	void					 **srv_conf;
} lxl_dfss_conf_ctx_t;

typedef struct {
	char 				   	   sockaddr[LXL_SOCKADDRLEN];
	socklen_t             	   socklen;

	lxl_dfss_conf_ctx_t	      *ctx;

	unsigned 			  	   wildcard:1;
} lxl_dfss_listen_t;

typedef struct {
	in_addr_t			  	   addr;
	lxl_dfss_conf_ctx_t	 	  *ctx;
} lxl_dfss_in_addr_t;

typedef struct {
	lxl_dfss_in_addr_t	  	   addr;
} lxl_dfss_port_t;

typedef struct {
	lxl_dfss_conf_ctx_t 	  *ctx;
	//lxl_dfss_handler_t  handler;  upstream handler
} lxl_dfss_core_srv_conf_t;

typedef struct {
	lxl_dfss_core_srv_conf_t  *server;
	lxl_dfss_listen_t   	  *listen;
} lxl_dfss_core_main_conf_t;

typedef struct {
//	lxl_str_t name;
	void 	*(*create_main_conf)	(lxl_conf_t *cf);
	char 	*(*init_main_conf)		(lxl_conf_t *cf, void *conf);
	void 	*(*create_srv_conf)		(lxl_conf_t *cf);
	char 	*(*init_srv_conf)		(lxl_conf_t *cf, void *conf);
} lxl_dfss_module_t;


extern lxl_module_t  lxl_dfss_core_module;
extern lxl_uint_t    lxl_dfss_max_module;


#define lxl_dfss_get_module_main_conf(r, module)	(r)->main_conf[module.ctx_index]
#define lxl_dfss_get_module_srv_conf(r, module)		(r)->srv_conf[module.ctx_index]

#define lxl_dfss_conf_get_module_main_conf(cf, module)					\
	((lxl_dfss_conf_ctx_t *) (cf)->ctx)->main_conf[module.ctx_index]


#endif	/* LXL_DFSS_CORE_MODULE_H_INCLUDE */
