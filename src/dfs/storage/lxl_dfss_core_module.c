
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_dfss.h>


#define  LXL_DFSS_DATA_PATH				"./data/"
#define  LXL_DFSS_SYSNLOG_PATH			"synclog/"
#define  LXL_DFSS_SYNCLOG_FILE_PREFIX	"sync_fid."

#define  LXL_DFSS_STAT_FILE				"/proc/stat"
#define  LXL_DFSS_LOADAVG_FILE			"/proc/loadavg"


static void *	lxl_dfss_create_main_conf(lxl_conf_t *cf);
static char *	lxl_dfss_init_main_conf(lxl_conf_t *cf, void *conf);
static void *	lxl_dfss_create_srv_conf(lxl_conf_t *cf);
static char *	lxl_dfss_init_srv_conf(lxl_conf_t *cf, void *conf);
static char *	lxl_dfss_core_server(lxl_conf_t *cf, lxl_command_t *cmd, void *conf);
static char *	lxl_dfss_core_listen(lxl_conf_t *cf, lxl_command_t *cmd, void *conf);

static int		lxl_dfss_core_init(lxl_cycle_t *cycle);
static int		lxl_dfss_collect_fid(char *path);
static int		lxl_dfss_collect_sync_push_fid(char *path);


static lxl_command_t lxl_dfss_core_commands[] = {
	
	{ lxl_string("server"),
  	  LXL_DFSS_MAIN_CONF|LXL_CONF_BLOCK|LXL_CONF_NOARGS,
	  lxl_dfss_core_server,
	  0,
	  0,
	  NULL },

	{ lxl_string("listen"),
	  LXL_DFSS_SRV_CONF|LXL_CONF_TAKE1,
	  lxl_dfss_core_listen,
	  0,
	  0,
	  NULL },

	lxl_null_command
};

static lxl_dfss_module_t lxl_dfss_core_module_ctx = {
//	lxl_string("dfss"),
	lxl_dfss_create_main_conf,
	lxl_dfss_init_main_conf,
	lxl_dfss_create_srv_conf,
	lxl_dfss_init_srv_conf
};

lxl_module_t lxl_dfss_core_module = {
	0,
	0,
	(void *) &lxl_dfss_core_module_ctx,
	lxl_dfss_core_commands,
	LXL_DFSS_MODULE,
	NULL,
	lxl_dfss_core_init
};


static void *
lxl_dfss_create_main_conf(lxl_conf_t *cf)
{
	lxl_dfss_core_main_conf_t *cmcf;

	cmcf = lxl_pcalloc(cf->pool, sizeof(lxl_dfss_core_main_conf_t));
	if (cmcf == NULL) {
		return NULL;
	}

	return cmcf;
}

static char *
lxl_dfss_init_main_conf(lxl_conf_t *cf, void *conf) 
{
//	lxl_dfss_core_main_conf_t *cmcf = conf;

	return LXL_CONF_OK;
}

static void *
lxl_dfss_create_srv_conf(lxl_conf_t *cf)
{
	lxl_dfss_core_srv_conf_t *cscf;

	cscf = lxl_pcalloc(cf->pool, sizeof(lxl_dfss_core_srv_conf_t));
	if (cscf == NULL) {
		return NULL;
	}

	return cscf;
}

static char *
lxl_dfss_init_srv_conf(lxl_conf_t *cf, void *conf)
{
//	lxl_dfss_core_srv_conf_t *cscf = conf;

	return LXL_CONF_OK;
}

