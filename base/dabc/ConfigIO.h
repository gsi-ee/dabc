#ifndef DABC_ConfigIO
#define DABC_ConfigIO

namespace dabc {

   class ConfigIO {
       public:
          virtual bool CreateItem(const char* name, const char* value = 0) = 0;
          virtual bool CreateAttr(const char* name, const char* value) = 0;
          virtual bool PopItem() = 0;
          virtual bool PushLastItem() = 0;
          virtual bool CreateIntAttr(const char* name, int value);

          enum FindKinds { selectTop, findTop, findChild };

          virtual bool FindItem(const char* name, FindKinds kind) = 0;
          virtual const char* GetItemValue() = 0;
          virtual const char* GetAttrValue(const char* name) = 0;
          virtual bool CheckAttr(const char* name, const char* value);
          virtual bool CheckIntAttr(const char* name, int value);
   };

}

#endif
