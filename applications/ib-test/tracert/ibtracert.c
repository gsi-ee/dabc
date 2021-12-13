/*
 * Copyright (c) 2004-2009 Voltaire Inc.  All rights reserved.
 * Copyright (c) 2009 HNR Consulting.  All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif				/* HAVE_CONFIG_H */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <getopt.h>
#include <netinet/in.h>
#include <inttypes.h>

#include <infiniband/umad.h>
#include <infiniband/mad.h>
#include <complib/cl_nodenamemap.h>

#include "ibdiag_common.h"

struct ibmad_port *srcport;

#define MAXHOPS	63

static char *node_type_str[] = {
	"???",
	"ca",
	"switch",
	"router",
	"iwarp rnic"
};

static int timeout = 0;		/* ms */
static int force;
static FILE *f;

static int use_cache = 0;  /* use cached information */



#define NUM_SWITCHES 48
#define MAX_LID 65536

// this is for one, two and three legs roue
// one leg route should be always 0               : coded as 0
// two leg route should be 0, indx (maximum 36)   : coded as 1 + indx
// three leg route should be 0, indx1, indx2      : coded as 37 + indx1 + indx2*36
// as result maximum route id is 37 + 36 + 36*36 = 1442
#define MAX_ROUTE 1442

int switches_lids[NUM_SWITCHES];

int8_t* switches_cache[NUM_SWITCHES];

int8_t route_cached[MAX_ROUTE];

char* route_cache[MAX_ROUTE];

int route_cache_skiplen; // number of address points, which can be skipped from direct path len

static char** node_lid_cache = 0; // this is cache for node info with normal lid numbers

static int dumplevel = 2, multicast, mlid;

static int maxnumlids = 1;

static int optimize_routes = 0;

static char* fnodes_list = NULL;


void init_cache()
{
   int id, lid;

   if (!use_cache) return;

   for (id=0;id<NUM_SWITCHES;id++) {
      switches_lids[id] = 0;
      switches_cache[id] = (int8_t*) malloc(MAX_LID*sizeof(int8_t));
      for (lid=0;lid<MAX_LID;lid++)
         switches_cache[id][lid] = -1;
   }

   for (id=0;id<MAX_ROUTE;id++) {
      route_cached[id] = 0;
      route_cache[id] = (char*) malloc(3*64);
   }


   route_cache_skiplen = 0;

   node_lid_cache = (char**) malloc(MAX_LID*sizeof(char*));

   for (id=0;id<MAX_LID;id++)
      node_lid_cache[id] = 0;
}


void reset_route_cache(int source_len)
{
   int id;
   for (id=0;id<MAX_ROUTE;id++)
      route_cached[id] = 0;
   route_cache_skiplen = source_len - 2;
//   printf("Reset route cache skip len %d\n", route_cache_skiplen);
}


void add_route_to_cache(int node_lid, ib_dr_path_t* path, char* desc, char* info, char* portinfo)
{
  int id = 0;

  char* cache = 0;

  int in_cache = 0;

  if (!use_cache) return;

  if (node_lid==0) {

     switch (path->cnt - route_cache_skiplen) {
        case 1:
           id = 0;
           break;
        case 2:
           id = 1 + path->p[route_cache_skiplen + 2];
           break;
        case 3:
           id = 37 + path->p[route_cache_skiplen + 2] + path->p[route_cache_skiplen + 3]*36;
           break;
        default:
           return;
     }

     if (id >= MAX_ROUTE) {
        fprintf(stderr,"Route id is too big %d  p0 %d p1 %d p2 %d \n",
                        id, path->p[route_cache_skiplen + 1], path->p[route_cache_skiplen + 2], path->p[route_cache_skiplen + 3]);
        return;
     }

     cache = route_cache[id];

     in_cache = route_cached[id];

     route_cached[id] = 1;

  } else {

     if (node_lid>=MAX_LID) return;

     if (node_lid_cache[node_lid] != 0) {
        in_cache = 1;
     } else {
        node_lid_cache[node_lid] = (char*) malloc(3*64);
     }
     cache = node_lid_cache[node_lid];

  }


  if (in_cache) {
//     printf("\nid = %d len %d already in cash ",id, path->cnt - route_cache_skiplen );
     if (memcmp(cache, desc, 64)!=0) fprintf(stderr, "desc mismatch\n");
     if (memcmp(cache+64, info, 64)!=0) fprintf(stderr, "info mismatch\n");
     if (memcmp(cache+128, portinfo, 64)!=0) fprintf(stderr, "portinfo mismatch\n");
  } else {
     memcpy(cache, desc, 64);
     memcpy(cache+64, info, 64);
     memcpy(cache+128, portinfo, 64);
  }
}