static char *
lxl_dfss_core_server(lxl_conf_t *cf, lxl_command_t *cmd, void *conf)
{
	char 					   *rv;
	void 					   *mconf;
	lxl_uint_t 					i;
	lxl_conf_t 					pcf;
	lxl_dfss_module_t 		   *module;
	lxl_dfss_conf_ctx_t 	   *ctx, *main_ctx;
	lxl_dfss_core_srv_conf_t   *cscf, *cscfp;
	lxl_dfss_core_main_conf_t  *cmcf;

	main_ctx = cf->ctx;
	cmcf = main_ctx->main_conf[lxl_dfss_core_module.ctx_index];
	//cscfp = cmcf->server;

	if (cmcf->server != NULL) {
		return "is duplicate";
	}

	/*cscfp = lxl_pcalloc(cf->pool, sizeof(lxl_dfss_core_srv_conf_t));
	if (cscfp == NULL) {
		return LXL_CONF_ERROR;
	}*/
	
	ctx = lxl_pcalloc(cf->pool, sizeof(lxl_dfss_conf_ctx_t));
	if (ctx == NULL) {
		return LXL_CONF_ERROR;
	}

	ctx->main_conf = main_ctx->main_conf;
	ctx->srv_conf = lxl_pcalloc(cf->pool, lxl_dfss_max_module * sizeof(void *));
	if (ctx->srv_conf == NULL) {
		return LXL_CONF_ERROR;
	}

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

	cscf = ctx->srv_conf[lxl_dfss_core_module.ctx_index];
	cscf->ctx = ctx;
	//cmcf = ctx->main_conf[lxl_dfss_core_module.ctx_index];
	/*cscfp = lxl_array_push(&cmcf->servers);
	if (cscfp == NULL) {
		return LXL_CONF_ERROR;
	}

	*cscfp = cscf;*/
	cmcf->server = cscf;

	pcf = *cf;
	cf->ctx = ctx;
	//cf->cmd_type = LXL_DFSS_MODULE;
//	cf->cmd_type = LXL_DFSS_MAIN_CONF;
	cf->cmd_type = LXL_DFSS_SRV_CONF;
	rv = lxl_conf_parse(cf, NULL);
	*cf = pcf;

	return rv;
}

static char *
lxl_dfss_core_listen(lxl_conf_t *cf, lxl_command_t *cmd, void *conf) {
	off_t 						off;
	lxl_uint_t 					i, nelts;
	lxl_url_t  					u;
	lxl_str_t 				   *value;
	in_port_t  					port;
	struct sockaddr 		   *sa;
	struct sockaddr_in 		   *sin;
	lxl_dfss_listen_t 		   *ls;
	lxl_dfss_core_main_conf_t  *cmcf;

	cmcf = lxl_dfss_conf_get_module_main_conf(cf, lxl_dfss_core_module);
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

	/*ls = lxl_array_elts(&cmcf->listens);
	nelts = lxl_array_nelts(&cmcf->listens);
	for (i = 0; i < nelts; ++i) {
		sa = (struct sockaddr *) ls[i].sockaddr;
		off = offsetof(struct sockaddr_in, sin_addr);
		sin = (struct sockaddr_in *) sa;
		port = sin->sin_port;
		if (memcmp(ls[i].sockaddr + off, u.sockaddr + off, 4) != 0)  {
			continue;
		}

		if (port != u.port) {
			continue;
		}

		lxl_conf_log_error(LXL_LOG_EMERG, cf, 0, "duplicate \"%s\" address and port again", &u.url);
		return LXL_CONF_ERROR;
	}

	ls = lxl_array_push(&cmcf->listens);
	if (ls == NULL) {
		return LXL_CONF_ERROR;
	}*/

	ls = lxl_pcalloc(cf->pool, sizeof(lxl_dfss_listen_t));
	if (ls == NULL) {
		return LXL_CONF_ERROR;
	}

	//memset(ls, 0x00, sizeof(lxl_dfss_listen_t));
	memcpy(ls->sockaddr, u.sockaddr, u.socklen);
	ls->socklen = u.socklen;
	ls->wildcard = u.wildcard;
	ls->ctx = cf->ctx;
	cmcf->listen = ls;

	sin = (struct sockaddr_in *) ls->sockaddr;
	lxl_dfss_ip_port.ip = sin->sin_addr.s_addr;
	lxl_dfss_ip_port.port = sin->sin_port;

	return LXL_CONF_OK;
}

