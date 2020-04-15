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

#ifndef DABC_timing
#include "dabc/timing.h"
#endif

#include <cstdint>

namespace dabc {

   extern const char* prop_kind;
   extern const char* prop_version;       // version number of hierarchy item
   extern const char* prop_realname;      // real name property specified, when dabc item can not have such name (including HTTP syntax)
   extern const char* prop_masteritem;    // relative name of master item, which should be loaded before item itself can be used
   extern const char* prop_producer;      // identifies item, which can deliver binary data for all its
   extern const char* prop_error;         // indicates any kind of error - typically in text form
   extern const char* prop_hash;          // content hash, which should describe if object is changed
   extern const char* prop_auth;          // when specified, enabled (true) or disabled (false) authentication for the elements
   extern const char* prop_history;       // indicates that history for that element is kept
   extern const char* prop_time;          // time property, supplied when history is created
   extern const char* prop_more;          // indicate that item can provide more hierarchy if requested
   extern const char* prop_view;          // property indicate preferable view for the item


   class Hierarchy;

   struct HistoryItem {
      public:
         uint64_t version{0};                ///< version number
         RecordFieldsMap* fields{nullptr};   ///< all fields, which are preserved
         HistoryItem() = default;
         ~HistoryItem() { reset(); }
         void reset() { version = 0; delete fields; fields = nullptr; }
         RecordFieldsMap* take() { auto res = fields; fields = nullptr; return res; }
   };


   class HistoryContainer : public Object {
      public:

         bool        fEnabled{false};         ///< indicates if history recording is enabled
         bool        fChildsEnabled{false};   ///< true if history recording also was enabled for childs
         RecordFieldsMap* fPrev{nullptr};     ///< map with previous set of attributes
         RecordsQueue<HistoryItem> fArr;      ///< container with history items
         uint64_t    fRemoteReqVersion{0};    ///< last version, which was taken from remote
         uint64_t    fLocalReqVersion{0};     ///< local version, when request was done
         bool        fCrossBoundary{false};   ///< flag set when recover from binary, indicates that history is complete for specified version

         HistoryContainer() :
            Object(0,"cont", flAutoDestroy | flIsOwner),
            fEnabled(false),
            fChildsEnabled(false),
            fPrev(nullptr),
            fArr(),
            fRemoteReqVersion(0),
            fLocalReqVersion(0),
            fCrossBoundary(false)
            {}

         virtual ~HistoryContainer()
         {
            delete fPrev; fPrev = nullptr;
         }

         uint64_t StoreSize(uint64_t version, int hist_limit = -1);

         bool Stream(iostream &s, uint64_t version, int hist_limit = -1);

         RecordFieldsMap* TakeNext()
         {
            if (fArr.Size()==0) return nullptr;
            RecordFieldsMap *next = fArr.Front().take();
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

      bool SaveTo(HStore& res);
   };

   // =======================================================

   class HistoryIterContainer;

   class HistoryIter : public Record {

      DABC_REFERENCE(HistoryIter, Record, RecordContainer)

      bool next();
   };

   // =================================================================


   enum HierarchyStreamKind {
      stream_NamesList,   // store only names list
      stream_Value,       // store only selected entry without childs (plus history, if specified)
      stream_Full,        // store full hierarchy (plus history, if specified)
      stream_NoDelete     // when reading, no any element will be deleted
   };

   class HierarchyContainer : public RecordContainer {

      friend class Hierarchy;
      friend class HistoryIter;
      friend class HistoryIterContainer;

      protected:

         enum {
            maskFieldsStored = 1,
            maskChildsStored = 2,
            maskDiffStored = 4,
            maskHistory = 8
         };

         /** \brief Version number of the node
          * Any changes in the node will cause changes of the version */
         uint64_t   fNodeVersion;       ///< version number of node itself

         /** \brief Version used in DNS requests.
          * Changed when any childs add/removed or dns-relevant fields are changed
          * This version is used by name services to detect and update possible changes in hierarchy
          * and do not care about any other values changes */
         uint64_t   fNamesVersion;

         /** \brief Version of hierarchy structure
          * Childs version is changed when any childs is changed or inserted/removed */
         uint64_t   fChildsVersion;  ///< version number of hierarchy below

         bool       fAutoTime;          ///< when enabled, by node change (not hierarchy) time attribute will be set

         bool       fPermanent;         ///< indicate that item is permanent and should be excluded from update

         bool       fNodeChanged;       ///< indicate if something was changed in the node during update
         bool       fNamesChanged;      ///< indicate if DNS structure was changed (either childs or relevant dabc fields)
         bool       fChildsChanged;     ///< indicate if something was changed in the hierarchy

