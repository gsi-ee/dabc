// $Id$
// Author: Sergey Linev   21/12/2013

#ifndef ROOT_TCivetweb
#define ROOT_TCivetweb

#ifndef ROOT_THttpEngine
#include "THttpEngine.h"
#endif

class TCivetweb : public THttpEngine {
protected:
   void *fCtx;             //! civetweb context
   void *fCallbacks;       //! call-back table for civetweb webserver
   TString fTopName;       //! name of top item
public:
   TCivetweb();
   virtual ~TCivetweb();

   virtual Bool_t Create(const char *args);

   const char *GetTopName() const
   {
      return fTopName.Data();
   }

   ClassDef(TCivetweb, 0) // http server implementation, based on civetweb embedded server
};


#endif
