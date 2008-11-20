#include "dabc/CommandDefinition.h"

#include "dabc/logging.h"
#include "dabc/Manager.h"
#include "dabc/Module.h"

dabc::CommandDefinition::CommandDefinition(Basic* parent, const char* name) :
   Basic(parent, name),
   fXml(),
   fClosed(false)
{
   fXml = format("<command name=\"%s\" scope=\"public\">\n", name);
}

dabc::CommandDefinition::~CommandDefinition()
{
   Register(false);
}

void dabc::CommandDefinition::Register(bool on)
{
   Module* m = GetParent() ? dynamic_cast<Module*>(GetParent()->GetParent()) : 0;

   DOUT3((" CommandDefinition::Register(%s) mgr = %p mdl %p cmd %s", DBOOL(on), dabc::mgr(), m, GetName()));

   if (dabc::mgr())
      dabc::mgr()->CommandRegistration(m, this, on);
}


void dabc::CommandDefinition::AddArgument(const char* name,
                                          CommandArgumentType typ,
                                          bool required)
{
   if (fClosed) {
      EOUT(("Command definition %s is closed", GetName()));
      return;
   }

   const char* styp = 0;
   switch (typ) {
      case argInt: styp = "I"; break;
      case argDouble: styp = "F"; break;
      case argString: styp = "C"; break;
   }

   fXml += format("  <argument name=\"%s\" type=\"%s\" value=\"\" required=\"%s\"/>\n",
                      name, styp, (required ? xmlTrueValue : xmlFalseValue));
}

const char* dabc::CommandDefinition::GetXml()
{
   if (!fClosed) {
      fXml+="</command>\n";
      fClosed = true;
   }

   return fXml.c_str();
}