         bool       fDisableDataReading;    ///< when true, non of data (fields and history) need to be read in streamer
         bool       fDisableChildsReading;  ///< when true, non of childs should be read
         bool       fDisableReadingAsChild; ///< when true, object will not be updated when provided as child

         Buffer     fBinData;           ///< binary data, assigned with element

         History    fHist;              ///< special object with history data

         Mutex*     fHierarchyMutex;    ///< mutex, which should be use for access to hierarchy and all its childs

         HierarchyContainer* TopParent();

         std::string ItemName();

         /** \brief Create child with specified name
          * \details If indx value specified, child will be created or kept at this position
          * All intermediate childs will be removed
          * \returns pointer on the child */
         HierarchyContainer* CreateChildAt(const std::string &name, int indx);

         /** \brief Update hierarchy from provided container
          * \details Some field or childs could be extracted and change there ownership
          * \returns true when something was changed */
         bool UpdateHierarchyFrom(HierarchyContainer* cont);

         /** \brief Duplicate  hierarchy from provided container
          * \details Childs or fields can be updated or inserted, nothing will be deleted
          * \returns true when something was changed */
         bool DuplicateHierarchyFrom(HierarchyContainer* cont);

         /** \brief Switch on node or hierarchy modified flags */
         void SetModified(bool node, bool hierarchy, bool recursive = false);

         /** \brief Add new entry to history */
         void AddHistory(RecordFieldsMap* diff);

         /** \brief Extract values of specified history step */
         bool ExtractHistoryStep(RecordFieldsMap* fields, unsigned step);

         /** \brief Clear all entries in history, but not history object itself */
         void ClearHistoryEntries();

         /** Return true if history activated for the node
          * If necessary, history object will be initialized */
         bool CheckIfDoingHistory();

         /** \brief If item changed, marked with version, time stamp applied, history recording
          * returns mask with changes - 1 - any child node was changed, 2 - hierarchy was changed */
         unsigned MarkVersionIfChanged(uint64_t ver, uint64_t& tm, bool withchilds);

         /** \brief Mark reading flags */
         void MarkReading(bool withchilds, bool readvalues, bool readchilds);

         /** \brief Central method, which analyzes all possible changes in node (and its childs)
          * If any changes found, node marked with new version
          * If enabled, history item will be created.
          * If enabled, time stamp will be provided for changed items
          * \param tm provides time which will be set for changed items, if =0 (default), current time will be used */
         void MarkChangedItems(uint64_t tm = 0);

         /** \brief Enable time recording for hierarchy element every time when item is changed */
         void EnableTimeRecording(bool withchilds = true);

         virtual Object *CreateInstance(const std::string &name) { return new HierarchyContainer(name); }

         virtual void _ChildsChanged() { fNamesChanged = true; fChildsChanged = true; fNodeChanged = true;  }

         uint64_t GetNextVersion() const;

         void SetVersion(uint64_t v) { fNodeVersion = v; }

         void CreateHMutex();

         /** \brief Save hierarchy in json/xml form. */
         virtual bool SaveTo(HStore& res, bool create_node = true);

      public:
         HierarchyContainer(const std::string &name);
         virtual ~HierarchyContainer();

         virtual const char* ClassName() const { return "Hierarchy"; }

         uint64_t StoreSize(unsigned kind = stream_Full, uint64_t v = 0, unsigned hist_limit = 0);

         bool Stream(iostream& s, unsigned kind = stream_Full, uint64_t v = 0, unsigned hist_limit = 0);

         /** \brief Returns true if any node field was changed or removed/inserted
          * If specified, all childs will be checked */
         bool IsNodeChanged(bool withchilds = true);

         uint64_t GetVersion() const { return fNodeVersion; }

         uint64_t GetChildsVersion() const { return fChildsVersion; }

         void BuildObjectsHierarchy(const Reference& top);

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

      /** \brief Create top-level object with specified name */
      void Create(const std::string &name, bool withmutex = false);

      Mutex* GetHMutex() const { return null() ? 0 : GetObject()->fHierarchyMutex; }

      /** \brief Find master item
       * It is used in ROOT to specify position of streamer info */
      Hierarchy FindMaster() const;

      /** Search for parent element, where binary_producer property is specified
       * Returns name of binary producer and item name, which should be requested (relative to producer itself)
       * \par topmost identify where in hierarchy producer property will be searched */
      std::string FindBinaryProducer(std::string& request_name, bool topmost = true);

