// $Id$
// Author: Sergey Linev   22/12/2013

#ifndef ROOT_TRootSnifferStore
#define ROOT_TRootSnifferStore

#ifndef ROOT_TObject
#include "TObject.h"
#endif

#ifndef ROOT_TString
#include "TString.h"
#endif

class TRootSnifferStore : public TObject {
public:
   void* res;             //! pointer on found item
   TClass* rescl;         //! class of found item
   Int_t res_chld;        //! count of found childs, -1 by default

   TRootSnifferStore() : TObject(), res(0), rescl(0), res_chld(-1)  {}
   virtual ~TRootSnifferStore() {}

   virtual void CreateNode(Int_t, const char*) {}
   virtual void SetField(Int_t, const char*, const char*, Int_t) {}
   virtual void BeforeNextChild(Int_t, Int_t, Int_t) {}
   virtual void CloseNode(Int_t, const char*, Bool_t) {}

   ClassDef(TRootSnifferStore, 0)
};

class TRootSnifferStoreXml : public TRootSnifferStore {
public:
   TString* buf;

   TRootSnifferStoreXml(TString& _buf) :
      TRootSnifferStore(),
      buf(&_buf) {}
   virtual ~TRootSnifferStoreXml() {}

   virtual void CreateNode(Int_t lvl, const char* nodename);
   virtual void SetField(Int_t lvl, const char* field, const char* value, Int_t);
   virtual void BeforeNextChild(Int_t lvl, Int_t nchld, Int_t);
   virtual void CloseNode(Int_t lvl, const char* nodename, Bool_t hadchilds);

   ClassDef(TRootSnifferStoreXml, 0)
};


class TRootSnifferStoreJson : public TRootSnifferStore {
public:
   TString* buf;

   TRootSnifferStoreJson(TString& _buf) :
      TRootSnifferStore(),
      buf(&_buf) {}
   virtual ~TRootSnifferStoreJson() {}

   virtual void CreateNode(Int_t lvl, const char* nodename);
   virtual void SetField(Int_t lvl, const char* field, const char* value, Int_t);
   virtual void BeforeNextChild(Int_t lvl, Int_t nchld, Int_t nfld);
   virtual void CloseNode(Int_t lvl, const char* nodename, Bool_t hadchilds);

   ClassDef(TRootSnifferStoreJson, 0)
};

#endif
