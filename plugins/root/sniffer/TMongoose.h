// $Id$
// Author: Sergey Linev   21/12/2013

#ifndef ROOT_TMongoose
#define ROOT_TMongoose

#ifndef ROOT_THttpEngine
#include "THttpEngine.h"
#endif

class TMongoose : public THttpEngine {
   protected:
      void* fCtx;             //! mongoose context
      void* fCallbacks;       //! call-back table for mongoose webserver
      TString fTopName;       //! name of top item
   public:
      TMongoose();
      virtual ~TMongoose();

      virtual Bool_t Create(const char* args);

      const char* GetTopName() const { return fTopName.Data(); }

   ClassDef(TMongoose, 0) // http server implementation, based on mongoose embedded server
};


#endif
