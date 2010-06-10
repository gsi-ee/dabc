/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009- 
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH 
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/
#ifndef DABC_CommandDefinition
#define DABC_CommandDefinition

#ifndef DABC_Basic
#include "dabc/Basic.h"
#endif

#ifndef DABC_string
#include "dabc/string.h"
#endif

namespace dabc {
    
   enum CommandArgumentType {
      argInt, 
      argDouble,
      argString
   }; 
   
   class CommandDefinition : public Basic {
      public:
         CommandDefinition(Basic* parent, const char* name);
         virtual ~CommandDefinition();
         
         void AddArgument(const char* name, 
                          CommandArgumentType typ,
                          bool required,
                          const char* dfltvalue = 0);
                          
         const char* GetXml();
         
         void Register(bool on = true);
      protected:
         std::string fXml;
         bool fClosed;
   };
    
}

#endif
