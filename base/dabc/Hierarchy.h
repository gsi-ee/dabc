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

#ifndef DABC_Queue
#include "dabc/Queue.h"
#endif

#include <stdint.h>

namespace dabc {

   extern const char* prop_kind;
   extern const char* prop_realname;      // real name property specified, when xml node can not have such name
   extern const char* prop_masteritem;    // relative name of master item, which should be loaded before item itself can be used
   extern const char* prop_binary_producer; // identifies item, which can deliver binary data for all its
   extern const char* prop_content_hash;  // content hash, which should describe if object is changed
   extern const char* prop_history;        // indicates that history for that element is kept
   extern const char* prop_time;           // time property, supplied when history is created

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


   struct HistoryItem {
      public:
         uint64_t version;
         std::string content;
         HistoryItem() : version(0), content() {}
         void reset() { version = 0; content.clear(); }
   };


   class HistoryContainer : public Object {
      public:

         bool        fEnabled;               ///< indicates if history recordnig is enabled
         RecordsQueue<HistoryItem> fArr;     ///< container with item history
         bool        fRecordTime;             ///< if true, time will be recorded when value is modified
         std::string fAutoRecord;            ///< field name, which change trigger automatic record of the history (when enabled)
         bool        fForceAutoRecord;       ///< if true, even same value will be recorded in history
         uint64_t    fRemoteReqVersion;      ///< last version, which was taken from remote
         uint64_t    fLocalReqVersion;       ///< local version, when request was done


         HistoryContainer() :
            Object(0,"cont", flAutoDestroy | flIsOwner),
            fEnabled(false),
            fArr(),
            fRecordTime(false),
            fAutoRecord(),
            fForceAutoRecord(false),
            fRemoteReqVersion(0),
            fLocalReqVersion(0)
            {}
         virtual ~HistoryContainer() {}


   };


   class History : public Reference {
      DABC_REFERENCE(History, Reference, HistoryContainer)

      void Allocate(unsigned sz = 0) {
         SetObject(new HistoryContainer);
         if (sz>0) GetObject()->fArr.Allocate(sz);
      }

      bool DoHistory() const { return null() ? false : GetObject()->fEnabled && (GetObject()->fArr.Capacity()>0); }

      unsigned Capacity() const { return null() ? 0 : GetObject()->fArr.Capacity(); }
   };

   // =================================================================


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

         /** when true, element cannot be removed during update
          * only first childs with such flag could remain */
         bool       fPermanent;         ///<

         bool       fNodeChanged;       ///< indicate if something was changed in the node during update
         bool       fHierarchyChanged;  ///< indicate if something was changed in the hierarchy

         Buffer     fBinData;           ///< binary data, assigned with element

         History    fHist;              ///< special object with history data


         HierarchyContainer* TopParent();

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

         /** \brief Switch on node or hierarchy modified flags */
         void SetModified(bool node, bool hierarchy, bool recursive = false);

         /** \brief method called every time when field should be changed
          * Used to detect important changes and to mark new version */
         virtual bool SetField(const std::string& name, const char* value, const char* kind);

         /** \brief produces simple diff, where only value is changing (and optionally time) */
         std::string MakeSimpleDiff(const char* oldvalue);

         /** \brief Add new entry to history */
         void AddHistory(const std::string& diff);

         /** \brief Return current time in format, which goes to history */
         std::string GetTimeStr();

         /** \brief Marks item and all its parents with changed version */
         void MarkWithChangedVersion();

      public:
         HierarchyContainer(const std::string& name);
         virtual ~HierarchyContainer();

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

         /** \brief Produces string with xml code */
         std::string RequestHistory(uint64_t version = 0, int limit = 0);

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

      /** \brief Activate history production for selected element */
      void EnableHistory(int length = 100, const std::string& autorec = "", bool force = false);

      /** \brief Set permanent flag for hierarchy item */
      void SetPermanent(bool on=true) { if (!null()) GetObject()->fPermanent = on; }

      /** \brief Returns true if item records history local, no need to request any other sources */
      bool HasLocalHistory() const;

      /** \brief Returns true if remote history is recorded and it is up-to-date */
      bool HasActualRemoteHistory() const;

      std::string SaveToXml(bool compact = false, uint64_t version = 0);

      uint64_t GetVersion() const { return GetObject() ? GetObject()->GetVersion() : 0; }

      void SetNodeVersion(uint64_t v) { if (GetObject()) GetObject()->fNodeVersion = v; }

      bool UpdateFromXml(const std::string& xml);

      std::string SaveToJSON(bool compact = false, bool excludetop = false);


      /** \brief If possible, returns buffer with binary data, which can be send as reply */
      Buffer GetBinaryData(uint64_t query_version = 0);

      /** \brief Create bin request, which should be submitted to get bindata */
      Command ProduceBinaryRequest();

      /** \brief Analyzes result of request and returns buffer which can be send to remote */
      Buffer ApplyBinaryRequest(Command cmd);

      /** \brief Return child element from hierarchy */
      Hierarchy FindChild(const char* name) { return Record::FindChild(name); }

      Command ProduceHistoryRequest();

      Buffer ExecuteHistoryRequest(Command cmd);

      bool ApplyHierarchyRequest(Command cmd);
   };


}

#endif