static int
lxl_dfss_core_init(lxl_cycle_t *cycle)
{
	int 			   s;
	unsigned  		   i, n;
	char 	     	  *base_path = LXL_DFSS_DATA_PATH;
	char 	  		   dir[256];
	struct ifreq	   ifr;
	lxl_uint_t 		   nelts;
	lxl_listening_t   *ls;
	lxl_connection_t  *c;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss core init");

	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s == -1) {
		lxl_log_error(LXL_LOG_ERROR, errno, "socket() failed");
		return -1;
	}
	
	strncpy(ifr.ifr_name, "eth0", sizeof(ifr.ifr_name));

	if (ioctl(s, SIOCGIFHWADDR, &ifr) != 0) {
		lxl_log_error(LXL_LOG_ERROR, errno, "ioctl() failed");
		return -1;
	}

	snprintf(lxl_dfss_uid, sizeof(lxl_dfss_uid), "%02x%02x%02x%02x%02x%02x%04x",
				(unsigned char) ifr.ifr_hwaddr.sa_data[0],
				(unsigned char) ifr.ifr_hwaddr.sa_data[1],
				(unsigned char) ifr.ifr_hwaddr.sa_data[2],
				(unsigned char) ifr.ifr_hwaddr.sa_data[3],
				(unsigned char) ifr.ifr_hwaddr.sa_data[4],
				(unsigned char) ifr.ifr_hwaddr.sa_data[5],
				lxl_dfss_ip_port.port);

	lxl_log_error(LXL_LOG_INFO, 0, "dfss uid %s", lxl_dfss_uid);

	if (lxl_hash1_init(&lxl_dfss_fid_hash, 1000000, lxl_hash_key) == -1) {
		return -1;
	}

	lxl_list_init(&lxl_dfss_sync_pull_list);
	lxl_list_init(&lxl_dfss_sync_push_list);
	lxl_dfss_temp_pool = lxl_create_pool(LXL_DEFAULT_POOL_SIZE);
	if (lxl_dfss_temp_pool == NULL) {
		return -1;
	}

	if (lxl_hash_init(&lxl_dfss_fid_phash, lxl_dfss_temp_pool, 10000000, lxl_hash_key) == -1) {
		goto failed;
	}

	if (access(base_path, R_OK|W_OK|X_OK) != 0) {
		//lxl_log_error(LXL_LOG_INFO, errno, "access(R_OK|W_OK|X_OK) %s failed", base_path);
		if (errno == ENOENT) {
			if (mkdir(base_path, 0744) != 0) {
			//if (mkdir(base_path, 0774) != 0) {
				lxl_log_error(LXL_LOG_EMERG, errno, "mkdir(%s) failed", base_path);
				goto failed;
			}
		} else {
			lxl_log_error(LXL_LOG_EMERG, errno, "access(R_OK|W_OK|X_OK) %s failed", base_path);
			goto failed;
		}
	}

	n = LXL_DFSS_DIR_COUNT;
	for (i = 0; i < n; ++i) {
		snprintf(dir, 256, "%s%02X", base_path, i);
		/* access qu diao panduan cuowu */
		if (access(dir, R_OK|W_OK|X_OK) != 0 && errno == ENOENT) {
			if (errno == ENOENT) {
			if (mkdir(dir, 0777) != 0) {
				lxl_log_error(LXL_LOG_EMERG, errno, "mkdir(%s) failed", dir);
				goto failed;
			}
			} else {
				lxl_log_error(LXL_LOG_EMERG, errno, "access(R_OK|W_OK|X_OK) %s failed", dir);
				goto failed;
			}
		}
	}

	lxl_dfss_collect_fid(LXL_DFSS_DATA_PATH);
	lxl_dfss_collect_sync_push_fid(LXL_DFSS_SYSNLOG_PATH);

	
	lxl_dfss_dir_seed = 0;
	lxl_dfss_file_count = 0;
	//lxl_dfss_ip_port.ip = 1024;
	//lxl_dfss_ip_port.port = 1024;

	nelts = lxl_array_nelts(&cycle->listening);
	if (nelts == 0) {
		lxl_log_error(LXL_LOG_ERROR, 0, "listening nelts is 0");
		return -1;
	}

	ls = lxl_array_data(&cycle->listening, lxl_listening_t, 0);

	c = lxl_get_connection(-1);
	if (c == NULL) {
		return -1;
	}

	c->listening = ls;
	memset(&lxl_dfss_report_state_event, 0x00, sizeof(lxl_event_t));
	lxl_dfss_report_state_event.data = c;
	lxl_dfss_report_state_event.handler = lxl_dfss_report_state_handler;
	//lxl_add_timer(&lxl_dfss_report_state_event, 100);

	c = lxl_get_connection(-1);
	if (c == NULL) {
		return -1;
	}

	c->listening = ls;
	memset(&lxl_dfss_sync_push_event, 0x00, sizeof(lxl_event_t));
	lxl_dfss_sync_push_event.data = c;
	lxl_dfss_sync_push_event.handler = lxl_dfss_sync_push_handler;
	//lxl_add_timer(&lxl_dfss_report_fid_event, 60000);
	
	c = lxl_get_connection(-1);
	if (c == NULL) {
		return -1;
	}

	c->listening = ls;
	memset(&lxl_dfss_sync_pull_event, 0x00, sizeof(lxl_event_t));
	lxl_dfss_sync_pull_event.data = c;
	lxl_dfss_sync_pull_event.handler = lxl_dfss_sync_pull_handler;

	c = lxl_get_connection(-1);
	if (c == NULL) {
		return -1;
	}
	
	c->listening = ls;
	memset(&lxl_dfss_sync_fid_event, 0x00, sizeof(lxl_event_t));
	lxl_dfss_sync_fid_event.data = c;
	lxl_dfss_sync_fid_event.handler = lxl_dfss_sync_fid_handler;
	//lxl_dfss_sync_fid_handler(&lxl_dfss_sync_fid_event);
	lxl_dfss_sync_fid();

	return 0;