int get_cached_route(int node_lid, ib_dr_path_t* path, char* desc, char* info, char* portinfo)
{
  int id = 0;

  char* cache = 0;

  if (!use_cache) return 0;

  if (node_lid==0) {

     switch (path->cnt - route_cache_skiplen) {
        case 1:
           id = 0;
           break;
        case 2:
           id = 1 + path->p[route_cache_skiplen + 2];
           break;
        case 3:
           id = 37 + path->p[route_cache_skiplen + 2] + path->p[route_cache_skiplen + 3]*36;
           break;
        default:
           return 0;
     }

     if ((id >= MAX_ROUTE) || !route_cached[id]) return 0;

     cache = route_cache[id];
  } else {
     if (node_lid>=MAX_LID) return 0;

     cache = node_lid_cache[node_lid];
  }

  if (cache==0) return 0;

//  fprintf(stderr,"Extract route id %d  len %d  p0 %d p1 %d p2 %d from cache\n", id, path->cnt, path->p[0], path->p[1], path->p[2]);

  memcpy(desc, cache, 64);
  memcpy(info, cache+64, 64);
  memcpy(portinfo, cache+128, 64);
  return 1;
}



void add_cache(int sw_lid, int lid, int8_t* map)
{
   int id;

   if ((lid >= MAX_LID) || !use_cache) return;

   for (id=0; id<NUM_SWITCHES; id++)
      if ((switches_lids[id]==0) || (switches_lids[id]==sw_lid)) break;
   if (id==NUM_SWITCHES) return;

   switches_lids[id] = sw_lid;

//   fprintf(stderr,"add cache for sw %d lid %d\n", sw_lid, lid);

   memcpy(switches_cache[id] + (lid/64)*64, map, 64*sizeof(int8_t));
}

int get_cache(int sw_lid, int lid)
{
   int id;

   if ((lid >= MAX_LID) || !use_cache) return -1;

   for (id=0; id<NUM_SWITCHES; id++) {
      if (switches_lids[id]==0) return -1;
      if (switches_lids[id]==sw_lid)
         return switches_cache[id][lid];
   }
   return -1;
}


static char *node_name_map_file = NULL;
static nn_map_t *node_name_map = NULL;

typedef struct Port Port;
typedef struct Switch Switch;
typedef struct Node Node;

struct Port {
	Port *next;
	Port *remoteport;
	uint64_t portguid;
	int portnum;
	int lid;
	int lmc;
	int state;
	int physstate;
	char portinfo[64];
};

struct Switch {
	int linearcap;
	int mccap;
	int linearFDBtop;
	int fdb_base;
	int8_t fdb[64];
	char switchinfo[64];
};

struct Node {
	Node *htnext;
	Node *dnext;
	Port *ports;
	ib_portid_t path;
	int type;
	int dist;
	int numports;
	int upport;
	Node *upnode;
	uint64_t nodeguid;	/* also portguid */
	char nodedesc[64];
	char nodeinfo[64];
};

Node *nodesdist[MAXHOPS];
uint64_t target_portguid;

static int get_node(Node * node, Port * port, ib_portid_t * portid, int with_cache)
{


	void *pi = port->portinfo, *ni = node->nodeinfo, *nd = node->nodedesc;
	char *s, *e;

	int i, find_cache = 0;

    if (with_cache) {
       //printf("\n callid %d portid lid %d cnt %d ", with_cache, portid->lid, portid->drpath.cnt);
       //for (i=0;i<=portid->drpath.cnt;i++) printf("  [%d]=%d", i, portid->drpath.p[i]);

       find_cache = get_cached_route(portid->lid, &portid->drpath, nd, ni, pi);
    }

	if (!find_cache) {

  	   if (!smp_query_via(ni, portid, IB_ATTR_NODE_INFO, 0, timeout, srcport))
		return -1;

	   if (!smp_query_via(nd, portid, IB_ATTR_NODE_DESC, 0, timeout, srcport))
		return -1;

	   for (s = nd, e = s + 64; s < e; s++) {
		if (!*s) break;
		if (!isprint(*s)) *s = ' ';
	   }

	   if (!smp_query_via(pi, portid, IB_ATTR_PORT_INFO, 0, timeout, srcport))
		return -1;
        }


	mad_decode_field(ni, IB_NODE_GUID_F, &node->nodeguid);
	mad_decode_field(ni, IB_NODE_TYPE_F, &node->type);
	mad_decode_field(ni, IB_NODE_NPORTS_F, &node->numports);

	mad_decode_field(ni, IB_NODE_PORT_GUID_F, &port->portguid);
	mad_decode_field(ni, IB_NODE_LOCAL_PORT_F, &port->portnum);
	mad_decode_field(pi, IB_PORT_LID_F, &port->lid);
	mad_decode_field(pi, IB_PORT_LMC_F, &port->lmc);
	mad_decode_field(pi, IB_PORT_STATE_F, &port->state);

	if ((node->type == IB_NODE_SWITCH) && with_cache && !find_cache)
            add_route_to_cache(portid->lid, &portid->drpath, nd, ni, pi);

	DEBUG("portid %s: got node %" PRIx64 " '%s'", portid2str(portid),
	      node->nodeguid, node->nodedesc);
	return 0;
}

