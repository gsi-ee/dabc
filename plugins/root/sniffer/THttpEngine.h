// $Id$
// Author: Sergey Linev   21/12/2013

#ifndef ROOT_THttpEngine
#define ROOT_THttpEngine

#ifndef ROOT_TNamed
#include "TNamed.h"
#endif

class THttpServer;

class THttpEngine : public TNamed {
protected:
   friend class THttpServer;

   THttpServer* fServer;    //! object server

   THttpEngine(const char* name, const char* title, THttpServer* serv);

   /** Method regularly called in main ROOT context */
   virtual void Process() {}

public:
   virtual ~THttpEngine();

   /** Indicates that engine is ready to process data. If not, instance will be deleted */
   virtual Bool_t IsReady() { return kFALSE; }

   THttpServer* GetServer() const { return fServer; }

   ClassDef(THttpEngine, 0) // abstract class which should provide http-based protocol for server
};

#endif
