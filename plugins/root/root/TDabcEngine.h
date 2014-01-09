// $Id$

/************************************************************
 * The Data Acquisition Backbone Core (DABC)                *
 ************************************************************
 * Copyright (C) 2009 -                                     *
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH      *
 * Planckstr. 1, 64291 Darmstadt, Germany                   *
 * Contact:  http://dabc.gsi.de                             *
 ************************************************************
 * This software can be used under the GPL license          *
 * agreements as stated in LICENSE.txt file                 *
 * which is part of the distribution.                       *
 ************************************************************/

#ifndef ROOT_TDabcEngine
#define ROOT_TDabcEngine

#ifndef ROOT_THttpEngine
#include "THttpEngine.h"
#endif

class TDabcEngine : public THttpEngine {
   protected:
      virtual void Process();
   public:
      TDabcEngine();
      virtual ~TDabcEngine();

      virtual Bool_t Create(const char* args);

   ClassDef(TDabcEngine, 0) // dabc engine for THttpServer
};


#endif
