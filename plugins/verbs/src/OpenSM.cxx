// $Id$

/************************************************************
 * The Data Acquisition Backbone Core (DABC)                *
 ************************************************************
 * Copyright (C) 2009 -                                     *
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH      *
 * Planckstr. 1, 64291 Darmstadt, Germany                   *
 * Contact:  http://dabc.gsi.de                             *
 ************************************************************
 * This software can be used under the GPL license          *
 * agreements as stated in LICENSE.txt file                 *
 * which is part of the distribution.                       *
 ************************************************************/

#include "verbs/OpenSM.h"

#include <infiniband/opensm/osm_helper.h>
#include <cstdlib>

#include "dabc/logging.h"
#include "verbs/QueuePair.h"


ib_gid_t TOsm_good_mgid = {
    {
      0xFF, 0x12, 0xA0, 0x1C,
      0xFE, 0x80, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
      0x88, 0x99, 0xaa, 0xbb
    }
  };

/*

int ibv_sa_init_ah_from_mcmember(
                 struct ibv_context *device,
                 uint8_t port_num,
             struct ibv_sa_net_mcmember_rec *mc_rec,
             struct ibv_ah_attr *ah_attr)
{
   struct ibv_sa_mcmember_rec rec;
   int ret;

   ibv_sa_unpack_mcmember_rec(&rec, mc_rec);

   ret = get_gid_index(device, port_num, &rec.port_gid);
   if (ret < 0)
      return ret;

   memset(ah_attr, 0, sizeof *ah_attr);
   ah_attr->dlid = ntohs(rec.mlid);
   ah_attr->sl = rec.sl;
   ah_attr->port_num = port_num;
   ah_attr->static_rate = rec.rate;
0
   ah_attr->is_global = 1;
   ah_attr->grh.dgid = rec.mgid;

   ah_attr->grh.sgid_index = (uint8_t) ret;
   ah_attr->grh.flow_label = ntohl(rec.flow_label);
   ah_attr->grh.hop_limit = rec.hop_limit;
   ah_attr->grh.traffic_class = rec.traffic_class;
   return 0;
}

*/


void TOsm_query_callback(osmv_query_res_t * p_rec )
{
  verbs::OpenSM* ctxt = (verbs::OpenSM*) p_rec->query_context;

  ctxt->SetResult(p_rec);

  if( p_rec->status != IB_SUCCESS )
    if ( p_rec->status != IB_INVALID_PARAMETER )
       EOUT("TOsm_query_result: Error on query (%s)", ib_get_err_str( p_rec->status));
}



// **************************************************

verbs::OpenSM::OpenSM() :
   f_vendor(0),
   f_log(0)
{
   memset(&f_local_port, 0, sizeof(f_local_port));
   memset(&f_bind_handle, 0, sizeof(f_bind_handle));
   memset(&f_mad_pool, 0, sizeof(f_mad_pool));
}

verbs::OpenSM::~OpenSM()
{
}

bool verbs::OpenSM::Init()
{
   ib_api_status_t status;

   f_log = (osm_log_t*) std::malloc (sizeof(osm_log_t));
   if (!f_log) {
      EOUT("Problem with memory allocation");
      return false;
   }

   memset(f_log, 0, sizeof(osm_log_t));
   osm_log_construct(f_log);

   status = osm_log_init_v2(f_log, TRUE, 0x0001, NULL, 0, TRUE );
//   status = osm_log_init(f_log, TRUE, 0x0001, NULL, TRUE );
   if( status != IB_SUCCESS ) {
      EOUT("Problem with osm_log_init_v2");
      return false;
   }

   /* but we do not want any extra stuff here */
   osm_log_set_level(f_log, OSM_LOG_ERROR/* | OSM_LOG_INFO */);

   //osm_log(f_log, OSM_LOG_INFO, "doing init");

   f_vendor = osm_vendor_new(f_log, 10);

   if (f_vendor==0) {
      //osm_log(f_log, OSM_LOG_ERROR, "Cannot create vendor\n" );
      EOUT("Cannot create vendor");
      return false;
   }


// the only change from OFED 1.2 to 1.4
//   status = osm_mad_pool_init( &f_mad_pool, f_log );

   osm_mad_pool_construct( &f_mad_pool);
   status = osm_mad_pool_init( &f_mad_pool);
   if( status != IB_SUCCESS ) {
      osm_log(f_log, OSM_LOG_ERROR, "problem with mad_pool\n" );
      return false;
   }

   return BindPort();
}

