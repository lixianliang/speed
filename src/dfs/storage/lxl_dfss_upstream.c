
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_dfss.h>
//#include <lxl_dfss_core_module.h>
//#include <lxl_dfss_upstream_round_robin.h>
//#include <lxl_dfss_upstream.h>


//static void lxl_dfss_upstream_handler(lxl_event_t *ev);
static char *lxl_dfss_upstream(lxl_conf_t *cf, lxl_command_t *cmd, void *dummy);
static char *lxl_dfss_upstream_server(lxl_conf_t *cf, lxl_command_t *cmd, void *conf);
static void *lxl_dfss_upstream_create_main_conf(lxl_conf_t *cf);
static char *lxl_dfss_upstream_init_main_conf(lxl_conf_t *cf, void *conf);


/* upstream */
static lxl_command_t  lxl_dfss_upstream_commands[] = {
	
	{ lxl_string("upstream"),
	  LXL_DFSS_MAIN_CONF|LXL_CONF_BLOCK|LXL_CONF_TAKE1,
	  lxl_dfss_upstream,
      0,
	  0,
	  NULL },

	{ lxl_string("server"),
	  LXL_DFSS_UPS_CONF|LXL_CONF_1MORE,
   	  lxl_dfss_upstream_server,
	  LXL_DFSS_SRV_CONF_OFFSET,
	  0,
	  NULL },
	
	lxl_null_command
};

static lxl_dfss_module_t  lxl_dfss_upstream_module_ctx = {
	lxl_dfss_upstream_create_main_conf,
	lxl_dfss_upstream_init_main_conf,
	NULL,
	NULL
};

lxl_module_t  lxl_dfss_upstream_module = {
	0,
	0,
	(void *) &lxl_dfss_upstream_module_ctx,
	lxl_dfss_upstream_commands,
	LXL_DFSS_MODULE,
	NULL,
	NULL
};


void
lxl_dfss_upstream_connect(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u)
{
	int rc;
	lxl_connection_t *c;

	rc = lxl_event_connect_peer(&u->peer);
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss upstream connect: %d", rc);
	if (rc == LXL_ERROR) {
		// lxl_dfss_upstream_finialize_request(r, u, internal_server_error);
		return;
	}

	if (rc == LXL_BUSY) {
		lxl_log_error(LXL_LOG_ERROR, 0, "dfss no live upstreams");
//		lxl_dfss_upstream_next(r, u, LXL_DFSS_UPSTREAM_FT_NOLIVE);
		return;
	}

	c = u->peer.connection;
	c->data = r;
//	c->write->handler = lxl_dfss_upstream_handler;
//	c->read->handler = lxl_dfss_upstream_handler;
	//u->write->handler = lxl_dfss_tracker_send
	//u->read_event_handler
}

/*static void 
lxl_dfss_upstream_handler(lxl_event_t *ev)
{
	lxl_connection_t *c;
	lxl_dfss_request_t *r;
	
	c = ev->data;
	r = c->data;

}*/

/*void
lxl_dfss_upstream_next(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u, lxl_uint_t ft_type)
{
}*/

void    
lxl_dfss_upstream_request_empty_handler(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u)
{
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss upstream request empty handler %s", r->rid);

	return;
}

void 
lxl_dfss_upstream_block_reading(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u)
{
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss upstream reading blocked %s", r->rid);

	return;
}

void
lxl_dfss_upstream_parse_response(lxl_dfss_request_t *r, lxl_dfss_upstream_t *u, lxl_buf_t *b)
{
	u->response_header.body_n = ntohl(*((uint32_t *) b->pos));
	b->pos += 4;
	u->response_header.flen = ntohl(*((uint16_t *) b->pos));
	b->pos += 2;
	u->response_header.rcode = ntohs(*((uint16_t *) b->pos));
	b->pos += 2;
}

