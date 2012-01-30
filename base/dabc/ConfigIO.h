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

#ifndef DABC_ConfigIO
#define DABC_ConfigIO

#ifndef DABC_string
#include "dabc/string.h"
#endif

#ifndef DABC_XmlEngine
#include "dabc/XmlEngine.h"
#endif

#include <list>

namespace dabc {

   class Object;
   class Configuration;
   class RecordContainer;

   // TODO: later ConfigIO should be non-transient reference on configuration class
   class ConfigIO {
      protected:
         Configuration* fCfg;

         XMLNodePointer_t fCurrItem; // currently found item
         XMLNodePointer_t fCurrChld; // selected child in current item
         bool             fCurrStrict; // must have strict syntax match

         XMLNodePointer_t FindSubItem(XMLNodePointer_t node, const char* name);

         static Object* GetObjParent(Object* obj, int lvl);

      public:
         ConfigIO(Configuration* cfg);

         ConfigIO(const ConfigIO& src);
         virtual ~ConfigIO() {}

         bool FindItem(const char* name);

         /** Check if item, found by FindItem routine, has attribute with specified value
          * If \param optional specified, attribute value will be checked only when attribute existing */
         bool CheckAttr(const char* name, const char* value);

         bool ReadRecord(Object* obj, const std::string& name, RecordContainer* cont);
   };

}

#endif