bool verbs::OpenSM::BindPort()
{
   uint32_t port_index = 0; // we always use first port
   ib_api_status_t status;
   uint32_t num_ports = OSM_GUID_ARRAY_SIZE;
   ib_port_attr_t attr_array[OSM_GUID_ARRAY_SIZE];

  /*
   * Call the transport layer for a list of local port
   * GUID values.
   */
   status = osm_vendor_get_all_port_attr( f_vendor, attr_array, &num_ports );
   if( status != IB_SUCCESS ) {
      EOUT("osm_vendor_get_all_port_attr fails");
      return false;
   }

/*
   DOUT1("Total number of ports = %u", num_ports);
   for (uint32_t n=0;n<num_ports;n++)
     DOUT1("  Port %u: %016lx ", n, cl_ntoh64(attr_array[n].port_guid);
*/

   /** Copy the port info for the selected port. */
   memcpy( &f_local_port, &(attr_array[port_index]), sizeof( f_local_port ) );

//   DOUT1("Try to bind to %016lx", cl_ntoh64(f_local_port.port_guid);

   f_bind_handle = osmv_bind_sa(f_vendor, &f_mad_pool, f_local_port.port_guid);

   if(f_bind_handle == OSM_BIND_INVALID_HANDLE ) {
      EOUT("Unable to bind to SA");
      return false;
   }

   return true;
}


bool verbs::OpenSM::Close()
{
   DOUT4( "osm_vendor_delete commented");
//   osm_vendor_delete(&f_vendor);
   DOUT4( "osm_mad_pool_destroy");
   osm_mad_pool_destroy(&f_mad_pool);
   DOUT4( "osm_log_destroy");
   osm_log_destroy(f_log);
   return true;
}


bool verbs::OpenSM::Query_SA(osmv_query_type_t query_type,
                             uint64_t          comp_mask,
                             ib_member_rec_t   *mc_req,
                             ib_sa_mad_t       *res)
{
  ib_api_status_t status = IB_SUCCESS;

  osmv_user_query_t user;
  osmv_query_req_t req;

  /*
   * Do a blocking query for this record in the subnet.
   *
   * The query structures are locals.
   */
  memset( &req, 0, sizeof( req ) );
  memset( &user, 0, sizeof( user ) );
  memset( res, 0, sizeof(ib_sa_mad_t ) );

  user.p_attr = mc_req;
  user.comp_mask = comp_mask;

  req.query_type = query_type;
  req.timeout_ms = 1000; //p_osmt->opt.transaction_timeout;
  req.retry_cnt = 3; // p_osmt->opt.retry_count;
  req.flags = OSM_SA_FLAGS_SYNC;
  req.query_context = this;
  req.pfn_query_cb = TOsm_query_callback;
  req.p_query_input = &user;
  req.sm_key = 0;

  status = osmv_query_sa( f_bind_handle, &req );

  if ( status != IB_SUCCESS ) {
    EOUT("osmv_query_sa error -(%s)", ib_get_err_str(status));
    goto Exit;
  }

  /* ok it worked */
  memcpy(res, osm_madw_get_mad_ptr(fLastResult.p_result_madw), sizeof(ib_sa_mad_t));

  status = fLastResult.status;

  if((status != IB_SUCCESS ) && (status == IB_REMOTE_ERROR))
      EOUT( "osmv_query_sa remote error");
//               ib_get_mad_status_str( osm_madw_get_mad_ptr( context.result.p_result_madw ) ) );

 Exit:
  /*
   * Return the IB query MAD to the pool as necessary.
   */
  if(fLastResult.p_result_madw != NULL )
  {
    osm_mad_pool_put( &f_mad_pool, fLastResult.p_result_madw );
    fLastResult.p_result_madw = NULL;
  }

  return (status==IB_SUCCESS);
}

