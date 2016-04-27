
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_dfst.h>


typedef struct {
	lxl_dfst_upstream_conf_t  upstream;
} lxl_dfst_storage_srv_conf_t;


static void lxl_dfst_storage_init_request(lxl_dfst_request_t *r);
static int	lxl_dfst_storage_create_request(lxl_dfst_request_t *r);


static void lxl_dfst_storage_handler(lxl_event_t *ev);

static void lxl_dfst_storage_connect(lxl_dfst_request_t *r, lxl_dfst_upstream_t *u);
static void lxl_dfst_storage_send_request(lxl_dfst_request_t *r, lxl_dfst_upstream_t *u);
static void lxl_dfst_storage_process_header(lxl_dfst_request_t *r, lxl_dfst_upstream_t *u);

static void lxl_dfst_storage_dummy_handler(lxl_dfst_request_t *r, lxl_dfst_upstream_t *u);
static void lxl_dfst_storage_next(lxl_dfst_request_t *r, lxl_dfst_upstream_t *u, lxl_uint_t ft_type);

static void *lxl_dfst_storage_create_conf(lxl_conf_t *cf);
static char *lxl_dfst_storage_init_conf(lxl_conf_t *cf, void *conf);
static char *lxl_dfst_storage_pass(lxl_conf_t *cf, lxl_command_t *cmd, void *conf);
static char *lxl_dfst_storage_lowat_check(lxl_conf_t *cf, void *post, void *data);

static lxl_conf_post_t  lxl_dfst_storage_lowat_post = { lxl_dfst_storage_lowat_check };

lxl_module_t  lxl_dfst_storage_module;

static lxl_command_t  lxl_dfst_storage_commands[] = {
	
	{ lxl_string("storage_pass"),
	  LXL_DFST_SRV_CONF|LXL_CONF_TAKE1,
	  lxl_dfst_storage_pass,
	  LXL_DFST_SRV_CONF_OFFSET,
	  0,
	  NULL },

	{ lxl_string("storage_connect_timeout"),
	  LXL_DFST_MAIN_CONF|LXL_DFST_SRV_CONF|LXL_CONF_TAKE1,
	  lxl_conf_set_msec_slot,
	  LXL_DFST_SRV_CONF_OFFSET,
	  offsetof(lxl_dfst_storage_srv_conf_t, upstream.connect_timeout),
	  NULL },

	{ lxl_string("storage_read_timeout"),
	  LXL_DFST_MAIN_CONF|LXL_DFST_SRV_CONF|LXL_CONF_TAKE1,
	  lxl_conf_set_msec_slot,
	  LXL_DFST_SRV_CONF_OFFSET,
	  offsetof(lxl_dfst_storage_srv_conf_t, upstream.read_timeout),
	  NULL },

	{ lxl_string("storage_send_lowat"),
	  LXL_DFST_MAIN_CONF|LXL_DFST_SRV_CONF|LXL_CONF_TAKE1,
	  lxl_conf_set_size_slot,
	  LXL_DFST_SRV_CONF_OFFSET,
	  offsetof(lxl_dfst_storage_srv_conf_t, upstream.send_lowat),
	  &lxl_dfst_storage_lowat_post },

	lxl_null_command
};

lxl_dfst_module_t  lxl_dfst_storage_module_ctx = {
	NULL,
	NULL,
	lxl_dfst_storage_create_conf,
	lxl_dfst_storage_init_conf
};

lxl_module_t  lxl_dfst_storage_module = {
	0,
	0,
	(void *) &lxl_dfst_storage_module_ctx,
	lxl_dfst_storage_commands,
	LXL_DFST_MODULE,
	NULL,
	NULL
};

void
lxl_dfst_storage_init(lxl_dfst_request_t *r)
{
	lxl_dfst_upstream_t 		 *u;
	lxl_dfst_storage_srv_conf_t  *sscf;

	u = r->storage;
	if (u == NULL) {
		u = lxl_pcalloc(r->pool, sizeof(lxl_dfst_upstream_t));
		if (u == NULL) {
			return;
		}
	
		r->storage = u;
	}

	sscf = lxl_dfst_get_module_srv_conf(r, lxl_dfst_storage_module);
	u->conf = &sscf->upstream;

	lxl_dfst_storage_init_request(r);
}

static void
lxl_dfst_storage_init_request(lxl_dfst_request_t *r)
{
	lxl_dfst_upstream_t			  *u;
	lxl_dfst_upstream_srv_conf_t  *uscf;

	lxl_dfst_storage_create_request(r);

	u = r->storage;
	uscf = u->conf->upstream;

	if (uscf == NULL) {
		lxl_log_error(LXL_LOG_ALERT, 0, "no upstream configuration");
		return;
	}

	if (uscf->peer.init(r, uscf, u) != 0) {
		// lxl_dfst
	}

	lxl_dfst_storage_connect(r, u);
}

static int
lxl_dfst_storage_create_request(lxl_dfst_request_t *r)
{
	return 0;
}