static int switch_lookup(Switch * sw, ib_portid_t * portid, int lid, int sw_lid)
{
        // printf("switch lid = %d tgt_lid = %d\n", sw_lid, lid);

	void *si = sw->switchinfo, *fdb = sw->fdb;

	int cached = get_cache(sw_lid, lid);

//      when commented out, cached value will be compared with real value
	if (cached>=0) return cached;

	if (!smp_query_via(si, portid, IB_ATTR_SWITCH_INFO, 0, timeout,
			   srcport))
		return -1;

	mad_decode_field(si, IB_SW_LINEAR_FDB_CAP_F, &sw->linearcap);
	mad_decode_field(si, IB_SW_LINEAR_FDB_TOP_F, &sw->linearFDBtop);

	if (lid >= sw->linearcap && lid > sw->linearFDBtop)
		return -1;

	if (!smp_query_via(fdb, portid, IB_ATTR_LINEARFORWTBL, lid / 64,
			   timeout, srcport))
		return -1;

	DEBUG("portid %s: forward lid %d to port %d",
	      portid2str(portid), lid, sw->fdb[lid % 64]);

        add_cache(sw_lid, lid, sw->fdb);

        if (cached>=0)
           if (sw->fdb[lid % 64] != cached)
              fprintf(stderr, "cache mismatch %d %d \n", sw->fdb[lid % 64], cached);

	return sw->fdb[lid % 64];
}

static int sameport(Port * a, Port * b)
{
	return a->portguid == b->portguid || (force && a->lid == b->lid);
}

static int extend_dpath(ib_dr_path_t * path, int nextport)
{
	if (path->cnt + 2 >= sizeof(path->p))
		return -1;
	++path->cnt;
	path->p[path->cnt] = (uint8_t) nextport;
	return path->cnt;
}

static void dump_endnode(int dump, char *prompt, Node * node, Port * port)
{
	char *nodename = NULL;

	if (!dump)
		return;
	if (dump == 1) {
		fprintf(f, "%s {0x%016" PRIx64 "}[%d]\n",
			prompt, node->nodeguid,
			node->type == IB_NODE_SWITCH ? 0 : port->portnum);
		return;
	}

	nodename =
	    remap_node_name(node_name_map, node->nodeguid, node->nodedesc);

	fprintf(f, "%s %s {0x%016" PRIx64 "} portnum %d lid %u-%u \"%s\"\n",
		prompt,
		(node->type <= IB_NODE_MAX ? node_type_str[node->type] : "???"),
		node->nodeguid,
		node->type == IB_NODE_SWITCH ? 0 : port->portnum, port->lid,
		port->lid + (1 << port->lmc) - 1, nodename);

	free(nodename);
}

static void dump_route(int dump, Node * node, int outport, Port * port)
{
	char *nodename = NULL;

	if (!dump && !ibverbose)
		return;

	nodename =
	    remap_node_name(node_name_map, node->nodeguid, node->nodedesc);

	if (dump == 1)
		fprintf(f, "[%d] -> {0x%016" PRIx64 "}[%d]\n",
			outport, port->portguid, port->portnum);
	else
		fprintf(f, "[%d] -> %s port {0x%016" PRIx64
			"}[%d] lid %u-%u \"%s\"\n", outport,
			(node->type <=
			 IB_NODE_MAX ? node_type_str[node->type] : "???"),
			port->portguid, port->portnum, port->lid,
			port->lid + (1 << port->lmc) - 1, nodename);

	free(nodename);
}