static char *
lxl_dfss_upstream(lxl_conf_t *cf, lxl_command_t *cmd, void *dummy)
{
	char 	   					  *rv;
	void 	   					  *mconf;
	lxl_uint_t					   i;
	lxl_url_t					   u;
	lxl_str_t					  *value;
	lxl_conf_t 					   pcf;
	lxl_dfss_module_t 			  *module;
	lxl_dfss_conf_ctx_t			  *ctx, *dfss_ctx;
	//lxl_dfss_upstream_main_conf_t *umcf;
	lxl_dfss_upstream_srv_conf_t  *uscf, **uscfp;

	/*umcf = lxl_dfss_conf_get_module_main_conf(cf, lxl_dfss_upstream_module);
	if (umcf->upstreams != NULL) {
		lxl_conf_log_error(LXL_LOG_EMERG, cf, 0, "dfss duplicate upstream");
		return LXL_CONF_ERROR;
	}

	uscf = umcf->upstream = lxl_pcalloc(cf->pool, sizeof(lxl_dfss_upstream_srv_conf_t));
	if (umcf->upstream == NULL) {
		return LXL_CONF_ERROR;
	}*/
	
	memset(&u, 0x00, sizeof(lxl_url_t));
	value = lxl_array_elts(cf->args);
	u.host = value[1];
	u.no_port = 1;

	uscf = lxl_dfss_upstream_add(cf, &u, LXL_DFSS_UPSTREAM_CREATE);
	if (uscf == NULL) {
		return LXL_CONF_ERROR;
	}

	ctx = lxl_pcalloc(cf->pool, sizeof(lxl_dfss_conf_ctx_t));
	if (ctx == NULL) {
		return LXL_CONF_ERROR;
	}

	dfss_ctx = cf->ctx;
	ctx->main_conf = dfss_ctx->main_conf;

	/* upstream{}'s srv_conf */
	ctx->srv_conf = lxl_pcalloc(cf->pool, sizeof(void *) * lxl_dfss_max_module);
	if (ctx->srv_conf == NULL) {
		return NULL;
	}
	
	ctx->srv_conf[lxl_dfss_upstream_module.ctx_index] = uscf;
	uscf->srv_conf = ctx->srv_conf;

	for (i = 0; lxl_modules[i]; ++i) {
		if (lxl_modules[i]->type != LXL_DFSS_MODULE) {
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

	/* parse inside upstream{} */
	pcf = *cf;
	cf->ctx = ctx;
	cf->cmd_type = LXL_DFSS_UPS_CONF;
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
lxl_dfss_upstream_server(lxl_conf_t *cf, lxl_command_t *cmd, void *conf)
{
	lxl_dfss_upstream_srv_conf_t  *uscf = conf;
	
	char 						 name[64];
	size_t 						 len;
	off_t						 off;
	lxl_uint_t 					 i, nelts;
	lxl_str_t      		   		*value;
	lxl_url_t  					 u;
	in_port_t  					 port;
	struct sockaddr 	   		*sa;
	struct sockaddr_in 	    	*sin;
	lxl_dfss_upstream_server_t  *s, *us;

	if (uscf->servers == NULL) {
		uscf->servers = lxl_array_create(cf->pool, 4, sizeof(lxl_dfss_upstream_server_t));
		if (uscf->servers == NULL) {
			return LXL_CONF_ERROR;
		}
	}

	value = lxl_array_elts(cf->args);
	memset(&u, 0x00, sizeof(lxl_url_t));
	u.url = value[1];
	u.default_port = 1305;	/* DFST(1305) DFSS(1304) */

	if (lxl_parse_url(cf->pool, &u) != 0) {
		if (u.err) {
			lxl_conf_log_error(LXL_LOG_EMERG, cf, 0, "%s in upstream %s", u.err, &u.url);
		}

		return LXL_CONF_ERROR;
	}

	/* max_fails fail_timeout */
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
	len = (size_t) snprintf(name, 64, "%s:%d", inet_ntoa(sin->sin_addr), htons(sin->sin_port)); 
	us = lxl_array_push(uscf->servers);
	if (us == NULL) {
		return LXL_CONF_ERROR;
	}

	memset(us, 0x00, sizeof(lxl_dfss_upstream_server_t));
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

lxl_dfss_upstream_srv_conf_t *
lxl_dfss_upstream_add(lxl_conf_t *cf, lxl_url_t *u, lxl_uint_t flags)
{
	lxl_uint_t 					   	i, nelts;
	lxl_dfss_upstream_srv_conf_t   *uscf, **uscfp;
	lxl_dfss_upstream_main_conf_t  *umcf;

	umcf = lxl_dfss_conf_get_module_main_conf(cf, lxl_dfss_upstream_module);
	nelts = lxl_array_nelts(&umcf->upstreams);
	uscfp = lxl_array_elts(&umcf->upstreams);
	for (i = 0; i < nelts; ++i) {
		if (uscfp[i]->host.len != u->host.len || strncasecmp(uscfp[i]->host.data, u->host.data, u->host.len) != 0) {
			continue;
		}

		if ((flags & LXL_DFSS_UPSTREAM_CREATE) && (uscfp[i]->flags & LXL_DFSS_UPSTREAM_CREATE)) {
			lxl_conf_log_error(LXL_LOG_EMERG, cf, 0, "duplicate upstream %s", u->host.data);
			return NULL;
		}

		return uscfp[i];
	}

	uscf = lxl_pcalloc(cf->pool, sizeof(lxl_dfss_upstream_srv_conf_t));
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
lxl_dfss_upstream_create_main_conf(lxl_conf_t *cf)
{
	lxl_dfss_upstream_main_conf_t *umcf;

	umcf = lxl_pcalloc(cf->pool, sizeof(lxl_dfss_upstream_main_conf_t));
	if (umcf == NULL) {
		return NULL;
	}

	if (lxl_array_init(&umcf->upstreams, cf->pool, 2, sizeof(lxl_dfss_upstream_srv_conf_t *)) != 0) {
		return NULL;
	}

	return umcf;
}

static char *
lxl_dfss_upstream_init_main_conf(lxl_conf_t *cf, void *conf)
{
	lxl_dfss_upstream_main_conf_t  *umcf = conf;
	lxl_uint_t					    i, nelts;
	lxl_dfss_upstream_srv_conf_t  **uscfp;

	/*uscf = umcf->upstream;
	uscf->peer.init_upstream = lxl_dfss_upstream_init_round_robin;
	if (uscf->peer.init_upstream(cf, uscf) != 0) {
		return LXL_CONF_ERROR;
	}*/

	nelts = lxl_array_nelts(&umcf->upstreams);
	uscfp = lxl_array_elts(&umcf->upstreams);
	for (i = 0; i < nelts; ++i) {
		uscfp[i]->peer.init_upstream = lxl_dfss_upstream_init_round_robin;
		if (!(uscfp[i]->flags & LXL_DFSS_UPSTREAM_STORAGE) && uscfp[i]->peer.init_upstream(cf, uscfp[i]) != 0) {
			return LXL_CONF_ERROR;
		}
	}
	
	return LXL_CONF_OK;
}
