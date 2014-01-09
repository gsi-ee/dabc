// $Id$
// Author: Sergey Linev   28/12/2013

#ifndef ROOT_TFastCgi
#define ROOT_TFastCgi

#ifndef ROOT_THttpEngine
#include "THttpEngine.h"
#endif

class TThread;

class TFastCgi : public THttpEngine {
   protected:
      Int_t  fSocket;   //! socket used by fastcgi
      TThread *fThrd;   //! thread which takes requests, can be many later
   public:
      TFastCgi();
      virtual ~TFastCgi();

      Int_t GetSocket() const { return fSocket; }

      virtual Bool_t Create(const char* args);

   ClassDef(TFastCgi, 0) // fastcgi engine for THttpServer
};


#endif
