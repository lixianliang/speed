
/*
 * Copyright (C) xianliang.li
 */

#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_dfst.h>


/* parse_request_header */
int
lxl_dfst_parse_request(lxl_dfst_request_t *r)
{
	lxl_buf_t  *b;

	snprintf(r->rid, sizeof(r->rid), "%08lx%08x", lxl_current_sec, lxl_dfst_request_seed);
	++lxl_dfst_request_seed;

	b = r->header_buf;
	r->request_header.body_n = ntohl(*((uint32_t *) b->pos));
	b->pos += 6;
	r->request_header.qtype = ntohs(*((uint16_t *) b->pos));
	b->pos += 2;

	return 0;
}

/*int
lxl_dfst_parse_udp_request(lxl_dfst_request_t *r, char *data)
{
	r->request_header.body_n = ntohl( *((uint32_t *) data));
	r->request_header.qtype = ntohs(*((uint16_t *) (data + 6)));
	
	return 0;
}*/