static void
lxl_dfst_storage_handler(lxl_event_t *ev)
{
	lxl_connection_t 	 *c;
	lxl_dfst_request_t   *r;
	lxl_dfst_upstream_t  *u;

	c = ev->data;
	r = c->data;
	u = r->storage;

	if (ev->write) {
		u->write_event_handler(r, u);
	} else {
		u->read_event_handler(r, u);
	}

	// posted_request;
}

static void
lxl_dfst_storage_connect(lxl_dfst_request_t *r, lxl_dfst_upstream_t *u)
{
	int				   rc;
	lxl_connection_t  *c;

	rc = lxl_event_connect_peer(&u->peer);
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst storage connect: %d", rc);
	if (rc == LXL_ERROR) {
		// finialize
		return;
	}

	if (rc == LXL_BUSY) {
		lxl_log_error(LXL_LOG_ERROR, 0, "dfst no live storages");
		lxl_dfst_storage_next(r, u, LXL_DFST_UPSTREAM_FT_NOLIVE);
		return;
	}

	if (rc == LXL_DECLINED) {
		lxl_dfst_storage_next(r, u, LXL_DFST_UPSTREAM_FT_ERROR);
		return;
	}

	c = u->peer.connection;
	c->data = r;
	c->write->handler = lxl_dfst_storage_handler;
	c->read->handler = lxl_dfst_storage_handler;

	if (rc == LXL_EAGAIN) {
		lxl_add_timer(c->write, 3000);
		return;
	}

	lxl_dfst_storage_send_request(r, u);
}

static void
lxl_dfst_storage_send_request(lxl_dfst_request_t *r, lxl_dfst_upstream_t *u)
{
	int				   rc;
	lxl_connection_t  *c;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst storage send request");

	c = u->peer.connection;
	if (rc == LXL_EAGAIN) {
		if (lxl_handle_write_event(c->write, 0) != 0) {
			return;
		}

		return;
	}

	lxl_add_timer(c->read, 3000);
	if (c->read->ready) {
		lxl_dfst_storage_process_header(r, u);
	}

	if (lxl_handle_write_event(c->write, 0) != 0) {
		// finalize
		return;
	}
}

static void
lxl_dfst_storage_process_header(lxl_dfst_request_t *r, lxl_dfst_upstream_t *u)
{
	return;
}

static void
lxl_dfst_storage_dummy_handler(lxl_dfst_request_t *r, lxl_dfst_upstream_t *u)
{
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst storage dummy handler");

	return;
}

static void
lxl_dfst_storage_next(lxl_dfst_request_t *r, lxl_dfst_upstream_t *u, lxl_uint_t ft_type)
{
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfst next storage, %lu", ft_type);
}

static void *
lxl_dfst_storage_create_conf(lxl_conf_t *cf)
{
	lxl_dfst_storage_srv_conf_t  *conf;

	conf = lxl_pcalloc(cf->pool, sizeof(lxl_dfst_storage_srv_conf_t));
	if (conf == NULL) {
		return NULL;
	}

	conf->upstream.connect_timeout = LXL_CONF_UNSET_MSEC;
	conf->upstream.send_timeout = LXL_CONF_UNSET_MSEC;
	conf->upstream.read_timeout = LXL_CONF_UNSET_MSEC;
	conf->upstream.send_timeout = LXL_CONF_UNSET_SIZE;

	return conf;
}

static char *
lxl_dfst_storage_init_conf(lxl_conf_t *cf, void *conf)
{
	lxl_dfst_storage_srv_conf_t  *sscf = conf;

	lxl_conf_init_msec_value(sscf->upstream.connect_timeout, 6000);
	lxl_conf_init_msec_value(sscf->upstream.send_timeout, 10000);
	lxl_conf_init_msec_value(sscf->upstream.read_timeout, 10000);
	lxl_conf_init_size_value(sscf->upstream.send_lowat, 0);

	return LXL_CONF_OK;
}

static char *
lxl_dfst_storage_pass(lxl_conf_t *cf, lxl_command_t *cmd, void *conf)
{
	lxl_dfst_storage_srv_conf_t  *sscf = conf;

	lxl_str_t  *value;
	lxl_url_t   u;

	if (sscf->upstream.upstream) {
		return "is duplicate";
	}

	memset(&u, 0x00, sizeof(lxl_url_t));
	value = lxl_array_elts(cf->args);
	u.host = value[1];
	sscf->upstream.upstream = lxl_dfst_upstream_add(cf, &u, LXL_DFST_UPSTREAM_STORAGE);
	if (sscf->upstream.upstream == NULL) {
		return LXL_CONF_ERROR;
	}

	return LXL_CONF_OK;
}

static char *
lxl_dfst_storage_lowat_check(lxl_conf_t *cf, void *post, void *data)
{
	lxl_conf_log_error(LXL_LOG_WARN, cf, 0, "storage_send_lowat is not supoort, ignored");

	return LXL_CONF_OK;
}
