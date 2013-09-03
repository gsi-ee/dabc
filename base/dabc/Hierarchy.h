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
   extern const char* prop_version;       // version number of hierarchy item
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
         uint64_t version;          ///< version number
         RecordFieldsMap* fields;   ///< all fields, which are preserved
         HistoryItem() : version(0), fields(0) {}
         ~HistoryItem() { reset(); }
         void reset() { version = 0; delete fields; fields = 0; }
         RecordFieldsMap* take() { RecordFieldsMap* res = fields; fields = 0; return res; }
   };


   class HistoryContainer : public Object {
      public:

         bool        fEnabled;               ///< indicates if history recording is enabled
         bool        fChildsEnabled;          ///< true if history recording also was enabled for childs
         RecordFieldsMap* fPrev;             ///< map with previous set of attributes
         RecordsQueue<HistoryItem> fArr;     ///< container with history items
         uint64_t    fRemoteReqVersion;      ///< last version, which was taken from remote
         uint64_t    fLocalReqVersion;       ///< local version, when request was done
         bool        fCrossBoundary;         ///< flag set when recover from binary, indicates that history is complete for specified version

         HistoryContainer() :
            Object(0,"cont", flAutoDestroy | flIsOwner),
            fEnabled(false),
            fChildsEnabled(false),
            fPrev(0),
            fArr(),
            fRemoteReqVersion(0),
            fLocalReqVersion(0),
            fCrossBoundary(false)
            {}

         virtual ~HistoryContainer()
         {
            delete fPrev; fPrev = 0;
         }

         uint64_t StoreSize(uint64_t version, int hist_limit = -1);

         bool Stream(iostream& s, uint64_t version, int hist_limit = -1);

         RecordFieldsMap* TakeNext()
         {
            if (fArr.Size()==0) return 0;
            RecordFieldsMap* next = fArr.Front().take();
            fArr.PopOnly();
            return next;
         }
   };


   class History : public Reference {
      DABC_REFERENCE(History, Reference, HistoryContainer)

      void Allocate(unsigned sz = 0)
      {
         SetObject(new HistoryContainer);
         if (sz>0) GetObject()->fArr.Allocate(sz);
      }

      bool DoHistory() const { return null() ? false : GetObject()->fEnabled && (GetObject()->fArr.Capacity()>0); }

      unsigned Capacity() const { return null() ? 0 : GetObject()->fArr.Capacity(); }

      unsigned Size() const { return null() ? 0 : GetObject()->fArr.Size(); }
   };

   // =================================================================


   class HierarchyContainer : public RecordContainerNew {

      friend class Hierarchy;

      protected:

         enum {
            maskNodeChanged = 1,
            maskChildsChanged = 2,
            maskDiffStored = 4,
            maskHistory = 8
         };

         uint64_t   fNodeVersion;       ///< version number of node itself
         uint64_t   fHierarchyVersion;  ///< version number of hierarchy below

         bool       fAutoTime;          ///< when enabled, by node change (not hierarchy) time attribute will be set

         bool       fNodeChanged;       ///< indicate if something was changed in the node during update
         bool       fHierarchyChanged;  ///< indicate if something was changed in the hierarchy
         bool       fDiffNode;          ///< if true, indicates that it is diff version

         Buffer     fBinData;           ///< binary data, assigned with element

         History    fHist;              ///< special object with history data

         HierarchyContainer* TopParent();

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

         /** \brief Add new entry to history */
         void AddHistory(RecordFieldsMap* diff);

         /** \brief Clear all entries in history, but not history object itself */
         void ClearHistoryEntries();

         /** \brief Returns true if any node field was changed or removed/inserted
          * If specified, all childs will be checked */
         bool IsNodeChanged(bool withchilds = true);

         /** Return true if history activated for the node
          * If necessary, history object will be initialized */
         bool CheckIfDoingHistory();

         /** \brief If item changed, marked with version, time stamp applied, history recording  */
         void MarkVersionIfChanged(uint64_t ver, double& tm, bool withchilds, bool force = false);

         /** \brief Central method, which analyzes all possible changes in node (and its childs)
          * If any changes found, node marked with new version
          * If enabled, history item will be created.
          * If enabled, time stamp will be provided for changed items */
         void MarkChangedItems(bool withchilds = true, double tm = 0.);

         /** \brief Enable time recording for hierarchy element every time when item is changed */
         void EnableTimeRecording(bool withchilds = true);

      public:
         HierarchyContainer(const std::string& name);
         virtual ~HierarchyContainer();

         virtual const char* ClassName() const { return "Hierarchy"; }

         uint64_t StoreSize(uint64_t v = 0, int hist_limit = -1);

         bool Stream(iostream& s, uint64_t v = 0, int hist_limit = -1);

         XMLNodePointer_t SaveHierarchyInXmlNode(XMLNodePointer_t parent);

         uint64_t GetVersion() const { return fNodeVersion > fHierarchyVersion ? fNodeVersion : fHierarchyVersion; }

         /** \brief Produces string with xml code, containing history */
         std::string RequestHistoryAsXml(uint64_t version = 0, int limit = 0);

         /** \brief Produces history in binary form */
         dabc::Buffer RequestHistory(uint64_t version = 0, int limit = 0);

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

   class Hierarchy : public RecordNew {

      DABC_REFERENCE(Hierarchy, RecordNew, HierarchyContainer)

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

      bool HasField(const std::string& name) const { return GetObject() ? GetObject()->Fields().HasField(name) : false; }
      bool RemoveField(const std::string& name) { return GetObject() ? GetObject()->Fields().RemoveField(name) : false; }

      unsigned NumFields() const { return GetObject() ? GetObject()->Fields().NumFields() : 0; }
      std::string FieldName(unsigned cnt) const { return GetObject() ? GetObject()->Fields().FieldName(cnt) : std::string(); }

      RecordFieldNew& Field(const std::string& name) { return GetObject()->Fields().Field(name); }
      const RecordFieldNew& Field(const std::string& name) const { return GetObject()->Fields().Field(name); }

      /** \brief Build full hierarchy of the objects structure, provided in reference */
      void Build(const std::string& topname, Reference top);

      /** \brief Reconstruct complete hierarchy, setting node/structure modifications fields correctly
       * \details source hierarchy can be modified to reuse fields inside */
      bool Update(Hierarchy& src);

      /** \brief Update from objects structure */
      bool UpdateHierarchy(Reference top);

      /** \brief Activate history production for selected element and its childs */
      void EnableHistory(unsigned length = 100, bool withchilds = false);

      /** \brief Enable time recording for hierarchy element every time when item is changed */
      void EnableTimeRecording(bool withchilds = true)
      { if (GetObject()) GetObject()->EnableTimeRecording(withchilds); }

      /** \brief If any field was modified, item will be marked with new version */
      void MarkChangedItems(bool withchilds = true, double tm = 0.)
      { if (GetObject()) GetObject()->MarkChangedItems(withchilds, tm); }

      /** \brief Returns true if item records history local, no need to request any other sources */
      bool HasLocalHistory() const;

      /** \brief Returns true if remote history is recorded and it is up-to-date */
      bool HasActualRemoteHistory() const;

      /** Save hierarchy in binary form, relative to specified version */
      dabc::Buffer SaveToBuffer(uint64_t version = 0);

      /** Read hierarchy from buffer */
      bool ReadFromBuffer(const dabc::Buffer& buf);

      /** Apply modification to hierarchy, using stored binary data  */
      bool UpdateFromBuffer(const dabc::Buffer& buf);

      /** \brief Store hierarchy in form of xml */
      std::string SaveToXml(bool compact = false);

      uint64_t GetVersion() const { return GetObject() ? GetObject()->GetVersion() : 0; }


      /** \brief If possible, returns buffer with binary data, which can be send as reply */
      Buffer GetBinaryData(uint64_t query_version = 0);

      /** \brief Create bin request, which should be submitted to get bindata */
      Command ProduceBinaryRequest();

      /** \brief Analyzes result of request and returns buffer which can be send to remote */
      Buffer ApplyBinaryRequest(Command cmd);

      /** \brief Return child element from hierarchy */
      Hierarchy FindChild(const char* name) { return RecordNew::FindChild(name); }

      Command ProduceHistoryRequest();
      Buffer ExecuteHistoryRequest(Command cmd);
      bool ApplyHistoryRequest(Command cmd);

      Buffer GetLocalImage(uint64_t version = 0);
      Command ProduceImageRequest();
      bool ApplyImageRequest(Command cmd);
   };


}

#endif
