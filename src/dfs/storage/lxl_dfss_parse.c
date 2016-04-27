
/*
 * Copyright (C) xianliang.li
 */


#include <lxl_config.h>
#include <lxl_core.h>
#include <lxl_dfss.h>


int
lxl_dfss_parse_request(lxl_dfss_request_t *r, char *data) 
{
	uint16_t  new_qtype;

	/* time suport*/
	r->new_request = 1;
	snprintf(r->rid, sizeof(r->rid), "%08lx%08x", lxl_current_sec, lxl_dfss_request_seed);
	++lxl_dfss_request_seed;

	r->request_header.body_n = ntohl(*((uint32_t *) data));
	r->request_header.flen = ntohs(*((uint16_t *) (data + 4)));
	if (r->first_qtype) {
		new_qtype = ntohs(*(uint16_t *) (data + 6));
		if (r->request_header.qtype != new_qtype) {
			lxl_log_error(LXL_LOG_WARN, 0, "dfss new qtype: %04X not same first qtype: %04X", new_qtype, r->request_header.qtype);
			// rcode r->header.rcode = 1
			return -1;
		}
	} else {
		r->request_header.qtype = ntohs(*((uint16_t *) (data + 6)));
		r->first_qtype = 1;
	}

	//snprintf(r->rid, 17, "%08lX%08X", lxl_current_sec, lxl_dfss_request_seed);
	//++lxl_dfss_request_seed;
	// switch(qtype)
	return 0;
}