static int find_route(ib_portid_t * from, ib_portid_t * to, int dump, char** route)
{
	Node *node, fromnode, tonode, nextnode;
	Port *port, fromport, toport, nextport;
	Switch sw;
	int maxhops = MAXHOPS;
	int portnum, outport, rcnt;

	rcnt = 0;

	DEBUG("from %s", portid2str(from));

	if (get_node(&fromnode, &fromport, from, 3) < 0 ||
	    get_node(&tonode, &toport, to, 3) < 0) {
		IBWARN("can't reach to/from ports");
		if (!force)
			return -1;
		if (to->lid > 0)
			toport.lid = to->lid;
		IBWARN("Force: look for lid %d", to->lid);
	}

	node = &fromnode;
	port = &fromport;
	portnum = port->portnum;

	dump_endnode(dump, "From", node, port);
	if (route)
	   route[rcnt++] = remap_node_name(node_name_map, node->nodeguid, node->nodedesc);


	while (maxhops--) {
		if (port->state != 4)
			goto badport;

		if (sameport(port, &toport))
			break;	/* found */

		outport = portnum;
		if (node->type == IB_NODE_SWITCH) {
			DEBUG("switch node");
			if ((outport = switch_lookup(&sw, from, to->lid, port->lid)) < 0 ||
			    outport > node->numports)
				goto badtbl;

			if (extend_dpath(&from->drpath, outport) < 0)
				goto badpath;

			if (get_node(&nextnode, &nextport, from, (route ? 1 : 0)) < 0) {
				IBWARN("can't reach port at %s",
				       portid2str(from));
				return -1;
			}
			if (outport == 0) {
				if (!sameport(&nextport, &toport))
					goto badtbl;
				else
					break;	/* found SMA port */
			}
		} else if ((node->type == IB_NODE_CA) ||
			   (node->type == IB_NODE_ROUTER)) {
			int ca_src = 0;

			DEBUG("ca or router node");
			if (!sameport(port, &fromport)) {
				IBWARN
				    ("can't continue: reached CA or router port %"
				     PRIx64 ", lid %d", port->portguid,
				     port->lid);
				return -1;
			}
			/* we are at CA or router "from" - go one hop back to (hopefully) a switch */
			if (from->drpath.cnt > 0) {
				DEBUG("ca or router node - return back 1 hop");
				from->drpath.cnt--;
			} else {
				ca_src = 1;
				if (portnum
				    && extend_dpath(&from->drpath, portnum) < 0)
					goto badpath;
			}
			if (get_node(&nextnode, &nextport, from, (route ? 2 : 0)) < 0) {
				IBWARN("can't reach port at %s",
				       portid2str(from));
				return -1;
			}
			/* fix port num to be seen from the CA or router side */
			if (!ca_src)
				nextport.portnum =
				    from->drpath.p[from->drpath.cnt + 1];
		}
		port = &nextport;
		if (port->state != 4)
			goto badoutport;
		node = &nextnode;
		portnum = port->portnum;
		dump_route(dump, node, outport, port);
	   if (route) {
	      route[rcnt++] = remap_node_name(node_name_map, node->nodeguid, node->nodedesc);
	      // we believe that 4 nodes is exactly that we looking for and can accept that route
	      if ((rcnt==4) && optimize_routes && use_cache) break;
	   }
	}

	if (maxhops <= 0) {
		IBWARN("no route found after %d hops", MAXHOPS);
		return -1;
	}
	dump_endnode(dump, "To", node, port);
	return 0;

badport:
	IBWARN("Bad port state found: node \"%s\" port %d state %d",
	       clean_nodedesc(node->nodedesc), portnum, port->state);
	return -1;
badoutport:
	IBWARN("Bad out port state found: node \"%s\" outport %d state %d",
	       clean_nodedesc(node->nodedesc), outport, port->state);
	return -1;
badtbl:
	IBWARN
	    ("Bad forwarding table entry found at: node \"%s\" lid entry %d is %d (top %d)",
	     clean_nodedesc(node->nodedesc), to->lid, outport, sw.linearFDBtop);
	return -1;
badpath:
	IBWARN("Direct path too long!");
	return -1;
}

/**************************
 * MC span part
 */

#define HASHGUID(guid)		((uint32_t)(((uint32_t)(guid) * 101) ^ ((uint32_t)((guid) >> 32) * 103)))
#define HTSZ 137

static int insert_node(Node * new)
{
	static Node *nodestbl[HTSZ];
	int hash = HASHGUID(new->nodeguid) % HTSZ;
	Node *node;

	for (node = nodestbl[hash]; node; node = node->htnext)
		if (node->nodeguid == new->nodeguid) {
			DEBUG("node %" PRIx64 " already exists", new->nodeguid);
			return -1;
		}

	new->htnext = nodestbl[hash];
	nodestbl[hash] = new;

	return 0;
}

static int get_port(Port * port, int portnum, ib_portid_t * portid)
{
	char portinfo[64];
	void *pi = portinfo;

	port->portnum = portnum;

	if (!smp_query_via(pi, portid, IB_ATTR_PORT_INFO, portnum, timeout,
			   srcport))
		return -1;

	mad_decode_field(pi, IB_PORT_LID_F, &port->lid);
	mad_decode_field(pi, IB_PORT_LMC_F, &port->lmc);
	mad_decode_field(pi, IB_PORT_STATE_F, &port->state);
	mad_decode_field(pi, IB_PORT_PHYS_STATE_F, &port->physstate);

	VERBOSE("portid %s portnum %d: lid %d state %d physstate %d",
		portid2str(portid), portnum, port->lid, port->state,
		port->physstate);
	return 1;
}

static void link_port(Port * port, Node * node)
{
	port->next = node->ports;
	node->ports = port;
}

static int new_node(Node * node, Port * port, ib_portid_t * path, int dist)
{
	if (port->portguid == target_portguid) {
		node->dist = -1;	/* tag as target */
		link_port(port, node);
		dump_endnode(ibverbose, "found target", node, port);
		return 1;	/* found; */
	}

	/* BFS search start with my self */
	if (insert_node(node) < 0)
		return -1;	/* known switch */

	VERBOSE("insert dist %d node %p port %d lid %d", dist, node,
		port->portnum, port->lid);

	link_port(port, node);

	node->dist = dist;
	node->path = *path;
	node->dnext = nodesdist[dist];
	nodesdist[dist] = node;

	return 0;
}

