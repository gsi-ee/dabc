#ifndef DABC_ConfigIO
#define DABC_ConfigIO

namespace dabc {

   class ConfigIO {
      protected:
          bool fExact;
          int  fSearchLevel;

       public:
          ConfigIO() : fExact(false), fSearchLevel(-1) {}
          virtual ~ConfigIO() {}

          virtual bool CreateItem(const char* name, const char* value = 0) = 0;
          virtual bool CreateAttr(const char* name, const char* value) = 0;
          virtual bool PopItem() = 0;
          virtual bool PushLastItem() = 0;
          virtual bool CreateIntAttr(const char* name, int value);

          enum FindKinds { selectTop, findChild, findNext, firstTop };

          virtual void SetExact(bool on = true) { fExact = on; fSearchLevel = -1; }
          bool IsExact() const { return fExact; }
          int SearchLevel() const { return fSearchLevel; }

          virtual bool FindItem(const char* name, FindKinds kind) = 0;
          virtual const char* GetItemValue() = 0;
          virtual const char* GetAttrValue(const char* name) = 0;
          virtual bool CheckAttr(const char* name, const char* value);
          virtual bool CheckIntAttr(const char* name, int value);
   };

}

#endif