failed:
	
	lxl_destroy_pool(lxl_dfss_temp_pool);

	return -1;
}

void
lxl_dfss_update_system_state()
{
	int    			  	i1, i2, i3, sys_idle, sys_usage, sys_idle_diff, sys_total_diff;
	char   			    buf[1024];
	FILE    		   *fp;
	uint64_t  		  	bytes;
	struct statfs  	  	st;
	lxl_dfss_cpu_use_t  cpu_use;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "dfss update system state");

	if (statfs("./", &st) != 0) {
		lxl_log_error(LXL_LOG_ERROR, errno, "stat(%s) failed", "./");
		return;
	}

	/* bytes = st.f_bfree * st.f_bsize; root user */
	bytes = st.f_bavail * st.f_bsize;
	lxl_dfss_disk_free_mb = bytes / 1048576;	/* 1024 * 1024 */

	lxl_log_error(LXL_LOG_INFO, 0, "dfss disk free %u mb", lxl_dfss_disk_free_mb);

	fp = fopen(LXL_DFSS_LOADAVG_FILE, "r");
	if (fp == NULL) {
		lxl_log_error(LXL_LOG_ERROR, errno, "fopen(%s) failed", LXL_DFSS_LOADAVG_FILE);
		return;
	}

	if (fgets(buf, sizeof(buf), fp) == NULL) {
		lxl_log_error(LXL_LOG_ERROR, errno, "fgets() failed");
		return;
	}

	sscanf(buf, "%*s %d.%d %*s", &i1, &i2);
	fclose(fp);

	i3 = 100 * i1 + i2;
	lxl_dfss_loadavg5 = i3 / lxl_ncpu;	/* lxl_ncpu */

	lxl_log_error(LXL_LOG_INFO, 0, "dfss loadavg5 %d %d.%d", lxl_dfss_loadavg5, i1, i2);

	fp = fopen(LXL_DFSS_STAT_FILE, "r");
	if (fp == NULL) {
		lxl_log_error(LXL_LOG_ERROR, errno, "fopen(%s) failed", LXL_DFSS_LOADAVG_FILE);
		return;
	}

	if (fgets(buf, sizeof(buf), fp) == NULL) {
		lxl_log_error(LXL_LOG_ERROR, errno, "fgets() failed");
		return;
	}

	sscanf(buf, "%*s %d %d %d %d %d %d %d", 
		&cpu_use.use1, &cpu_use.use2, &cpu_use.use3, &cpu_use.use4, &cpu_use.use5, &cpu_use.use6, &cpu_use.use7);
	fclose(fp);
	
	/* idle cpu use  use4 */
	cpu_use.total = cpu_use.use1 + cpu_use.use2 + cpu_use.use3 + cpu_use.use4 + cpu_use.use5 + cpu_use.use6 + cpu_use.use7;
	sys_idle_diff = cpu_use.use4 - lxl_dfss_cpu_use.use4;
	sys_total_diff = cpu_use.total - lxl_dfss_cpu_use.total;
	sys_idle = (100 * sys_idle_diff) / sys_total_diff;
	/*sys_usage = 100 - sys_idle;
	lxl_dfss_cpu_usage = sys_usage;*/
	lxl_dfss_cpu_idle = sys_idle;
	lxl_dfss_cpu_use = cpu_use;

	lxl_log_error(LXL_LOG_INFO, 0, "dfss cpu idle %d, idle diff:%d, total diff:%d", sys_idle, sys_idle_diff, sys_total_diff);
}