static int switch_mclookup(Node * node, ib_portid_t * portid, int mlid,
			   char *map)
{
	Switch sw;
	char mdb[64];
	void *si = sw.switchinfo;
	uint16_t *msets = (uint16_t *) mdb;
	int maxsets, block, i, set;

	memset(map, 0, 256);

	if (!smp_query_via(si, portid, IB_ATTR_SWITCH_INFO, 0, timeout,
			   srcport))
		return -1;

	mlid -= 0xc000;

	mad_decode_field(si, IB_SW_MCAST_FDB_CAP_F, &sw.mccap);

	if (mlid >= sw.mccap)
		return -1;

	block = mlid / 32;
	maxsets = (node->numports + 15) / 16;	/* round up */

	for (set = 0; set < maxsets; set++) {
		if (!smp_query_via(mdb, portid, IB_ATTR_MULTICASTFORWTBL,
				   block | (set << 28), timeout, srcport))
			return -1;

		for (i = 0; i < 16; i++, map++) {
			uint16_t mask = ntohs(msets[mlid % 32]);
			if (mask & (1 << i))
				*map = 1;
			else
				continue;
			VERBOSE("Switch guid 0x%" PRIx64
				": mlid 0x%x is forwarded to port %d",
				node->nodeguid, mlid + 0xc000, i + set * 16);
		}
	}

	return 0;
}

/*
 * Return 1 if found, 0 if not, -1 on errors.
 */
static Node *find_mcpath(ib_portid_t * from, int mlid)
{
	Node *node, *remotenode;
	Port *port, *remoteport;
	char map[256];
	int r, i;
	int dist = 0, leafport = 0;
	ib_portid_t *path;

	DEBUG("from %s", portid2str(from));

	if (!(node = calloc(1, sizeof(Node))))
		IBERROR("out of memory");

	if (!(port = calloc(1, sizeof(Port))))
		IBERROR("out of memory");

	if (get_node(node, port, from, 0) < 0) {
		IBWARN("can't reach node %s", portid2str(from));
		return 0;
	}

	node->upnode = 0;	/* root */
	if ((r = new_node(node, port, from, 0)) > 0) {
		if (node->type != IB_NODE_SWITCH) {
			IBWARN("ibtracert from CA to CA is unsupported");
			return 0;	/* ibtracert from host to itself is unsupported */
		}

		if (switch_mclookup(node, from, mlid, map) < 0 || !map[0])
			return 0;
		return node;
	}

	for (dist = 0; dist < MAXHOPS; dist++) {

		for (node = nodesdist[dist]; node; node = node->dnext) {

			path = &node->path;

			VERBOSE("dist %d node %p", dist, node);
			dump_endnode(ibverbose, "processing", node,
				     node->ports);

			memset(map, 0, sizeof(map));

			if (node->type != IB_NODE_SWITCH) {
				if (dist)
					continue;
				leafport = path->drpath.p[path->drpath.cnt];
				map[port->portnum] = 1;
				node->upport = 0;	/* starting here */
				DEBUG("Starting from CA 0x%" PRIx64
				      " lid %d port %d (leafport %d)",
				      node->nodeguid, port->lid, port->portnum,
				      leafport);
			} else {	/* switch */

				/* if starting from a leaf port fix up port (up port) */
				if (dist == 1 && leafport)
					node->upport = leafport;

				if (switch_mclookup(node, path, mlid, map) < 0) {
					IBWARN("skipping bad Switch 0x%" PRIx64
					       "", node->nodeguid);
					continue;
				}
			}

			for (i = 1; i <= node->numports; i++) {
				if (!map[i] || i == node->upport)
					continue;

				if (dist == 0 && leafport) {
					if (from->drpath.cnt > 0)
						path->drpath.cnt--;
				} else {
					if (!(port = calloc(1, sizeof(Port))))
						IBERROR("out of memory");

					if (get_port(port, i, path) < 0) {
						IBWARN
						    ("can't reach node %s port %d",
						     portid2str(path), i);
						return 0;
					}

					if (port->physstate != 5) {	/* LinkUP */
						free(port);
						continue;
					}
#if 0
					link_port(port, node);
#endif

					if (extend_dpath(&path->drpath, i) < 0)
						return 0;
				}

				if (!(remotenode = calloc(1, sizeof(Node))))
					IBERROR("out of memory");

				if (!(remoteport = calloc(1, sizeof(Port))))
					IBERROR("out of memory");

				if (get_node(remotenode, remoteport, path, 0) < 0) {
					IBWARN
					    ("NodeInfo on %s port %d failed, skipping port",
					     portid2str(path), i);
					path->drpath.cnt--;	/* restore path */
					free(remotenode);
					free(remoteport);
					continue;
				}

				remotenode->upnode = node;
				remotenode->upport = remoteport->portnum;
				remoteport->remoteport = port;

				if ((r = new_node(remotenode, remoteport, path,
						  dist + 1)) > 0)
					return remotenode;

				if (r == 0)
					dump_endnode(ibverbose, "new remote",
						     remotenode, remoteport);
				else if (remotenode->type == IB_NODE_SWITCH)
					dump_endnode(2,
						     "ERR: circle discovered at",
						     remotenode, remoteport);

				path->drpath.cnt--;	/* restore path */
			}
		}
	}

	return 0;		/* not found */
}

static uint64_t find_target_portguid(ib_portid_t * to)
{
	Node tonode;
	Port toport;

	if (get_node(&tonode, &toport, to, 0) < 0) {
		IBWARN("can't find to port\n");
		return -1;
	}

	return toport.portguid;
}