      //RecordField& Field(const std::string &name) { return GetObject()->Fields().Field(name); }
      const RecordField& Field(const std::string &name) const { return GetObject()->Fields().Field(name); }

      bool IsAnyFieldChanged() const { return GetObject() ? GetObject()->Fields().WasChanged() : false; }

      /** \brief Build objects hierarchy, referenced by top */
      void BuildNew(Reference top);

      /** \brief Mark item as permanent - it will not be touched when update from other places will be done */
      void SetPermanent(bool on = true) { if (GetObject()) GetObject()->fPermanent = on; }

      /** \brief Reconstruct complete hierarchy, setting node/structure modifications fields correctly
       * \details source hierarchy can be modified to reuse fields inside */
      bool Update(Hierarchy& src);

      /** \brief Duplicate hierarchy from the source.
       *  Existing items are preserved */
      bool Duplicate(const Hierarchy& src);

      /** \brief Activate history production for selected element and its childs */
      void EnableHistory(unsigned length = 100, bool withchilds = false);

      /** \brief Enable time recording for hierarchy element every time when item is changed */
      void EnableTimeRecording(bool withchilds = true)
         { if (GetObject()) GetObject()->EnableTimeRecording(withchilds); }

      /** \brief If any field was modified, item will be marked with new version */
      void MarkChangedItems(uint64_t tm = 0)
         { if (GetObject()) GetObject()->MarkChangedItems(tm); }

      /** \brief Returns true if item records history local, no need to request any other sources */
      bool HasLocalHistory() const;

      /** \brief Returns true if remote history is recorded and it is up-to-date */
      bool HasActualRemoteHistory() const;

      /** Save hierarchy in binary form, relative to specified version */
      dabc::Buffer SaveToBuffer(unsigned kind = stream_Full, uint64_t version = 0, unsigned hlimit = 0);

      /** Read hierarchy from buffer */
      bool ReadFromBuffer(const dabc::Buffer& buf);

      /** Apply modification to hierarchy, using stored binary data  */
      bool UpdateFromBuffer(const dabc::Buffer& buf, HierarchyStreamKind kind = stream_Full);

      /** \brief Returns actual version of hierarchy entry */
      uint64_t GetVersion() const { return GetObject() ? GetObject()->GetVersion() : 0; }

      /** \brief Change version of the item, only for advanced usage */
      void SetVersion(uint64_t v) { if (GetObject()) GetObject()->SetVersion(v); }

      /** \brief Return true if one could suppose that binary item is changed and
       *  binary data must be regenerated. First of all version is proved and than hash (if available) */
      bool IsBinItemChanged(const std::string &itemname, uint64_t hash, uint64_t last_version = 0);

      /** \brief Fill binary header with item and master versions */
      bool FillBinHeader(const std::string &itemname, dabc::Command& cmd, uint64_t mhash = 0, const std::string &dflt_master_name = "");

      /** \brief Return child element from hierarchy */
      Hierarchy FindChild(const char* name) { return Record::FindChild(name); }

      /** Removes folder and its parents as long as no other childs are present */
      bool RemoveEmptyFolders(const std::string &path);

      /** \brief Name which is used as item name in hierarchy. Name of top object is not included */
      std::string ItemName() const;

      /** \brief Returns reference on the top element of the hierarchy */
      Hierarchy GetTop() const;

      /** \brief Detach from parent object */
      bool DettachFromParent();

      /** \brief Mark all elements that non of data will be read */
      void DisableReading(bool withchlds = true);

      /** \brief Disable reading of element when it appears as child in the structure */
      void DisableReadingAsChild();

      /** \brief Enable element and all its parents to read data */
      void EnableReading(const Hierarchy &upto = nullptr);

      /** \brief Return child, if necessary creates with full subfolder
       * If force specified, missing childs and folders will be recreated
       * If allowslahes enabled, instead of subfolders item with coded name will be created */
      Hierarchy GetHChild(const std::string &name, bool allowslahes = false, bool force = false, bool sortorder = false);

      /** \brief Create child item in hierarchy with specified name
       * If allowslahes enabled, instead of subfolders item with coded name will be created */
      Hierarchy CreateHChild(const std::string &name, bool allowslahes = false, bool sortorder = false)
        { return GetHChild(name, allowslahes, true, sortorder); }

      /** Delete H item, including all empty parent folders */
      bool RemoveHChild(const std::string &name, bool allowslahes = false);

      /** \brief Create folder in hierarchy, one could use it to add new childs to it */
      Hierarchy CreateFolder(const std::string &name) { return GetHChild(name, true); }

      /** \brief Produce history iterator */
      HistoryIter MakeHistoryIter();
   };

}

#endif
