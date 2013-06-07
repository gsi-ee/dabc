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

#ifndef DABC_Hierarchy
#define DABC_Hierarchy

#ifndef DABC_Record
#include "dabc/Record.h"
#endif

#include <stdint.h>

namespace dabc {

   class HierarchyContainer : public RecordContainer {
      protected:

         enum {
            maskNodeChanged = 1,
            maskChildsChanged = 2,
            maskDefaultValue = 3
         };

         uint64_t   fNodeVersion;       ///< version number of node itself
         uint64_t   fHierarchyVersion;  ///< version number of hierarchy below

         bool       fNodeChanged;       ///< indicate if something was changed in the node
         bool       fHierarchyChanged;  ///< indicate if something was changed in the hierarchy

         HierarchyContainer* TopParent();

         virtual bool SetField(const std::string& name, const char* value, const char* kind);

      public:
         HierarchyContainer(const std::string& name);

         virtual const char* ClassName() const { return "Hierarchy"; }

         XMLNodePointer_t SaveHierarchyInXmlNode(XMLNodePointer_t parent, uint64_t version = 0);

         /** \brief Update hierarchy structure according to object
          * \returns true when something was changed */
         bool UpdateHierarchyFromObject(Object* obj);

         bool UpdateHierarchyFromXmlNode(XMLNodePointer_t objnode);

         void SetVersion(uint64_t version, bool recursive = false, bool force = false);

         bool WasHierarchyModifiedAfter(uint64_t version) const
            { return fHierarchyVersion >= version; }

         bool WasNodeModifiedAfter(uint64_t version) const
            { return fNodeVersion >= version; }

         unsigned ModifiedMask(uint64_t version) const
         {
            return (WasNodeModifiedAfter(version) ? maskNodeChanged : 0) |
                   (WasHierarchyModifiedAfter(version) ? maskChildsChanged : 0);
         }

         uint64_t GetVersion() const { return fNodeVersion > fHierarchyVersion ? fNodeVersion : fHierarchyVersion; }

   };

   // ______________________________________________________________

   /** \brief Represents objects hierarchy of remote (or local) DABC process
    *
    * Idea to completely replicate folder structure with many attributes,
    * which are stored into HierarchyContainer class.
    * One should be able to convert such hierarchy to/from xml file.
    * Idea to create such hierarchy is to able provide different clients to monitor and control DABC process.
    */

   class Hierarchy : public Record {

      DABC_REFERENCE(Hierarchy, Record, HierarchyContainer)

      void MakeHierarchy(Reference top);

      bool UpdateHierarchy(Reference top);

      std::string SaveToXml(bool compact = false, uint64_t version = 0);

      uint64_t GetVersion() const { return GetObject() ? GetObject()->GetVersion() : 0; }

      bool UpdateFromXml(const std::string& xml);
   };


}

#endif