static void dump_mcpath(Node * node, int dumplevel)
{
	char *nodename = NULL;

	if (node->upnode)
		dump_mcpath(node->upnode, dumplevel);

	nodename =
	    remap_node_name(node_name_map, node->nodeguid, node->nodedesc);

	if (!node->dist) {
		printf("From %s 0x%" PRIx64 " port %d lid %u-%u \"%s\"\n",
		       (node->type <=
			IB_NODE_MAX ? node_type_str[node->type] : "???"),
		       node->nodeguid, node->ports->portnum, node->ports->lid,
		       node->ports->lid + (1 << node->ports->lmc) - 1,
		       nodename);
		goto free_name;
	}

	if (node->dist) {
		if (dumplevel == 1)
			printf("[%d] -> %s {0x%016" PRIx64 "}[%d]\n",
			       node->ports->remoteport->portnum,
			       (node->type <=
				IB_NODE_MAX ? node_type_str[node->type] :
				"???"), node->nodeguid, node->upport);
		else
			printf("[%d] -> %s 0x%" PRIx64 "[%d] lid %u \"%s\"\n",
			       node->ports->remoteport->portnum,
			       (node->type <=
				IB_NODE_MAX ? node_type_str[node->type] :
				"???"), node->nodeguid, node->upport,
			       node->ports->lid, nodename);
	}

	if (node->dist < 0)
		/* target node */
		printf("To %s 0x%" PRIx64 " port %d lid %u-%u \"%s\"\n",
		       (node->type <=
			IB_NODE_MAX ? node_type_str[node->type] : "???"),
		       node->nodeguid, node->ports->portnum, node->ports->lid,
		       node->ports->lid + (1 << node->ports->lmc) - 1,
		       nodename);

free_name:
	free(nodename);
}

static int resolve_lid(ib_portid_t * portid, const void *srcport)
{
	uint8_t portinfo[64];
	uint16_t lid;

	if (!smp_query_via(portinfo, portid, IB_ATTR_PORT_INFO, 0, 0, srcport))
		return -1;
	mad_decode_field(portinfo, IB_PORT_LID_F, &lid);

	ib_portid_set(portid, lid, 0, 0);

	return 0;
}


static int process_opt(void *context, int ch, char *optarg)
{
	switch (ch) {
	case 1:
		node_name_map_file = strdup(optarg);
		break;
   case 'l':
      fnodes_list = strdup(optarg);
      break;
   case 't':
		maxnumlids = strtoul(optarg, 0, 0);
		break;
   case 'o':
		optimize_routes = 1;
		break;
   case 'c':
                use_cache = 1;
                break;

	case 'm':
		multicast++;
		mlid = strtoul(optarg, 0, 0);
		break;
	case 'f':
		force++;
		break;
	case 'n':
		dumplevel = 1;
		break;
	default:
		return -1;
	}
	return 0;
}

/** Parameter optimize enables skip of many entries in the routing table.
 *  This is possible only when all routes to the node from same switch are the same.
 *  Up to now it was true, therefore one do not need to discover these routes many time again.
 *  Also routes between nodes in the same switch are not stored.
 *  As result, only routes from first node in the switch are stored, the rest is skipped. */

