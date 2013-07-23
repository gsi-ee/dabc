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

#ifndef DABC_Buffer
#include "dabc/Buffer.h"
#endif

#include <stdint.h>

namespace dabc {

   extern const char* prop_kind;
   extern const char* prop_realname;      // real name property specified, when xml node can not have such name
   extern const char* prop_masteritem;    // relative name of master item, which should be loaded before item itself can be used
   extern const char* prop_binary_producer;
   extern const char* prop_content_hash;  // content hash, which should describe if object is changed

   class Hierarchy;

   /** This is header, which should be supplied for every binary data,
    * created in Hierarchy classes
    *
    */

#pragma pack(push, 4)

   struct BinDataHeader {
      uint32_t typ;              ///< type of the binary header (1 for the moment)
      uint32_t version;          ///< version of data in binary data
      uint32_t master_version;   ///< version of master object for that binary data
      uint32_t zipped;           ///< length of unzipped data
      uint32_t payload;          ///< length of payload (not including header)

      void reset() {
         typ = 1;
         version = 0;
         master_version = 0;
         zipped = 0;
         payload = 0;
      }
   };

#pragma pack(pop)


   // ===================================================================================

   class HierarchyContainer : public RecordContainer {

      friend class Hierarchy;

      protected:

         enum {
            maskNodeChanged = 1,
            maskChildsChanged = 2,
            maskDefaultValue = 3
         };

         uint64_t   fNodeVersion;       ///< version number of node itself
         uint64_t   fHierarchyVersion;  ///< version number of hierarchy below

         bool       fNodeChanged;       ///< indicate if something was changed in the node during update
         bool       fHierarchyChanged;  ///< indicate if something was changed in the hierarchy

         Buffer     fBinData;           ///< binary data, assigned with element

         HierarchyContainer* TopParent();

         virtual bool SetField(const std::string& name, const char* value, const char* kind);

         bool SaveToJSON(std::string& sbuf, bool isfirstchild = true, int level = 0);

         std::string HtmlBrowserText();

         std::string ItemName();

         /** \brief Create child with specified name
          * \details If indx value specified, child will be created or kept at this position
          * All intermediate childs will be removed
          * \returns pointer on the child */
         HierarchyContainer* CreateChildAt(const std::string& name, int indx);

         /** \brief Update hierarchy from provided container
          * \details Some field or childs could be extracted and change there ownership
          * \returns true when something was changed */
         bool UpdateHierarchyFrom(HierarchyContainer* cont);

         /** Switch on node or hierarchy modified flags */
         void SetModified(bool node, bool hierarchy, bool recursive = false);

      public:
         HierarchyContainer(const std::string& name);

         virtual const char* ClassName() const { return "Hierarchy"; }

         XMLNodePointer_t SaveHierarchyInXmlNode(XMLNodePointer_t parent, uint64_t version = 0, bool withversion = false);

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

         /** \brief Method used to create copy of hierarchy again */
         virtual void BuildHierarchy(HierarchyContainer* cont);

         Buffer& bindata() { return fBinData; }
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

      void Create(const std::string& name);

      /** \brief Create child item in hierarchy with specified name
       * \details If par indx specified, child will be created at specified position */
      Hierarchy CreateChild(const std::string& name, int indx = -1);

      /** \brief Find master item
       * It is used in ROOT to specify position of streamer info */
      Hierarchy FindMaster();

      /** Search for parent element, where binary_producer property is specified
       * Returns name of binary producer and item name, which should be requested (relative to producer itself)  */
      std::string FindBinaryProducer(std::string& request_name);

      /** \brief Build full hierarchy of the objects structure, provided in reference */
      void Build(const std::string& topname, Reference top);

      /** \brief Reconstruct complete hierarchy, setting node/structure modifications fields correctly
       * \details source hierarchy can be modified to reuse fields inside */
      bool Update(Hierarchy& src);

      /** \brief Update from objects structure */
      bool UpdateHierarchy(Reference top);

      std::string SaveToXml(bool compact = false, uint64_t version = 0);

      uint64_t GetVersion() const { return GetObject() ? GetObject()->GetVersion() : 0; }

      void SetNodeVersion(uint64_t v) { if (GetObject()) GetObject()->fNodeVersion = v; }

      bool UpdateFromXml(const std::string& xml);

      std::string SaveToJSON(bool compact = false, bool excludetop = false);
   };


}

#endif
