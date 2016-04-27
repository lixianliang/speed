
/*
 * Copright (C) xianliang.li
 */


#include <lxl_dfss.h>


static char *lxl_dfss_block(lxl_conf_t *cf, lxl_command_t *cmd, void *conf);
static char *lxl_dfss_optimize_server(lxl_conf_t *cf, lxl_dfss_listen_t *listen);


uint16_t			lxl_dfss_area_id = 16; 	/* 0 - 255 */
uint16_t     	   	lxl_dfss_group_id = 1;
uint16_t	 		lxl_dfss_idc_id = 1;
uint16_t		   	lxl_dfss_loadavg5 = 0;
uint16_t		   	lxl_dfss_cpu_usage = 0;
uint16_t			lxl_dfss_cpu_idle = 90;
uint32_t		   	lxl_dfss_disk_free_mb = 0;

uint16_t			lxl_dfss_fid_seed = 0;	/* jia suo */
uint32_t     	   	lxl_dfss_id = 1;
uint32_t     	   	lxl_dfss_dir_seed = 0;
//uint32_t   	 	   	lxl_dfss_file_seed = 0;
uint32_t     	   	lxl_dfss_request_seed = 0;
lxl_uint_t 	 	   	lxl_dfss_file_count;
lxl_uint_t   	   	lxl_dfss_max_module;

char				lxl_dfss_uid[24];

lxl_pool_t		   *lxl_dfss_temp_pool;
lxl_hash_t 		lxl_dfss_fid_phash;
lxl_hash1_t			lxl_dfss_fid_hash;
lxl_list_t 			lxl_dfss_sync_push_list;
lxl_list_t			lxl_dfss_sync_pull_list;
lxl_event_t		    lxl_dfss_sync_fid_event;
lxl_event_t			lxl_dfss_sync_push_event;
lxl_event_t			lxl_dfss_sync_pull_event;
lxl_event_t 	   	lxl_dfss_report_state_event;
lxl_event_t		   	lxl_dfss_report_fid_event;
lxl_dfs_ip_port_t  	lxl_dfss_ip_port;
lxl_dfss_cpu_use_t 	lxl_dfss_cpu_use = { 0, 0, 0, 0, 0, 0, 0, 0 };


static lxl_command_t lxl_dfss_commands[] = {
	{ lxl_string("dfs_storage"),
	  LXL_MAIN_CONF|LXL_CONF_BLOCK|LXL_CONF_NOARGS,
	  lxl_dfss_block,
	  0,
      0,
	  NULL },
	
	lxl_null_command
};

static lxl_core_module_t lxl_dfss_module_ctx = {
	lxl_string("dfss"),
	NULL,
	NULL
};

lxl_module_t lxl_dfss_module = {
	0,
	0,
	(void *) &lxl_dfss_module_ctx,
	lxl_dfss_commands,
	LXL_CORE_MODULE,
	NULL,
	NULL
};