void process_nodes_list(const char* fname, int maxnumlids, int optimize)
{
   FILE *f;
   char sbuf[100];
   char* matrix[1000][1000];
   char lids[1000][30];
   char names[1000][30];
   int lmcs[1000];
   ib_portid_t all_ports[1000];

   int numnodes, n1, n2, n3, nn, cnt, lll, percent, lastpercent, spine_mask;
   int numlids;
   int skip_node;
   char* route[100];
   char* mybuf;
   ib_portid_t my_portid = { 0 };
   ib_portid_t src_portid = { 0 };
   ib_portid_t dest_portid = { 0 };

   for (n1=0;n1<1000;n1++) {
      for (n2=0;n2<1000;n2++)
         matrix[n1][n2] = 0;
   }

   f = fopen (fname , "r");
   if (f == NULL) { printf("Error opening file %s\n", fname); return; }
   numnodes = 0;

   while (fgets (sbuf, sizeof(sbuf), f)) {

      if (sscanf(sbuf, "%s %s %d", names[numnodes], lids[numnodes], &(lmcs[numnodes])) != 3) {
         printf("Wrong line format %s", sbuf);
         return;
      }
      numnodes++;
   }

   fclose(f);

   memset(all_ports, 0, sizeof(all_ports));

   fprintf(stderr, "Total number of nodes = %d\n", numnodes);

   for (n1=0;n1<numnodes;n1++) {
     if (ib_resolve_portid_str_via(all_ports+n1, lids[n1], ibd_dest_type, ibd_sm_id, srcport) < 0)
       IBERROR("can't resolve source port %s on node %s", lids[n1], names[n1]);
   }

   fprintf(stderr, "Progress   0%s","%");

   percent = 0;
   lastpercent = 0;

   for (n1=0;n1<numnodes;n1++) {
      memcpy(&src_portid, all_ports+n1, sizeof(src_portid));

      memset(&my_portid, 0, sizeof(my_portid));

      if (find_route(&my_portid, &src_portid, 0, 0) < 0)
         IBERROR("can't find a route to the src port");

      reset_route_cache(my_portid.drpath.cnt);

      // if optimize enabled, one can skip node from switch where routes from other node were measured already
      skip_node = 0;
      if (optimize)
         for (n2=0;n2<numnodes;n2++) {
            if (n1==n2) continue;
            if (matrix[n1][n2] != 0) skip_node = 1;
         }

      if (!skip_node)
      for (n2=0;n2<numnodes;n2++) {
         if (n1==n2) continue;
         //printf("Resolve %s -> %s  %s -> %s\n", names[n1], names[n2], lids[n1], lids[n2]);


         numlids = 1 << lmcs[n2];

         spine_mask = 0;

         if (numlids > maxnumlids) {
            numlids = maxnumlids;
         }


         if (matrix[n1][n2] != 0) {

            printf("%9s -> %9s  %5s -> %5s ", names[n1], names[n2], lids[n1], lids[n2]);

            printf("%s\n", matrix[n1][n2]);

         } else

            // we make loop over all lids of target node
            for (lll=0; lll<numlids; lll++) {

               src_portid = my_portid;
               memcpy(&dest_portid, all_ports+n2, sizeof(dest_portid));
               dest_portid.lid += lll;


               printf("%9s -> %9s  %5s -> %5d ", names[n1], names[n2], lids[n1], dest_portid.lid);



               //         if (resolve_lid(&src_portid, NULL) < 0)
               //            IBERROR("cannot resolve lid for source port %s", lids[n1]);
               //         if (resolve_lid(&dest_portid, NULL) < 0)
               //            IBERROR("cannot resolve lid for target port %s", lids[n2]);


               for (nn=0;nn<100;nn++)
                  route[nn] = 0;

               if (find_route(&src_portid, &dest_portid, 0 /*dumplevel*/, route) < 0) {
                  fprintf(stderr, "can't find a route from src to dest %s %s\n", names[n1], names[n2]);
                  printf("\n");
                  continue;
               }

               cnt = 0;

               for (nn=0;nn<100;nn++) {
                  if (route[nn]) {
                     cnt++;

                     if (strstr(route[nn],"ibswitch")==route[nn]+4) {
                        mybuf = (char*) malloc(11);
                        if (mybuf) {
                           memcpy(mybuf, route[nn]+4, 10);
                           mybuf[10] = 0;
                           free(route[nn]);
                           route[nn] = mybuf;
                        } else {
                           printf("Memory allocation failure\n");
                        }
                     } else
                     if (strstr(route[nn],"ibspine")==route[nn]+4) {
                        mybuf = (char*) malloc(10);
                        if (mybuf) {
                           memcpy(mybuf, route[nn]+4, 9);
                           mybuf[9] = 0;
                           free(route[nn]);
                           route[nn] = mybuf;
                        } else {
                           printf("Memory allocation failure\n");
                        }
                     }

                     //               printf("   item %d = %s\n", nn, route[nn]);

                  }
               }

               // we add artificially last node - we believe it works correctly
               if ((cnt==4) && optimize_routes && use_cache) {
                  mybuf = (char*) malloc(30);
                  strncpy(mybuf, names[n2], 30);
                  route[cnt] = mybuf;
                  cnt++;
               }


               if ((cnt!=3) && (cnt!=5)) {
                  fprintf(stderr, "Wrong number of nodes %d in route %s -> %s\n", cnt,  names[n1], names[n2]);
                  printf("\n");
                  lll = numlids; // stop looping over target lids
               } else

               if (strstr(route[0], names[n1])!=route[0]) {
                  fprintf(stderr, "First node %s is not found in the beginning of the route %s\n", names[n1], route[0]);
                  printf("\n");
                  lll = numlids; // stop looping over target lids
               } else

               if (strstr(route[cnt-1], names[n2])!=route[cnt-1]) {
                  fprintf(stderr, "Second node %s is not found in the end of the route %s\n", names[n2], route[cnt-1]);
                  printf("\n");
                  lll = numlids; // stop looping over target lids
               } else

               if (cnt==3) {
                  printf("%s\n", route[1]);
                  matrix[n1][n2] = route[1];
                  if (matrix[n2][n1]==0) matrix[n2][n1] = matrix[n1][n2]; // single route is single route
                  route[1] = 0;
                  lll = numlids; // same switch - no need to scan other lids
               }  else {
                  // thid is longest route with 3 switches in between
                  printf("%s %s %s\n", route[1], route[2], route[3]);

                  if (strstr(route[2],"ibspine")==route[2]) {
                     spine_mask |= 1 << (atoi(route[2] + 7) - 1);
                     if ((spine_mask == 0xfff) && optimize) lll = numlids; // all spines are detected - no need to scan further lids
                  }
               }

               for (nn=0;nn<100;nn++) {
                  if (route[nn]) {
                     free(route[nn]);
                     route[nn] = 0;
                  }
               }

            } // end loop for lll - all target lids

      } // end loop for n2

      // detect all nodes, connected to single switch and fill matrix complete for that switch
      for (n2=0;n2<numnodes;n2++) {
         if ((n2==n1) || (matrix[n1][n2]==0)) continue;
         for (n3=0;n3<numnodes;n3++) {
            if ((n3==n1) || (n3==n2) || (matrix[n1][n3]==0)) continue;
            if (matrix[n2][n3]==0) matrix[n2][n3]=matrix[n1][n2];
            if (matrix[n3][n2]==0) matrix[n3][n2]=matrix[n1][n2];
         }
      }

      percent = n1*100 / (numnodes - 1);
      if (percent > lastpercent) {
         fprintf(stderr, "\b\b\b\b%3d%s", percent,"%");
         fflush(stderr);
         lastpercent = percent;
      }

   } // end loop for n1

   fprintf(stderr, "\nExploring done\n", numnodes);


}



