
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_dfst.h>


static char *lxl_dfst_upstream(lxl_conf_t *cf, lxl_command_t *cmd, void *dummy);
static char *lxl_dfst_upstream_server(lxl_conf_t *cf, lxl_command_t *cmd, void *conf);
static void *lxl_dfst_upstream_create_main_conf(lxl_conf_t *cf);
static char *lxl_dfst_upstream_init_main_conf(lxl_conf_t *cf, void *conf);


static lxl_command_t  lxl_dfst_upstream_commands[] = {
	
	{ lxl_string("upstream"),
	  LXL_DFST_MAIN_CONF|LXL_CONF_BLOCK|LXL_CONF_TAKE1,
	  lxl_dfst_upstream,
	  0,
	  0,
	  NULL },

	{ lxl_string("server"),
	  LXL_DFST_UPS_CONF|LXL_CONF_1MORE,
	  lxl_dfst_upstream_server,
  	  LXL_DFST_SRV_CONF_OFFSET,
	  0,
	  NULL },

	  lxl_null_command
};

static lxl_dfst_module_t  lxl_dfst_upstream_module_ctx = {
	lxl_dfst_upstream_create_main_conf,
	lxl_dfst_upstream_init_main_conf,
	NULL,
	NULL
};

lxl_module_t  lxl_dfst_upstream_module = {
	0,
	0,
	(void *) &lxl_dfst_upstream_module_ctx,
	lxl_dfst_upstream_commands,
	LXL_DFST_MODULE,
	NULL,
	NULL
};

static char *
lxl_dfst_upstream(lxl_conf_t *cf, lxl_command_t *cmd, void *dummy)
{
	char 						  *rv;
	void						  *mconf;
	lxl_uint_t					   i;
	lxl_url_t					   u;
	lxl_str_t					  *value;
	lxl_conf_t					   pcf;
	lxl_dfst_module_t			  *module;
	lxl_dfst_conf_ctx_t		      *ctx, *dfst_ctx;
	lxl_dfst_upstream_srv_conf_t  *uscf, **uscfp;

	memset(&u, 0x00, sizeof(lxl_url_t));
	value = lxl_array_elts(cf->args);
	u.host = value[1];
	u.no_port = 1;

	uscf = lxl_dfst_upstream_add(cf, &u, LXL_DFST_UPSTREAM_CREATE);
	if (uscf == NULL) {
		return LXL_CONF_ERROR;
	}

	ctx = lxl_pcalloc(cf->pool, sizeof(lxl_dfst_conf_ctx_t));
	if (ctx == NULL) {
		return LXL_CONF_ERROR;
	}

	dfst_ctx = cf->ctx;
	ctx->main_conf = dfst_ctx->main_conf;

	ctx->srv_conf = lxl_pcalloc(cf->pool, sizeof(void *) * lxl_dfst_max_module);
	if (ctx->srv_conf == NULL) {
		return NULL;
	}

	ctx->srv_conf[lxl_dfst_upstream_module.ctx_index] = uscf;
	uscf->srv_conf = ctx->srv_conf;
	
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

	pcf = *cf;
	cf->ctx = ctx;
	cf->cmd_type = LXL_DFST_UPS_CONF;
	rv = lxl_conf_parse(cf, NULL);
	*cf = pcf;
	if (rv != LXL_CONF_OK) {
		return rv;
	}

	if (uscf->servers == NULL) {
		lxl_conf_log_error(LXL_LOG_EMERG, cf, 0, "no servers are inside upstream");
		return LXL_CONF_ERROR;
	}

	return LXL_CONF_OK;
}

