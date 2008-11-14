#ifndef DABC_Configuration
#define DABC_Configuration

#ifndef DABC_ConfigBase
#include "dabc/ConfigBase.h"
#endif

#ifndef DABC_ConfigIO
#include "dabc/ConfigIO.h"
#endif

#include <list>


namespace dabc {

   class Basic;

   class Configuration : public ConfigBase,
                         public ConfigIO {
      protected:
         XMLNodePointer_t  fSelected; // selected context node

         std::list<XMLNodePointer_t> fStoreStack; // stack of nodes during store
         XMLNodePointer_t fStoreLastPop; // last pop-ed item

         XMLNodePointer_t fCurrMasterItem; // current top (master) item in search
         XMLNodePointer_t fCurrItem; // currently found item

         bool XDAQ_LoadLibs();
         bool XDAQ_ReadPars();

      public:
         Configuration(const char* fname = 0);
         virtual ~Configuration();

         bool SelectContext(unsigned cfgid, unsigned nodeid, unsigned numnodes, const char* logfile = 0);

         bool LoadLibs();

         bool ReadPars();

         virtual bool CreateItem(const char* name, const char* value = 0);
         virtual bool CreateAttr(const char* name, const char* value);
         virtual bool PopItem();
         virtual bool PushLastItem();

         virtual bool FindItem(const char* name, FindKinds kind);
         virtual const char* GetItemValue();
         virtual const char* GetAttrValue(const char* name);

         bool StoreObject(const char* fname, Basic* obj);

   };


}

#endif