bool verbs::OpenSM::ManageMultiCastGroup(bool isadd,
                                uint8_t* mgid_raw,
                                uint16_t* mlid)
{
   if ((mgid_raw==0) || (mlid==0)) return false;

   ib_member_rec_t mc_req_rec;
   uint64_t     comp_mask=0;
   ib_sa_mad_t  res_sa_mad;
   ib_member_rec_t *p_mc_res;

   memset(&mc_req_rec, 0, sizeof(ib_member_rec_t));

   // set multicast group id
//   memcpy(&mc_req_rec.mgid, mgid, sizeof(ib_gid_t));
   memcpy(&mc_req_rec.mgid.raw, mgid_raw, sizeof(mc_req_rec.mgid.raw));

  DOUT4("  %s multicast port %016lx", (isadd ? "Add to" : "Remove from"), cl_ntoh64(f_local_port.port_guid));

   // set port id, which will be add/removed from multicast group
  memcpy(&mc_req_rec.port_gid.unicast.interface_id,
         &f_local_port.port_guid,
         sizeof(f_local_port.port_guid));

   // lets use our own prefix ????
   mc_req_rec.port_gid.unicast.prefix = CL_HTON64(0xFE80000000000000ULL);

   // set q key, for now I see only 0 in osmt_multicast ???
   mc_req_rec.qkey = cl_hton32(VERBS_DEFAULT_QKEY);

   // as I see, mlid (multicast lid) is 0 and will be returned after request
   mc_req_rec.mlid = 0;

   // we try exactly 2048, probably one should use less restrictive construction
   // check when group is executed
   mc_req_rec.mtu = IB_MTU_LEN_2048 | IB_PATH_SELECTOR_EXACTLY << 6;
//   mc_req_rec.mtu = IB_MTU_LEN_256 | IB_PATH_SELECTOR_GREATER_THAN << 6;

   // everywhere 0 as I can see
   mc_req_rec.tclass = 0;

   mc_req_rec.pkey = cl_hton16(0xffff);
//   mc_req_rec.pkey = cl_hton16(2); //cl_hton16(0xffff);

   // this is our actual physical link datarate
   mc_req_rec.rate = IB_LINK_WIDTH_ACTIVE_4X;

   // try to set packet life time grater than 0
   // there is also comment, that just 0 should work
   mc_req_rec.pkt_life = 0 | IB_PATH_SELECTOR_GREATER_THAN << 6;

   // did not used here
   mc_req_rec.sl_flow_hop = 0;

   // if I understood correctly scope = 0x02 is local scope (that ever it means),
   // no other values are observed in osmt_multicast
   // For state some other flags are possible: IB_MC_REC_STATE_NON_MEMBER or IB_MC_REC_STATE_SEND_ONLY_MEMBER
   // Try as it is mostly used in examples.
   mc_req_rec.scope_state = ib_member_set_scope_state(0x02, IB_MC_REC_STATE_FULL_MEMBER);

   // never appears in example
   mc_req_rec.proxy_join = 0;

   comp_mask =
    IB_MCR_COMPMASK_MGID |     // there is also IB_MCR_COMPMASK_GID==IB_MCR_COMPMASK_MGID
    IB_MCR_COMPMASK_PORT_GID |
    IB_MCR_COMPMASK_QKEY |
    IB_MCR_COMPMASK_PKEY |
    IB_MCR_COMPMASK_SL |
    IB_MCR_COMPMASK_FLOW |
    IB_MCR_COMPMASK_JOIN_STATE |
    IB_MCR_COMPMASK_TCLASS |  /*  all above are required */
    IB_MCR_COMPMASK_MTU_SEL |
    IB_MCR_COMPMASK_MTU |
    IB_MCR_COMPMASK_SCOPE | // used only once, probably should not be specified
    IB_MCR_COMPMASK_LIFE |
    IB_MCR_COMPMASK_LIFE_SEL;

   bool res = Query_SA(isadd ? OSMV_QUERY_UD_MULTICAST_SET :
                               OSMV_QUERY_UD_MULTICAST_DELETE,
                       comp_mask,
                       &mc_req_rec,
                       &res_sa_mad);

   if (!res) return false;

   p_mc_res = (ib_member_rec_t*) ib_sa_mad_get_payload_ptr(&res_sa_mad);

   *mlid = cl_ntoh16(p_mc_res->mlid);

   return true;
}


