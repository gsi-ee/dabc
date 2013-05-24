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

#ifndef VERBS_OpenSM
#define VERBS_OpenSM

#define OSM_GUID_ARRAY_SIZE 10

#define OSM_VENDOR_INTF_OPENIB

#include <infiniband/vendor/osm_vendor_api.h>
#include <infiniband/vendor/osm_vendor_sa_api.h>

namespace verbs {

   /** \brief Interface class to opensm */

   class OpenSM {
      public:
         OpenSM();
         virtual ~OpenSM();

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
