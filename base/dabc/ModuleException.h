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

#ifndef DABC_ModuleException
#define DABC_ModuleException

#ifndef DABC_Exception
#include "dabc/Exception.h"
#endif

namespace dabc {
   
   class Module;

   class ModuleException : public Exception {
      public: 
         ModuleException(Module* m, const char* info = "") throw ();
         Module* GetModule() const throw() { return fModule; }
      
      protected:
         Module* fModule;
   };
}


#endif
