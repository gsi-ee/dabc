#ifndef DABC_Folder
#define DABC_Folder

#ifndef DABC_Basic
#include "dabc/Basic.h"
#endif

#ifndef DABC_collections
#include "dabc/collections.h"
#endif

namespace dabc {

   class Folder : public Basic {
      public:
         Folder(Basic* parent, const char* name, bool owner = false);
         virtual ~Folder();

         virtual void AddChild(Basic* child);
         virtual void RemoveChild(Basic* child);
         void DeleteChilds(int appid = -1);
         void RemoveChilds();

         unsigned NumChilds() const;
         Basic* GetChild(unsigned n) const;
         Basic* FindChild(const char* name) const;
         bool IsChild(Basic* obj) const;
         bool IsOwner() const { return fOwner; }

      protected:
         Folder* GetFolder(const char* name,
                           bool force = false,
                           bool isowner = true);

         virtual void _SetParent(Mutex* mtx, Basic* parent);

         Basic* _FindChild(const char* name) const;

      private:
         PointersVector    fChilds;
         bool              fOwner;
   };
}

#endif
