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

#ifndef DABC_CommandDef
#define DABC_CommandDef

#ifndef DABC_Hierarchy
#include "dabc/Hierarchy.h"
#endif

#ifndef DABC_Command
#include "dabc/Command.h"
#endif

namespace dabc {

   class CommandDef : public Hierarchy {
      protected:
         template<class T>
         bool verify_object(Object* src, T* &tgt)
         {
            tgt = dynamic_cast<T*>(src);
            if (tgt==0) return false;

            if (!tgt->Fields().HasField(prop_kind)) return false;

            return tgt->Fields().Field(prop_kind).AsStr() == "DABC.Command";
         }

      public:

      DABC_REFERENCE(CommandDef, Hierarchy, HierarchyContainer)

      /** \brief Add argument */

      void AddArg(const std::string& name, const std::string& typ_name = "int");

      /** \brief Creates command according to command definition */
      Command CreateCommand(const std::string& query = "");
   };

}

#endif
