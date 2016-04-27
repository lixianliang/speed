
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_dfss.h>
//#include <lxl_dfss_upstream_round_robin.h>


static lxl_dfss_upstream_rr_peer_t *lxl_dfss_upstream_get_peer(lxl_dfss_upstream_rr_peer_data_t *rrp);


int
lxl_dfss_upstream_init_round_robin(lxl_conf_t *cf, lxl_dfss_upstream_srv_conf_t *us)
{
	lxl_uint_t 					   i, nelts;
	lxl_dfss_upstream_server_t 	  *servers;
	lxl_dfss_upstream_rr_peers_t  *peers;

	us->peer.init = lxl_dfss_upstream_init_round_robin_peer;
	if (us->servers) {
		servers = lxl_array_elts(us->servers);
		nelts = lxl_array_nelts(us->servers);
		peers = lxl_pcalloc(cf->pool, sizeof(lxl_dfss_upstream_rr_peers_t) + sizeof(lxl_dfss_upstream_rr_peer_t) * (nelts - 1));
		if (peers == NULL) {
			return -1;
		}

		peers->single = 1;
		peers->number = nelts;
		for (i = 0; i < nelts; ++i) {
			// ?? char sockaddr
			peers->peer[i].sockaddr = (struct sockaddr *) servers[i].sockaddr;
			peers->peer[i].socklen = servers[i].socklen;
			peers->peer[i].name = servers[i].name;
			peers->peer[i].max_fails = servers[i].max_fails;
			peers->peer[i].fail_timeout = servers[i].fail_timeout;
		}

		us->peer.data = peers;

		return 0;
	}

	lxl_log_error(LXL_LOG_EMERG, 0, "dfss no server in upstream");

	return -1;
}

int
lxl_dfss_upstream_init_round_robin_storage(lxl_dfss_request_t *r, lxl_dfss_upstream_srv_conf_t *us)
{
	lxl_uint_t 					   i, nelts;
	lxl_dfss_upstream_server_t 	  *servers;
	lxl_dfss_upstream_rr_peers_t  *peers;

	us->peer.init = lxl_dfss_upstream_init_round_robin_peer;
	if (us->servers) {
		nelts = lxl_array_nelts(us->servers);
		servers = lxl_array_elts(us->servers);
		peers = lxl_pcalloc(r->pool, sizeof(lxl_dfss_upstream_rr_peers_t) + sizeof(lxl_dfss_upstream_rr_peer_t) * (nelts - 1));
		if (peers == NULL) {
			return -1;
		}

		peers->single = 1;
		peers->number = nelts;
		for (i = 0; i < nelts; ++i) {
			// ?? char sockaddr
			peers->peer[i].sockaddr = (struct sockaddr *) servers[i].sockaddr;
			peers->peer[i].socklen = servers[i].socklen;
			peers->peer[i].name = servers[i].name;
			peers->peer[i].max_fails = servers[i].max_fails;
			peers->peer[i].fail_timeout = servers[i].fail_timeout;
		}

		us->peer.data = peers;

		return 0;
	}

	lxl_log_error(LXL_LOG_EMERG, 0, "dfss no server in storage");

	return -1;
}

int 
lxl_dfss_upstream_init_round_robin_peer(lxl_dfss_request_t *r, lxl_dfss_upstream_srv_conf_t *us, lxl_dfss_upstream_t *u)
{
	lxl_uint_t 						   n;
	lxl_dfss_upstream_rr_peer_data_t  *rrp;

	rrp = u->peer.data;
	if (rrp == NULL) {
		rrp = lxl_palloc(r->pool, sizeof(lxl_dfss_upstream_rr_peer_data_t));
		if (rrp == NULL) {
			return -1;
		}

		u->peer.data = rrp;
	}

	rrp->peers = us->peer.data;
	rrp->current = 0;
	n = rrp->peers->number;
	rrp->tries = 0;
	rrp->data = 0;
	
	u->peer.get = lxl_dfss_upstream_get_round_robin_peer;
	u->peer.free = lxl_dfss_upstream_free_round_robin_peer;
	u->peer.tries = rrp->peers->number;
	
	return 0;
}

int
lxl_dfss_upstream_get_round_robin_peer(lxl_peer_connection_t *pc, void *data)
{
	lxl_dfss_upstream_rr_peer_data_t  *rrp = data;

	lxl_uint_t 					   i;
	lxl_dfss_upstream_rr_peer_t   *peer;
	lxl_dfss_upstream_rr_peers_t  *peers;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "get rr peer, try: %lu", pc->tries);

	/*if (rrp->peers->single) {
		peer = &rrp->peers->peer[0];
		if (peer->down) {
			goto failed;
		}
	}*/
	peer = lxl_dfss_upstream_get_peer(rrp);
	if (peer == NULL) {
		goto failed;
	}
	
	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "get rr peer, current: %lu", rrp->current);

	pc->sockaddr = peer->sockaddr;
	pc->socklen = peer->socklen;
	pc->name = peer->name;

	return LXL_OK;
	
failed:

	/* all peers failed, mark them as live for quick recovery */
	for (i = 0; i < peers->number; ++i) {
		peers->peer[i].fails = 0;
	}
	//pc->name = peers->name;

	return LXL_BUSY;

}

static lxl_dfss_upstream_rr_peer_t *
lxl_dfss_upstream_get_peer(lxl_dfss_upstream_rr_peer_data_t *rrp)
{
	time_t 						  now;
	lxl_uint_t 					  i;
	lxl_dfss_upstream_rr_peer_t  *peer, *best;
	
	now = lxl_time();
	best = NULL;

	for (i = 0; i < rrp->peers->number; ++i) {
		peer = &rrp->peers->peer[i];
		if (peer->down) {
			continue;
		}

		if (peer->max_fails && peer->fails >= peer->max_fails && now - peer->checked <= peer->fail_timeout) {
			continue;
		}

		if (best == NULL) {
			best = peer;
		}
	}

	if (peer == NULL) {
		return NULL;
	}

	i = best - &rrp->peers->peer[0];
	rrp->current = i;
	if (now - best->checked > best->fail_timeout) {
		best->checked = now;
	}

	return best;
}

void
lxl_dfss_upstream_free_round_robin_peer(lxl_peer_connection_t *pc, void *data, lxl_uint_t state)
{
	lxl_dfss_upstream_rr_peer_data_t  *rrp = data;

	time_t now;
	lxl_dfss_upstream_rr_peer_t *peer;

	lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "free rr peer %lu %lu", pc->tries, state);

	peer = &rrp->peers->peer[rrp->current];
	if (state & LXL_PEER_FAILED) {
		now = lxl_time();

		++peer->fails;
		peer->accessed = now;
		peer->checked = now;
		
		lxl_log_debug(LXL_LOG_DEBUG_DFS, 0, "free rr peer failed: %lu", rrp->current);
	} else {
		if (peer->accessed < peer->checked) {
			peer->fails = 0;
		}
	}

	if (pc->tries) {
		--pc->tries;
	}
}
