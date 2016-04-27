
/*
 * Copyright (C) xianliang.li
 */


#ifndef	LXL_DFST_CORE_MODULE_H_INCLUDE
#define LXL_DFST_CORE_MODULE_H_INCLUDE


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_dfst.h>


#define LXL_DFST_MODULE				0x44465354	/* DFST */
#define LXL_DFST_MAIN_CONF			0x02000000
#define LXL_DFST_SRV_CONF			0x04000000
#define LXL_DFST_UPS_CONF			0x10000000

#define LXL_DFST_MAIN_CONF_OFFSSET	offsetof(lxl_dfst_conf_ctx_t, main_conf)
#define LXL_DFST_SRV_CONF_OFFSET	offsetof(lxl_dfst_conf_ctx_t, srv_conf)


typedef struct {
	void	**main_conf;
	void 	**srv_conf;
} lxl_dfst_conf_ctx_t;

typedef struct {
	char				  sockaddr[LXL_SOCKADDRLEN];
	socklen_t			  socklen;
	
	lxl_dfst_conf_ctx_t  *ctx;

	unsigned 			  wildcard:1;
} lxl_dfst_listen_t;

typedef struct {
	in_addr_t			  addr;
	lxl_dfst_conf_ctx_t  *ctx;
} lxl_dfst_in_addr_t;

typedef struct {
	lxl_dfst_in_addr_t  addr;
} lxl_dfst_port_t;

typedef struct {
	lxl_dfst_conf_ctx_t  *ctx;
} lxl_dfst_core_srv_conf_t;

typedef struct {
	lxl_dfst_core_srv_conf_t  *server;
	lxl_dfst_listen_t 		  *listen;
}lxl_dfst_core_main_conf_t;

typedef struct {
	void 	*(*create_main_conf)	(lxl_conf_t *cf);
	char	*(*init_main_conf)		(lxl_conf_t *cf, void *conf);
	void	*(*create_srv_conf)		(lxl_conf_t *cf);
	char 	*(*init_srv_conf)		(lxl_conf_t *cf, void *conf);
} lxl_dfst_module_t;


extern lxl_module_t	 lxl_dfst_core_module;
extern lxl_uint_t	 lxl_dfst_max_module;


#define lxl_dfst_get_module_main_conf(r, module)	(r)->main_conf[module.ctx_index]
#define lxl_dfst_get_module_srv_conf(r, module)		(r)->srv_conf[module.ctx_index]

#define lxl_dfst_conf_get_module_main_conf(cf, module)					\
	((lxl_dfst_conf_ctx_t *) (cf)->ctx)->main_conf[module.ctx_index]


#endif	/* LXL_DFST_CORE_MODULE_H_INCLUDE */
