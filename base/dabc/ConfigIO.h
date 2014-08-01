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

   class RecordField;
   class RecordFieldsMap;

   /** \brief Interface class between xml configuration and dabc objects
    *
    * \ingroup dabc_all_classes
    *
    * Used to find xml nodes, which correspond to dabc objects like module or application
    */

   class ConfigIO {

     // TODO: later ConfigIO should be non-transient reference on configuration class

      protected:
         Configuration* fCfg;           ///< pointer on configuration object

         XMLNodePointer_t fCurrItem;    ///< currently found item
         XMLNodePointer_t fCurrChld;    ///< selected child in current item
         bool             fCurrStrict;  ///< must have strict syntax match

         XMLNodePointer_t FindSubItem(XMLNodePointer_t node, const char* name);

         static Object* GetObjParent(Object* obj, int lvl);

         std::string ResolveEnv(const char* value);

      public:
         ConfigIO(Configuration* cfg);

         ConfigIO(const ConfigIO& src);

         virtual ~ConfigIO() {}

         bool FindItem(const char* name);

         /** \brief Check if item, found by FindItem routine, has attribute with specified value */
         bool CheckAttr(const char* name, const char* value);

         bool ReadRecordField(Object* obj, const std::string& name, RecordField* field, RecordFieldsMap* fieldsmap);
   };

}

#endif
