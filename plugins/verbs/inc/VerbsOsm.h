#ifndef DABC_VerbsOsm
#define DABC_VerbsOsm

#define OSM_GUID_ARRAY_SIZE 10

#define OSM_VENDOR_INTF_OPENIB

#include <vendor/osm_vendor_api.h>
#include <vendor/osm_vendor_sa_api.h>

namespace dabc {

   class VerbsOsm {
      public:
         VerbsOsm();
         virtual ~VerbsOsm();
   
         bool Init();
         bool Close();
   
         void SetResult(osmv_query_res_t* res) { fLastResult = *res; }
   
         bool ManageMultiCastGroup(bool isadd,
                                   uint8_t* mgid,
                                   uint16_t* mlid);
   
         bool QueryMyltucastGroup(uint8_t* mgid, uint16_t& mlid);
   
         bool PrintAllMulticasts();
   
      protected:
   
         bool BindPort();
   
         bool Query_SA(osmv_query_type_t query_type,
                       uint64_t          comp_mask,
                       ib_member_rec_t   *mc_req,
                       ib_sa_mad_t  *res);
   
   
         osm_vendor_t*       f_vendor;
         osm_log_t*          f_log;
         ib_port_attr_t      f_local_port;
         osm_bind_handle_t   f_bind_handle;
         osm_mad_pool_t      f_mad_pool;
   
         osmv_query_res_t    fLastResult; // used in call back function to store result
   };
}

#endif