bool verbs::OpenSM::PrintAllMulticasts()
{
  ib_api_status_t status = IB_SUCCESS;
  osmv_user_query_t user;
  osmv_query_req_t req;
  ib_member_rec_t *p_rec;
  uint32_t i,num_recs = 0;

  memset( &req, 0, sizeof( req ) );
  memset( &user, 0, sizeof( user ) );

  user.method = 0; //IB_MAD_METHOD_GET;
  user.attr_id = IB_MAD_ATTR_MCMEMBER_RECORD;
  user.attr_offset = ib_get_attr_offset( sizeof( ib_member_rec_t ) );
  user.comp_mask = 0;

  req.query_type = OSMV_QUERY_USER_DEFINED;
  req.timeout_ms = 1000;
  req.retry_cnt = 3;
  req.flags = OSM_SA_FLAGS_SYNC;
  req.query_context = this;
  req.pfn_query_cb = TOsm_query_callback;
  req.p_query_input = &user;
  req.sm_key = 0;

  status = osmv_query_sa(f_bind_handle, &req );

  if( status != IB_SUCCESS ) {
     EOUT("PrintAllMulticasts error"
             "osmv_query_sa failed (%s)\n", ib_get_err_str( status ));
     goto Exit;
  }

  status = fLastResult.status;

  if(status != IB_SUCCESS) {
    EOUT("ib_query failed (%s)", ib_get_err_str( status ) ));
    if(status==IB_REMOTE_ERROR) EOUT("Remote error");
    goto Exit;
  }

  num_recs = fLastResult.result_cnt;
  DOUT0("Number of Multicast records: %u", num_recs);

  for( i = 0; i < num_recs; i++ ) {
    p_rec = (ib_member_rec_t*) osmv_get_query_result( fLastResult.p_result_madw, i );

/*    DOUT0( "   Rec %2d: "
             "mgid: %02x%02x%02x%02x%02x%02x%02x%02x : %02x%02x%02x%02x%02x%02x%02x%02x"
             "   mlid: %4x  mtu: %x  rate: %x", i,
              p_rec->mgid.raw[0], p_rec->mgid.raw[1], p_rec->mgid.raw[2], p_rec->mgid.raw[3],
              p_rec->mgid.raw[4], p_rec->mgid.raw[5], p_rec->mgid.raw[6], p_rec->mgid.raw[7],
              p_rec->mgid.raw[8], p_rec->mgid.raw[9], p_rec->mgid.raw[10], p_rec->mgid.raw[11],
              p_rec->mgid.raw[12], p_rec->mgid.raw[13], p_rec->mgid.raw[14], p_rec->mgid.raw[15],
              p_rec->mlid, p_rec->mtu, p_rec->rate));
*/

    DOUT0( "   Rec %2d: "
             "mgid: %016lx : %016lx  mlid: %04x  mtu: %x  pkey: %x rate: %x", i,
             cl_ntoh64(p_rec->mgid.unicast.prefix),
             cl_ntoh64(p_rec->mgid.unicast.interface_id),
             cl_ntoh16(p_rec->mlid), p_rec->mtu, cl_ntoh16(p_rec->pkey),  p_rec->rate));

    if (i==num_recs-1)
       osm_dump_mc_record( f_log, p_rec, OSM_LOG_ERROR);
  }

 Exit:
  if( fLastResult.p_result_madw != NULL )
  {
    osm_mad_pool_put( &f_mad_pool, fLastResult.p_result_madw );
    fLastResult.p_result_madw = NULL;
  }

  return ( status==IB_SUCCESS );
}


bool verbs::OpenSM::QueryMyltucastGroup(uint8_t* mgid, uint16_t& mlid)
{
  bool res = false;

  ib_api_status_t status = IB_SUCCESS;
  osmv_user_query_t user;
  osmv_query_req_t req;
  ib_member_rec_t *p_rec;
  uint32_t i,num_recs = 0;

  memset( &req, 0, sizeof( req ) );
  memset( &user, 0, sizeof( user ) );

  user.method = 0; //IB_MAD_METHOD_GET;
  user.attr_id = IB_MAD_ATTR_MCMEMBER_RECORD;
  user.attr_offset = ib_get_attr_offset( sizeof( ib_member_rec_t ) );
  user.comp_mask = 0;

  req.query_type = OSMV_QUERY_USER_DEFINED;
  req.timeout_ms = 1000;
  req.retry_cnt = 3;
  req.flags = OSM_SA_FLAGS_SYNC;
  req.query_context = this;
  req.pfn_query_cb = TOsm_query_callback;
  req.p_query_input = &user;
  req.sm_key = 0;

  status = osmv_query_sa(f_bind_handle, &req );

  if( status != IB_SUCCESS ) {
     EOUT("osmv_query_sa failed (%s)", ib_get_err_str(status));
     goto Exit;
  }

  status = fLastResult.status;

  if(status != IB_SUCCESS) {
    EOUT("PrintAllMulticasts error"
           "ib_query failed (%s)\n", ib_get_err_str( status ) ));
    if(status==IB_REMOTE_ERROR) EOUT("PrintAllMulticasts Remote error");
    goto Exit;
  }

  num_recs = fLastResult.result_cnt;

  for( i = 0; i < num_recs; i++ ) {
    p_rec = (ib_member_rec_t*) osmv_get_query_result( fLastResult.p_result_madw, i );

    if (memcmp(p_rec->mgid.raw, mgid, sizeof(uint8_t)*16)!=0) continue;

    mlid = cl_ntoh16(p_rec->mlid);
    res = true;
    break;
  }

 Exit:
  if( fLastResult.p_result_madw != NULL )
  {
    osm_mad_pool_put( &f_mad_pool, fLastResult.p_result_madw );
    fLastResult.p_result_madw = NULL;
  }

  return res;
}
