#ifndef DABC_ConfigIO
#define DABC_ConfigIO

namespace dabc {

   class Basic;

   class ConfigIO {
       public:
          ConfigIO() {}
          virtual ~ConfigIO() {}

          virtual bool CreateItem(const char* name, const char* value = 0) = 0;
          virtual bool CreateAttr(const char* name, const char* value) = 0;
          virtual bool PopItem() = 0;
          virtual bool PushLastItem() = 0;
          virtual bool CreateIntAttr(const char* name, int value);

          virtual bool FindItem(const char* name) = 0;
          virtual bool CheckAttr(const char* name, const char* value) = 0;

          virtual const char* Find(Basic* obj, const char* findattr = 0) = 0;
          virtual const char* GetAttrValue(const char* name) = 0;
   };

}

#endif