static int
lxl_dfss_collect_fid(char *base_path)
{
	size_t			len;
	DIR			   *dir;
	struct stat		stat;
	struct dirent  *de;
	lxl_str_t      *path, *value, *fid;
	lxl_queue_t    *queue;
	lxl_dfs_fid_t  *dfs_fid;

	lxl_log_error(LXL_LOG_INFO, 0, "dfss collect fid %s", base_path);

	queue = lxl_queue_create(lxl_dfss_temp_pool, 1024, sizeof(lxl_str_t));
	if (queue == NULL) {
		return -1;
	}

	value = lxl_queue_in(queue);
	if (value  == NULL) {
		return -1;
	}

	len = strlen(base_path);
	value->data = lxl_palloc(lxl_dfss_temp_pool, len + 1);
	if (value->data == NULL) {
		return -1;
	}

	lxl_str_memcpy(value, base_path, len);

	while ((path = lxl_queue_out(queue)) != NULL) {
		dir = opendir(path->data);
		if (dir == NULL) {
			lxl_log_error(LXL_LOG_ERROR, errno, "opendir(%s) failed", path->data);
			goto failed;
		}

		while ((de = readdir(dir)) != NULL) {
			if (S_ISDIR(de->d_type)) {
				value = lxl_queue_in(queue);
				if (value == NULL) {
					goto failed;
				}

				len = strlen(de->d_name);
				value->data = lxl_palloc(lxl_dfss_temp_pool, len + 1);
				if (value->data == NULL) {
					goto failed;
				}

				lxl_str_memcpy(value, de->d_name, len);
			} else if (S_ISREG(de->d_type)) {
				fid = lxl_palloc(lxl_dfss_temp_pool, sizeof(lxl_str_t));
				if (fid == NULL) {
					goto failed;
				}

				len = strlen(de->d_name);
				fid->data = lxl_palloc(lxl_dfss_temp_pool, len + 1);
				if (fid->data == NULL) {
					goto failed;
				}

				lxl_str_memcpy(fid, de->d_name, len);
				lxl_hash_add(&lxl_dfss_fid_phash, fid->data, len, fid);
			} else {
			}
		}

		closedir(dir);
	}

	return 0;

failed:

	closedir(dir);

	return -1;
}

static int
lxl_dfss_collect_sync_push_fid(char *base_path)
{
	size_t		 	len;
	DIR	  		   *dir;
	FILE  		   *fp;
	struct dirent  *de;
	char   			buf[1024], b[1024];
	lxl_dfs_fid_t  *dfs_fid;

	lxl_log_error(LXL_LOG_INFO, 0, "dfss collect sync push fid %s", base_path);

	dir = opendir(base_path);
	if (dir == NULL) {
		lxl_log_error(LXL_LOG_ERROR, errno, "opendir(%s) failed", base_path);
		return -1;
	}

	while ((de = readdir(dir)) != NULL) {
		if (S_ISREG(de->d_type)
			&& strncmp(de->d_name, LXL_DFSS_SYNCLOG_FILE_PREFIX, strlen(LXL_DFSS_SYNCLOG_FILE_PREFIX)) == 0) {
			fp = fopen(de->d_name, "r");
			if (fp == NULL) {
				lxl_log_error(LXL_LOG_ERROR, errno, "fopen(%s) failed", de->d_name);
				goto failed;
			}

			while (fgets(buf, sizeof(buf), fp) != NULL) {
				len = strlen(buf);		
				buf[len - 1] = '\0';
				dfs_fid = lxl_alloc(sizeof(lxl_dfs_fid_t));
				if (dfs_fid == NULL) {
					fclose(fp);
					goto failed;
				}

				dfs_fid->fid.data = lxl_alloc(len + 1);
				if (dfs_fid->fid.data == NULL) {
					fclose(fp);
					goto failed;
				}

				lxl_str_memcpy(&dfs_fid->fid, buf, len);
				lxl_list_add_tail(&lxl_dfss_sync_push_list, &dfs_fid->list);
			}

			fclose(fp);
		}
	}

	closedir(dir);

	return 0;

failed:

	closedir(dir);

	return -1;
}

void 
lxl_dfss_sync_fid(void)
{
	lxl_log_error(LXL_LOG_INFO, 0, "dfss sync fid");
	lxl_dfss_server_start();
}

void 
lxl_dfss_sync_pull(void)
{
	lxl_log_error(LXL_LOG_INFO, 0, "dfss sync pull");
	if (lxl_list_empty(&lxl_dfss_sync_pull_list)) {
		lxl_dfss_server_start();
	} else {
		lxl_dfss_sync_pull_handler(&lxl_dfss_sync_pull_event);
	}
}

void
lxl_dfss_server_start(void)
{
	lxl_log_error(LXL_LOG_INFO, 0, "dfss server start");

	lxl_dfss_update_system_state();
	lxl_add_timer(&lxl_dfss_report_state_event, 100);
	lxl_add_timer(&lxl_dfss_sync_push_event, 100);
}
