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

         XMLNodePointer_t fCurrItem; // currently found item
         XMLNodePointer_t fCurrChld; // selected child in current item
         XMLNodePointer_t fLastTop; // currently found item
         bool             fCurrStrict; // must have strict syntax match

         std::string  fMgrName;
         int          fMgrNodeId;
         int          fMgrNumNodes;
         std::string  fMgrDimNode;
         int          fMgrDimPort;

         bool XDAQ_LoadLibs();
         bool XDAQ_ReadPars();

         Basic* GetObjParent(Basic* obj, int lvl);

      public:
         Configuration(const char* fname = 0);
         virtual ~Configuration();

         bool SelectContext(unsigned cfgid, unsigned nodeid, unsigned numnodes, const char* logfile = 0, const char* dimnode = 0);

         const char* MgrName() const { return fMgrName.c_str(); }
         int MgrNodeId() const { return fMgrNodeId; }
         int MgrNumNodes() const { return fMgrNumNodes; }
         const char* MgrDimNode() const { return fMgrDimNode.c_str(); }
         int MgrDimPort() const { return fMgrDimPort; }

         std::string StartFuncName();

         bool LoadLibs(const char* startfunc = 0);

         bool ReadPars();

         virtual bool CreateItem(const char* name, const char* value = 0);
         virtual bool CreateAttr(const char* name, const char* value);
         virtual bool PopItem();
         virtual bool PushLastItem();

         bool StoreObject(const char* fname, Basic* obj);

         virtual bool FindItem(const char* name);
         virtual const char* GetItemValue();
         virtual const char* GetAttrValue(const char* name);
         virtual bool CheckAttr(const char* name, const char* value);

         virtual const char* Find(Basic* obj, const char* findattr = 0);
   };


}

#endif
