
/*
 * Copright (C) xianliang.li
 */


#include <lxl_dfst.h>


lxl_hash1_t	lxl_dfst_fid_hash;
lxl_list_t  lxl_dfst_group_list;
lxl_list_t	lxl_dfst_idc_list;


static char *lxl_dfst_block(lxl_conf_t *cf, lxl_command_t *cmd, void *conf);
static int	 lxl_dfst_optimize_server(lxl_conf_t *cf, lxl_dfst_listen_t *listen);


uint32_t	lxl_dfst_request_seed = 0;
lxl_uint_t  lxl_dfst_max_module;


static lxl_command_t lxl_dfst_commands[] = {
	{ lxl_string("dfs_tracker"),
	  LXL_MAIN_CONF|LXL_CONF_BLOCK|LXL_CONF_NOARGS,
	  lxl_dfst_block,
	  0,
      0,
	  NULL },
	
	lxl_null_command
};

static lxl_core_module_t lxl_dfst_module_ctx = {
	lxl_string("dfst"),
	NULL,
	NULL
};

lxl_module_t lxl_dfst_module = {
	0,
	0,
	(void *) &lxl_dfst_module_ctx,
	lxl_dfst_commands,
	LXL_CORE_MODULE,
	NULL,
	NULL
};


static char *
lxl_dfst_block(lxl_conf_t *cf, lxl_command_t *cmd, void *conf)
{
	char 					   *rv;
	lxl_uint_t 					i, j, mi, nelts;
	lxl_conf_t 					pcf;
	lxl_dfst_module_t 		   *module;
	lxl_dfst_conf_ctx_t *ctx;
	lxl_dfst_core_srv_conf_t   *cscf, **cscfp;
	lxl_dfst_core_main_conf_t  *cmcf;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst block");
	if (*(void **) conf) {
		return "is duplicate";
	}

	ctx = lxl_pcalloc(cf->pool, sizeof(lxl_dfst_conf_ctx_t));
	if (ctx == NULL) {
		return LXL_CONF_ERROR;
	}

	*(lxl_dfst_conf_ctx_t **) conf = ctx;
	lxl_dfst_max_module = 0;
	for (i = 0; lxl_modules[i]; ++i) {
		if (lxl_modules[i]->type != LXL_DFST_MODULE) {
			continue;
		}

		lxl_modules[i]->ctx_index = lxl_dfst_max_module++;
	}

	ctx->main_conf = lxl_pcalloc(cf->pool, lxl_dfst_max_module * sizeof(void *));
	if (ctx->main_conf == NULL) {
		return LXL_CONF_ERROR;
	}

	ctx->srv_conf = lxl_pcalloc(cf->pool, lxl_dfst_max_module * sizeof(void *));
	if (ctx->srv_conf == NULL) {
		return LXL_CONF_ERROR;
	}

	for (i = 0; lxl_modules[i]; ++i) {
		if (lxl_modules[i]->type != LXL_DFST_MODULE) {
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
	cf->module_type = LXL_DFST_MODULE;
	cf->cmd_type = LXL_DFST_MAIN_CONF;
	rv = lxl_conf_parse(cf, NULL);
	if (rv != LXL_CONF_OK) {
		*cf = pcf;
		return rv;
	}

	cmcf = ctx->main_conf[lxl_dfst_core_module.ctx_index];
	//cscfp = lxl_array_elts(&cmcf->servers);
	cscf = cmcf->server;
	for (i = 0; lxl_modules[i]; ++i) {
		if (lxl_modules[i]->type != LXL_DFST_MODULE) {
			continue;
		}

		module = lxl_modules[i]->ctx;
		mi = lxl_modules[i]->ctx_index;
		if (module->init_main_conf) {
			rv = module->init_main_conf(cf, ctx->main_conf[mi]);
			if (rv != LXL_CONF_OK) {
				*cf = pcf;
				return rv;
			}
		}

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
	
	if (lxl_dfst_optimize_server(cf, cmcf->listen) != 0) {
		return LXL_CONF_ERROR;
	}

	return LXL_CONF_OK;
}

static int	 
lxl_dfst_optimize_server(lxl_conf_t *cf, lxl_dfst_listen_t *listen)
{
	lxl_uint_t 			 i, nelts;
	lxl_listening_t		*ls;
	lxl_dfst_port_t	 	*port;
	struct sockaddr_in  *sin;

	ls = lxl_create_listening(cf, listen->sockaddr, listen->socklen, SOCK_STREAM);
	if (ls == NULL) {
		return -1;
	}

	ls->handler = lxl_dfst_init_connection;
	ls->pool_size = 256;
	
	port = lxl_pcalloc(cf->pool, sizeof(lxl_dfst_port_t));
	if (port == NULL) {
		return -1;
	}

	sin = (struct sockaddr_in *) ls->sockaddr;
	port->addr.addr = sin->sin_addr.s_addr;
	port->addr.ctx = listen->ctx;
	ls->servers = port;

	ls = lxl_create_listening(cf, listen->sockaddr, listen->socklen, SOCK_DGRAM);
	if (ls == NULL) {
		return -1;
	}

	ls->handler = lxl_dfst_init_udp_connection;
	ls->pool_size = 256;
	
	port = lxl_pcalloc(cf->pool, sizeof(lxl_dfst_port_t));
	if (port == NULL) {	
		return -1;
	}

	sin = (struct sockaddr_in *) ls->sockaddr;
	port->addr.addr = sin->sin_addr.s_addr;
	port->addr.ctx = listen->ctx;
	ls->servers = port;
	
	return 0;
}