static char *
lxl_dfst_upstream_server(lxl_conf_t *cf, lxl_command_t *cmd, void *conf)
{
	lxl_dfst_upstream_srv_conf_t  *uscf = conf;

	char 				   		 name[64];
	size_t						 len;
	off_t						 off;
	lxl_uint_t					 i, nelts;
	lxl_str_t					*value;
	lxl_url_t					 u;
	in_port_t					 port;
	struct sockaddr				*sa;
	struct sockaddr_in			*sin;
	lxl_dfst_upstream_server_t  *s, *us;

	if (uscf->servers == NULL) {
		uscf->servers = lxl_array_create(cf->pool, 4, sizeof(lxl_dfst_upstream_server_t));
		if (uscf->servers == NULL) {
			return LXL_CONF_ERROR;
		}
	}

	value = lxl_array_elts(cf->args);
	memset(&u, 0x00, sizeof(lxl_url_t));
	u.url = value[1];
	u.default_port = 1304;

	if (lxl_parse_url(cf->pool, &u) != 0) {
		if (u.err) {
			lxl_conf_log_error(LXL_LOG_EMERG, cf, 0, "%s in upstream %s", u.err, &u.url);
		}

		return LXL_CONF_ERROR;
	}

	s = lxl_array_elts(uscf->servers);
	nelts = lxl_array_nelts(uscf->servers);
	for (i = 0; i < nelts; ++i) {
		sa = (struct sockaddr *) s[i].sockaddr;
		off = offsetof(struct sockaddr_in, sin_addr);
		sin = (struct sockaddr_in *) sa;
		port = sin->sin_port;
		if (memcmp(s[i].sockaddr + off, u.sockaddr + off, 4) != 0) {
			continue;
		}

		if (port != u.port) {
			continue;
		}

		lxl_conf_log_error(LXL_LOG_EMERG, cf, 0, "duplicate %s address and port again", &u.url);
		return LXL_CONF_ERROR;
	}

	sin = (struct sockaddr *) u.sockaddr;
	len = (size_t) snprintf(name, 64, "%s:%d", inet_ntoa(sin->sin_addr), ntohs(sin->sin_port));
	us = lxl_array_push(uscf->servers);
	if (us == NULL) {
		return LXL_CONF_ERROR;
	}

	memset(us, 0x00, sizeof(lxl_dfst_upstream_server_t));
	memcpy(us->sockaddr, u.sockaddr, u.socklen);
	us->socklen = u.socklen;
	us->fail_timeout = 10;

	us->name = lxl_palloc(cf->pool, len + 1);
	if (us->name == NULL) {
		return LXL_CONF_ERROR;
	}

	memcpy(us->name, name, len + 1);

	return LXL_CONF_OK;
}

lxl_dfst_upstream_srv_conf_t *
lxl_dfst_upstream_add(lxl_conf_t *cf, lxl_url_t *u, lxl_uint_t flags)
{
	lxl_uint_t						i,nelts;
	lxl_dfst_upstream_srv_conf_t   *uscf, **uscfp;
	lxl_dfst_upstream_main_conf_t  *umcf;

	umcf = lxl_dfst_conf_get_module_main_conf(cf, lxl_dfst_upstream_module);
	nelts = lxl_array_nelts(&umcf->upstreams);
	uscfp = lxl_array_elts(&umcf->upstreams);
	for (i = 0; i < nelts; ++i) {
		if (uscfp[i]->host.len != u->host.len || strncasecmp(uscfp[i]->host.data, u->host.data, u->host.len) != 0) {
			continue;
		}

		if ((flags & LXL_DFST_UPSTREAM_CREATE) && (uscfp[i]->flags & LXL_DFST_UPSTREAM_CREATE)) {
			lxl_conf_log_error(LXL_LOG_EMERG, cf, 0, "duplicate upstream %s", u->host.data);
			return NULL;
		}

		return uscfp[i];
	}

	uscf = lxl_pcalloc(cf->pool, sizeof(lxl_dfst_upstream_srv_conf_t));
	if (uscf == NULL) {
		return NULL;
	}

	uscf->flags = flags;
	uscf->host = u->host;

	uscfp = lxl_array_push(&umcf->upstreams);
	if (uscfp == NULL) {
		return NULL;
	}

	*uscfp = uscf;

	return uscf;
}

static void *
lxl_dfst_upstream_create_main_conf(lxl_conf_t *cf)
{
	lxl_dfst_upstream_main_conf_t  *umcf;

	umcf = lxl_pcalloc(cf->pool, sizeof(lxl_dfst_upstream_main_conf_t));
	if (umcf == NULL) {
		return NULL;
	}

	if (lxl_array_init(&umcf->upstreams, cf->pool, 1, sizeof(lxl_dfst_upstream_srv_conf_t *)) != 0) {
		return NULL;
	}

	return umcf;
}

static char *
lxl_dfst_upstream_init_main_conf(lxl_conf_t *cf, void *conf)
{
	lxl_dfst_upstream_main_conf_t  *umcf = conf;
	lxl_uint_t						i, nelts;
	lxl_dfst_upstream_srv_conf_t  **uscfp;

	nelts = lxl_array_nelts(&umcf->upstreams);
	uscfp = lxl_array_elts(&umcf->upstreams);
	for (i = 0; i < nelts; ++i) {
		uscfp[i]->peer.init_upstream = lxl_dfst_upstream_init_round_robin_storage;
	}

	return LXL_CONF_OK;
}
