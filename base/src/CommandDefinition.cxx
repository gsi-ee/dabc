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
   if (dabc::mgr())
      dabc::mgr()->CommandRegistration(this, on);
}

void dabc::CommandDefinition::AddArgument(const char* name,
                                          CommandArgumentType typ,
                                          bool required,
                                          const char* dfltvalue)
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

   if (dfltvalue==0) dfltvalue = "";

   fXml += format("  <argument name=\"%s\" type=\"%s\" value=\"%s\" required=\"%s\"/>\n",
                      name, styp, dfltvalue, (required ? xmlTrueValue : xmlFalseValue));
}

const char* dabc::CommandDefinition::GetXml()
{
   if (!fClosed) {
      fXml+="</command>\n";
      fClosed = true;
   }

   return fXml.c_str();
}