int main(int argc, char **argv)
{
	int mgmt_classes[3] =
	    { IB_SMI_CLASS, IB_SMI_DIRECT_CLASS, IB_SA_CLASS };
	ib_portid_t my_portid = { 0 };
	ib_portid_t src_portid = { 0 };
	ib_portid_t dest_portid = { 0 };
	Node *endnode;


	const struct ibdiag_opt opts[] = {
		{"force", 'f', 0, NULL, "force"},
		{"no_info", 'n', 0, NULL, "simple format"},
		{"mlid", 'm', 1, "<mlid>", "multicast trace of the mlid"},
		{"node-name-map", 1, 1, "<file>", "node name map file"},
      {"list", 'l', 1, "<file>", "nodes and lids list"},
      {"ttt", 't', 1, "<num>", "number of lids per node"},
      {"optimize", 'o', 0, NULL, "optimize (reduce) number of searched routes"},
      {"cache", 'c', 0, NULL, "use caching for different information"},
		{0}
	};
	char usage_args[] = "<src-addr> <dest-addr>";
	const char *usage_examples[] = {
		"- Unicast examples:",
		"4 16\t\t\t# show path between lids 4 and 16",
		"-n 4 16\t\t# same, but using simple output format",
		"-G 0x8f1040396522d 0x002c9000100d051\t# use guid addresses",
		"-l nodes_list_filename -t 4 -o",
		" - Multicast examples:",
		"-m 0xc000 4 16\t# show multicast path of mlid 0xc000 between lids 4 and 16",
		NULL,
	};

	ibdiag_process_opts(argc, argv, NULL, NULL, opts, process_opt,
			    usage_args, usage_examples);

	f = stdout;
	argc -= optind;
	argv += optind;

	if (argc < 2)
		ibdiag_show_usage();

	if (ibd_timeout)
		timeout = ibd_timeout;

	srcport = mad_rpc_open_port(ibd_ca, ibd_ca_port, mgmt_classes, 3);
	if (!srcport)
		IBERROR("Failed to open '%s' port '%d'", ibd_ca, ibd_ca_port);

	node_name_map = open_node_name_map(node_name_map_file);

   init_cache();

   if (fnodes_list!=0) {
      fprintf(stderr, "Processing special file %s, numlids %d, optimize %d, cache %d  to extract complete routing\n", fnodes_list, maxnumlids, optimize_routes, use_cache);

      process_nodes_list(fnodes_list, maxnumlids, optimize_routes);

      close_node_name_map(node_name_map);

      mad_rpc_close_port(srcport);

      exit(0);

   }


	if (ib_resolve_portid_str_via(&src_portid, argv[0], ibd_dest_type,
				      ibd_sm_id, srcport) < 0)
		IBERROR("can't resolve source port %s", argv[0]);

	if (ib_resolve_portid_str_via(&dest_portid, argv[1], ibd_dest_type,
				      ibd_sm_id, srcport) < 0)
		IBERROR("can't resolve destination port %s", argv[1]);

	if (ibd_dest_type == IB_DEST_DRPATH) {
		if (resolve_lid(&src_portid, NULL) < 0)
			IBERROR("cannot resolve lid for port \'%s\'",
				portid2str(&src_portid));
		if (resolve_lid(&dest_portid, NULL) < 0)
			IBERROR("cannot resolve lid for port \'%s\'",
				portid2str(&dest_portid));
	}

	if (dest_portid.lid == 0 || src_portid.lid == 0) {
		IBWARN("bad src/dest lid");
		ibdiag_show_usage();
	}

	if (ibd_dest_type != IB_DEST_DRPATH) {
		/* first find a direct path to the src port */
		if (find_route(&my_portid, &src_portid, 0, 0) < 0)
			IBERROR("can't find a route to the src port");

		src_portid = my_portid;
	}

	if (!multicast) {
		if (find_route(&src_portid, &dest_portid, dumplevel ,0) < 0)
			IBERROR("can't find a route from src to dest");
		exit(0);
	} else {
		if (mlid < 0xc000)
			IBWARN("invalid MLID; must be 0xc000 or larger");
	}

	if (!(target_portguid = find_target_portguid(&dest_portid)))
		IBERROR("can't reach target lid %d", dest_portid.lid);

	if (!(endnode = find_mcpath(&src_portid, mlid)))
		IBERROR("can't find a multicast route from src to dest");

	/* dump multicast path */
	dump_mcpath(endnode, dumplevel);

	close_node_name_map(node_name_map);

	mad_rpc_close_port(srcport);

	exit(0);
}
