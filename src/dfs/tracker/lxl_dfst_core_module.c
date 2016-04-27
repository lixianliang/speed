
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_dfst.h>


static void *	lxl_dfst_create_main_conf(lxl_conf_t *cf);
static char *	lxl_dfst_init_main_conf(lxl_conf_t *cf, void *conf);
static void *	lxl_dfst_create_srv_conf(lxl_conf_t *cf);
static char *	lxl_dfst_init_srv_conf(lxl_conf_t *cf, void *conf);
static char *	lxl_dfst_core_server(lxl_conf_t *cf, lxl_command_t *cmd, void *conf);
static char *	lxl_dfst_core_listen(lxl_conf_t *cf, lxl_command_t *cmd, void *conf);

static int		lxl_dfst_core_init(lxl_cycle_t *cycle);


static lxl_command_t lxl_dfst_core_commands[] = {
	
	{ lxl_string("server"),
  	  LXL_DFST_MAIN_CONF|LXL_CONF_BLOCK|LXL_CONF_NOARGS,
	  lxl_dfst_core_server,
	  0,
	  0,
	  NULL },

	{ lxl_string("listen"),
	  LXL_DFST_SRV_CONF|LXL_CONF_TAKE1,
	  lxl_dfst_core_listen,
	  0,
	  0,
	  NULL },

	lxl_null_command
};

static lxl_dfst_module_t lxl_dfst_core_module_ctx = {
	lxl_dfst_create_main_conf,
	lxl_dfst_init_main_conf,
	lxl_dfst_create_srv_conf,
	lxl_dfst_init_srv_conf
};

lxl_module_t lxl_dfst_core_module = {
	0,
	0,
	(void *) &lxl_dfst_core_module_ctx,
	lxl_dfst_core_commands,
	LXL_DFST_MODULE,
	NULL,
	lxl_dfst_core_init
};


static void *
lxl_dfst_create_main_conf(lxl_conf_t *cf)
{
	lxl_dfst_core_main_conf_t *cmcf;

	cmcf = lxl_pcalloc(cf->pool, sizeof(lxl_dfst_core_main_conf_t));
	if (cmcf == NULL) {
		return NULL;
	}

	return cmcf;
}

static char *
lxl_dfst_init_main_conf(lxl_conf_t *cf, void *conf) 
{
//	lxl_dfst_core_main_conf_t *cmcf = conf;

	return LXL_CONF_OK;
}

static void *
lxl_dfst_create_srv_conf(lxl_conf_t *cf)
{
	lxl_dfst_core_srv_conf_t  *cscf;

	cscf = lxl_pcalloc(cf->pool, sizeof(lxl_dfst_core_srv_conf_t));
	if (cscf == NULL) {
		return NULL;
	}

	return cscf;
}

static char *
lxl_dfst_init_srv_conf(lxl_conf_t *cf, void *data)
{
//	lxl_dfst_core_srv_conf_t *prev = parent;
//	lxl_dfst_core_srv_conf_t *conf = child;

	return LXL_CONF_OK;
}

static char *
lxl_dfst_core_server(lxl_conf_t *cf, lxl_command_t *cmd, void *conf)
{
	char 					   *rv;
	void					   *mconf;
	lxl_uint_t				    i;
	lxl_conf_t				    pcf;
	lxl_dfst_module_t		   *module;
	lxl_dfst_conf_ctx_t		   *ctx, *main_ctx;
	lxl_dfst_core_srv_conf_t   *cscf, **cscfp;
	lxl_dfst_core_main_conf_t  *cmcf;

	main_ctx = cf->ctx;
	cmcf = main_ctx->main_conf[lxl_dfst_core_module.ctx_index];

	if (cmcf->server != NULL) {
		return "is duplicate";
	}

	ctx = lxl_pcalloc(cf->pool, sizeof(lxl_dfst_conf_ctx_t));
	if (ctx == NULL) {
		return LXL_CONF_ERROR;
	}

	ctx->main_conf = main_ctx->main_conf;
	ctx->srv_conf = lxl_pcalloc(cf->pool, lxl_dfst_max_module * sizeof(void *));
	if (ctx->srv_conf == NULL) {
		return LXL_CONF_ERROR;
	}

	for (i = 0; lxl_modules[i]; ++i) {
		if (lxl_modules[i]->type != LXL_DFST_MODULE) {
			continue;
		}

		module = lxl_modules[i]->ctx;
		if (module->create_srv_conf) {
			mconf = module->create_srv_conf(cf);
			if (mconf == NULL) {
				return LXL_CONF_ERROR;
			}

			ctx->srv_conf[lxl_modules[i]->ctx_index] = mconf;
		}
	}

	cscf = ctx->srv_conf[lxl_dfst_core_module.ctx_index];
	cscf->ctx = ctx;
	cmcf->server = cscf;

	pcf = *cf;
	cf->ctx = ctx;
	cf->cmd_type = LXL_DFST_SRV_CONF;
	rv = lxl_conf_parse(cf, NULL);
	*cf = pcf;

	return rv;
}

static char *
lxl_dfst_core_listen(lxl_conf_t *cf, lxl_command_t *cmd, void *conf)
{
	lxl_url_t					u;
	lxl_str_t				   *value;
	lxl_dfst_listen_t		   *ls;
	lxl_dfst_core_main_conf_t  *cmcf;
	
	cmcf = lxl_dfst_conf_get_module_main_conf(cf, lxl_dfst_core_module);
	if (cmcf->listen != NULL) {
		return "is duplicate";
	}

	value = lxl_array_elts(cf->args);
	u.url = value[1];
	u.listen = 1;
	if (lxl_parse_url(cf->pool, &u) != 0) {
		if (u.err) {
			lxl_conf_log_error(LXL_LOG_EMERG, cf, 0, "%s in \"%s\" of the \"listen\" directive", u.err, u.url.data);
		}

		return LXL_CONF_ERROR;
	}

	ls = lxl_pcalloc(cf->pool, sizeof(lxl_dfst_listen_t));
	if (ls == NULL) {
		return LXL_CONF_ERROR;
	}

	memcpy(ls->sockaddr, u.sockaddr, u.socklen);
	ls->socklen = u.socklen;
	ls->wildcard = u.wildcard;
	ls->ctx = cf->ctx;
	cmcf->listen = ls;

	return LXL_CONF_OK;
}

static int
lxl_dfst_core_init(lxl_cycle_t *cycle)
{
	//lxl_list_init(&lxl_dfst_group_list);
	lxl_list_init(&lxl_dfst_idc_list);
	if (lxl_array1_init(&lxl_dfst_idc_array, 4, sizeof(lxl_dfst_idc_t)) == -1) {
		return -1;
	}

	if (lxl_hash1_init(&lxl_dfst_fid_hash, 10000, lxl_hash_key) != 0) {
		lxl_log_error(LXL_LOG_ERROR, 0, "dfst lxl_hash1_init() failed");
		return -1;
	}

	if (lxl_dfst_storages_init() == -1) {
		lxl_log_error(LXL_LOG_ERROR, 0, "dfst storages_binlog init failed");
		return -1;
	}

	if (lxl_dfst_fids_init() ==-1) {
		lxl_log_error(LXL_LOG_ERROR, 0, "dfst fids binlog init failed");
		return -1;
	}

	return 0;
}
