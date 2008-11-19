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
                          bool required);
                          
         const char* GetXml();
         
         void Register(bool on = true);
      protected:
         std::string fXml;
         bool fClosed;
   };
    
}

#endif