static char *
lxl_dfss_block(lxl_conf_t *cf, lxl_command_t *cmd, void *conf)
{
	char *rv;
	lxl_uint_t i, j, mi, nelts;
	lxl_conf_t pcf;
	lxl_dfss_module_t *module;
	lxl_dfss_conf_ctx_t *ctx;
	//lxl_dfss_core_srv_conf_t **cscfp;
	lxl_dfss_core_srv_conf_t  *cscf;
	lxl_dfss_core_main_conf_t *cmcf;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss block");
	if (*(void **) conf) {
		return "is duplicate";
	}

	ctx = lxl_pcalloc(cf->pool, sizeof(lxl_dfss_conf_ctx_t));
	if (ctx == NULL) {
		return LXL_CONF_ERROR;
	}

	*(lxl_dfss_conf_ctx_t **) conf = ctx;
	lxl_dfss_max_module = 0;
	for (i = 0; lxl_modules[i]; ++i) {
		if (lxl_modules[i]->type != LXL_DFSS_MODULE) {
			continue;
		}

		lxl_modules[i]->ctx_index = lxl_dfss_max_module++;
	}

	ctx->main_conf = lxl_pcalloc(cf->pool, lxl_dfss_max_module * sizeof(void *));
	if (ctx->main_conf == NULL) {
		return LXL_CONF_ERROR;
	}

	ctx->srv_conf = lxl_pcalloc(cf->pool, lxl_dfss_max_module * sizeof(void *));
	if (ctx->srv_conf == NULL) {
		return LXL_CONF_ERROR;
	}

	for (i = 0; lxl_modules[i]; ++i) {
		if (lxl_modules[i]->type != LXL_DFSS_MODULE) {
			continue;
		}

		module = lxl_modules[i]->ctx;
		mi = lxl_modules[i]->ctx_index;
		if (module->create_main_conf) {
			ctx->main_conf[mi] = module->create_main_conf(cf);
			if (ctx->main_conf[mi] == NULL) {
				return LXL_CONF_ERROR;
			}
		}

		if (module->create_srv_conf) {
			ctx->srv_conf[mi] = module->create_srv_conf(cf);
			if (ctx->srv_conf[mi] == NULL) {
				return LXL_CONF_ERROR;
			}
		}
	}

	pcf = *cf;
	cf->ctx = ctx;
	cf->module_type = LXL_DFSS_MODULE;
	cf->cmd_type = LXL_DFSS_MAIN_CONF;
	rv = lxl_conf_parse(cf, NULL);
	if (rv != LXL_CONF_OK) {
		*cf = pcf;
		return rv;
	}

	cmcf = ctx->main_conf[lxl_dfss_core_module.ctx_index];
	//cscfp = lxl_array_elts(&cmcf->servers);
	cscf = cmcf->server;
	for (i = 0; lxl_modules[i]; ++i) {
		if (lxl_modules[i]->type != LXL_DFSS_MODULE) {
			continue;
		}

		module = lxl_modules[i]->ctx;
		mi = lxl_modules[i]->ctx_index;
		cf->ctx = ctx;
		if (module->init_main_conf) {
			rv = module->init_main_conf(cf, ctx->main_conf[mi]);
			if (rv != LXL_CONF_OK) {
				*cf = pcf;
				return rv;
			}
		}

		/*nelts = lxl_array_nelts(&cmcf->servers);
		for (j = 0; j < nelts; ++j) {
			cf->ctx = cscfp[j]->ctx;
			if (module->merge_srv_conf) {
				rv = module->merge_srv_conf(cf, ctx->srv_conf[mi], cscfp[j]->ctx->srv_conf[mi]);
				if (rv != LXL_CONF_OK) {
					*cf = pcf;
					return rv;
				}
			}
		}*/
		if (module->init_srv_conf) {
			cf->ctx = cscf->ctx;
			rv = module->init_srv_conf(cf, cscf->ctx->srv_conf[mi]);
			if (rv != LXL_CONF_OK) {
				*cf = pcf;
				return rv;
			}
		} 
	}
		
	*cf = pcf;
		
	if (lxl_dfss_optimize_server(cf, cmcf->listen) != 0) {
		return LXL_CONF_ERROR;
	}

	return LXL_CONF_OK;
}

static char * 
lxl_dfss_optimize_server(lxl_conf_t *cf, lxl_dfss_listen_t *listen)
{
	lxl_uint_t 			 i, nelts;
	lxl_listening_t     *ls;
	lxl_dfss_port_t     *port;
	struct sockaddr_in 	*sin;

	ls = lxl_create_listening(cf, listen->sockaddr, listen->socklen, SOCK_STREAM);
	if (ls == NULL) {
		return LXL_CONF_ERROR;
	}

	ls->handler = lxl_dfss_init_connection;
	ls->pool_size = 256;

	port = lxl_pcalloc(cf->pool, sizeof(lxl_dfss_port_t));
	if (port == NULL) {
		return LXL_CONF_ERROR;
	}

	sin = (struct sockaddr_in *) ls->sockaddr;

	port->addr.addr =  sin->sin_addr.s_addr;
	port->addr.ctx = listen->ctx;
	ls->servers = port;
	
	return LXL_CONF_OK;
}
