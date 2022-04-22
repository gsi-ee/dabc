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

#ifndef DABC_Record
#define DABC_Record

#ifndef DABC_Object
#include "dabc/Object.h"
#endif

#ifndef DABC_Buffer
#include "dabc/Buffer.h"
#endif

#include <vector>
#include <map>

namespace dabc {

   class ConfigIO;
   class DateTime;

   // =================================================================================

   class RecordFieldsMap;

   /** \brief class to stream binary data */

   class iostream {
      protected:
         bool fInput;

         /** \brief return number of bytes which are directly available in temporary buffer */
         virtual uint64_t tmpbuf_size() const { return 0; }
         /** \brief return temporary buffer */
         virtual char* tmpbuf() const { return nullptr; }

         iostream() : fInput(false) { }
         iostream(bool isinp) : fInput(isinp) { }

      public:
         virtual ~iostream() {}

         bool is_input() const { return fInput; }
         bool is_output() const { return !fInput; }
         virtual bool is_real() const { return true; }

         /** \brief Insted of reading object we read size and shift on that size
          * Only can be done where size stored as 32-bit integer in the current position
          * and contains number of 64-bit stored values, including size itself */
         bool skip_object();

         bool verify_size(uint64_t pos, uint64_t sz);

         virtual bool write(const void * /* src */, uint64_t /* len */) { return false; }
         virtual bool read(void * /* tgt */, uint64_t /* len */) { return false; }
         virtual bool shift(uint64_t /* len */) { return false; }
         /** \brief return number of bytes, written or read from the stream */
         virtual uint64_t size() const { return 0; }

         /** \brief return maximum size of data which can be stored in the stream */
         virtual uint64_t maxstoresize() const { return 0; }

         bool write_int32(int32_t v) { return write(&v, sizeof(int32_t)); }
         bool write_uint32(uint32_t v) { return write(&v, sizeof(uint32_t)); }
         bool write_int64(int64_t v) { return write(&v, sizeof(int64_t)); }
         bool write_uint64(uint64_t v) { return write(&v, sizeof(uint64_t)); }
         bool write_double(double v) { return write(&v, sizeof(double)); }

         bool read_int32(int32_t& v) { return read(&v, sizeof(int32_t)); }
         bool read_uint32(uint32_t& v) { return read(&v, sizeof(uint32_t)); }
         bool read_int64(int64_t& v) { return read(&v, sizeof(int64_t)); }
         bool read_uint64(uint64_t& v) { return read(&v, sizeof(uint64_t)); }
         bool read_double(double& v) { return read(&v, sizeof(double)); }


         /** \brief Returns bytes count, required to store string. Bytes count rounded to 64 bit */
         static uint64_t str_storesize(const std::string &str);

         /** \brief Store string in the stream */
         bool write_str(const std::string &str);

         /** \brief Restore string from the stream */
         bool read_str(std::string& str);
   };

   // ===================================================================================

   /** \brief special class only to define how many data will be written to the stream */
   class sizestream : public iostream {
      protected:
         uint64_t fLength{0};

      public:

         sizestream() : iostream(false)
         {
         }

         virtual ~sizestream() {}

         uint64_t size() const override { return fLength; }
         uint64_t maxstoresize() const  override { return 0xffffffffLLU; }

         bool is_real() const  override { return false; }

         bool shift(uint64_t len) override  { fLength += len; return true; }

         bool write(const void *src, uint64_t len) override  { (void)src; fLength += len; return true; }

         bool read(void */* tgt */, uint64_t /* len */) override { return true; }

   };

   // ===================================================================================

   /** \brief iostream class, which write and read data from memory */

   class memstream : public iostream {
      protected:
         char *fMem{nullptr};
         uint64_t fLength{0};
         char *fCurr{nullptr};
         uint64_t fRemains{0};

         uint64_t tmpbuf_size() const  override { return fRemains; }
         char* tmpbuf() const  override { return fCurr; }

      public:

         uint64_t size() const  override { return fCurr - fMem; }
         uint64_t maxstoresize() const  override { return fRemains; }

         bool shift(uint64_t len) override;
         bool write(const void* src, uint64_t len) override;
         bool read(void* tgt, uint64_t len) override;

         memstream(bool isinp, char* buf, uint64_t len) :
            iostream(isinp),
            fMem(buf),
            fLength(len),
            fCurr(buf),
            fRemains(len)
         {
         }

         virtual ~memstream() {}
   };

   // ===================================================================================


   enum HStoreMask {
      storemask_Compact =    0x03,   // 0..3 level of compactness
      storemask_Version =    0x04,   // write all versions
      storemask_TopVersion = 0x08,   // write version only for top node
      storemask_History =    0x10,   // write full history in the output
      storemask_NoChilds =   0x20,
      storemask_AsXML    =   0x80    // create XML output
   };

   // ===================================================================================

   /** \brief class, used for direct store of records in JSON/XML form */
   class HStore {
      protected:
         unsigned          fMask{0};
         std::string       buf;
         int               lvl{0};
         std::vector<int>  numflds;
         std::vector<int>  numchilds;
         uint64_t          fVersion{0};
         unsigned          fHLimit{0};
         bool              first_node{false};

         unsigned compact() const { return mask() & storemask_Compact; }

         void NewLine()
         {
            if (compact() < 2)
               buf.append("\n");
            else if (compact() < 3)
               buf.append(" ");
         }

         bool isxml() { return (mask() & storemask_AsXML) != 0; }

      public:
         HStore(unsigned m = 0) : fMask(m), buf(), lvl(0), numflds(), numchilds(), fVersion(0), fHLimit(0), first_node(true) {}
         virtual ~HStore() {}

         void SetLimits(uint64_t v, unsigned hlimit) { fVersion = v; fHLimit = hlimit; }

         unsigned mask() const { return fMask; }
         void SetMask(unsigned m) { fMask = m; }

         uint64_t version() const { return fVersion; }
         unsigned hlimit() const { return fHLimit; }

         std::string GetResult() { return buf; }

         void CreateNode(const char *nodename);
         void SetField(const char *name, const char * value);
         void BeforeNextChild(const char *basename = nullptr);
         void CloseChilds();
         void CloseNode(const char *nodename);
   };

   // =========================================================

   class RecordField {
      friend class RecordFieldsMap;

      protected:
         enum ValueKind {
            kind_none      = 0,
            kind_bool      = 1,
            kind_int       = 2,
            kind_datime    = 3,
            kind_uint      = 4,
            kind_double    = 5,
            kind_arrint    = 6,
            kind_arruint   = 7,
            kind_arrdouble = 8,
            kind_string    = 9,
            kind_arrstr    = 10,
            kind_buffer    = 11,
            kind_reference = 12
         };

         ValueKind fKind;

         union {
            int64_t valueInt;      /// scalar int type
            uint64_t valueUInt;    /// scalar unsigned int type
            double valueDouble;    /// scalar double type
         };

         union {
            int64_t* arrInt;       ///! int array, size in valueInt
            uint64_t* arrUInt;     ///! uint array, size in valueInt
            double* arrDouble;     ///! double array, size in valueInt
            char* valueStr;        ///! string or array of strings
            Buffer* valueBuf;      ///! buffer object
            Reference* valueRef;   ///! object reference
         };

         bool                fModified;    ///! when true, field was modified at least once
         bool                fTouched;     ///! flag, used to detect in streamer when field was touched at all
         bool                fProtected;   ///! when true, field will not be automatically deleted when full list updated from other hierarchy

         void release();

         bool modified(bool reallychanged = true)
         {
            if (reallychanged) fModified = true;
            return true;
         }

         bool isreadonly() const { return false; }
         bool cannot_modify();

         void SetArrStrDirect(int64_t size, char* arr, bool owner = false);

         void constructor() { fKind = kind_none; fModified = false; fTouched = false; fProtected = false; }

      public:
         RecordField();
         RecordField(const RecordField& src);
         RecordField(const char* v) { constructor(); SetStr(v); }
         RecordField(const std::string &v) { constructor(); SetStr(v); }
         RecordField(const int& v) { constructor(); SetInt(v); }
         RecordField(const int64_t& v) { constructor(); SetInt(v); }
         RecordField(const unsigned& v) { constructor(); SetUInt(v); }
         RecordField(const uint64_t& v) { constructor(); SetUInt(v); }
         RecordField(const double& v) { constructor(); SetDouble(v); }
         RecordField(const bool& v) { constructor(); SetBool(v); }
         RecordField(const DateTime& v) { constructor(); SetDatime(v); }
         RecordField(const std::vector<int64_t>& v) { constructor(); SetVectInt(v); }
         RecordField(const std::vector<uint64_t>& v) { constructor(); SetVectUInt(v); }
         RecordField(const std::vector<double>& v) { constructor(); SetVectDouble(v); }
         RecordField(const std::vector<std::string>& v) { constructor(); SetStrVect(v); }
         RecordField(const Buffer& buf) { constructor(); SetBuffer(buf); }
         RecordField(const Reference& ref) { constructor(); SetReference(ref); }

         RecordField& operator=(const RecordField& src) { SetValue(src); return *this; }

         virtual ~RecordField();

         bool null() const { return fKind == kind_none; }

         bool IsModified() const { return fModified; }
         void SetModified(bool on = true) { fModified = on; }

         bool IsProtected() const { return fProtected; }
         void SetProtected(bool on = true) { fProtected = on; }

         bool IsArray() const
         {
            return (fKind == kind_arrint) ||
                   (fKind == kind_arruint) ||
                   (fKind == kind_arrdouble) ||
                   (fKind == kind_arrstr);
         }
         int64_t GetArraySize() const { return IsArray() ? valueInt : -1; }

         bool AsBool(bool dflt = false) const;
         int64_t AsInt(int64_t dflt = 0) const;
         uint64_t AsUInt(uint64_t dflt = 0) const;
         double AsDouble(double dflt = 0.) const;
         std::string AsStr(const std::string &dflt = "") const;
         std::vector<int64_t> AsIntVect() const;
         int64_t* GetIntArr() const { return fKind == kind_arrint ? arrInt : 0; }
         std::vector<uint64_t> AsUIntVect() const;
         uint64_t* GetUIntArr() const { return fKind == kind_arruint ? arrUInt : 0; }
         std::vector<double> AsDoubleVect() const;
         double* GetDoubleArr() const { return fKind == kind_arrdouble ? arrDouble : 0; }
         std::vector<std::string> AsStrVect() const;
         dabc::Buffer AsBuffer() const;
         dabc::Reference AsReference() const;

         /** Returns field value in JSON format */
         std::string AsJson() const;

         bool SetValue(const RecordField& src);
         bool SetNull();
         bool SetBool(bool v);
         bool SetInt(int64_t v);
         bool SetDatime(uint64_t v);
         bool SetDatime(const DateTime& v);
         bool SetUInt(uint64_t v);
         bool SetDouble(double v);
         bool SetStr(const std::string &v);
         bool SetStr(const char* v);
         bool SetStrVect(const std::vector<std::string>& vect);
         bool SetBuffer(const Buffer& buf);
         bool SetReference(const Reference& ref);

         /** Set as array, if owner flag specified, one get ownership over array and do not need to create copy */
         bool SetArrInt(int64_t size, int64_t* arr, bool owner = false);
         bool SetVectInt(const std::vector<int64_t>& v);
         /** Set as array, if owner flag specified, one get ownership over array and do not need to create copy */
         bool SetArrUInt(int64_t size, uint64_t* arr, bool owner = false);
         bool SetVectUInt(const std::vector<uint64_t>& v);
         /** Set as array, if owner flag specified, one get ownership over array and do not need to create copy */
         bool SetArrDouble(int64_t size, double* arr, bool owner = false);
         bool SetVectDouble(const std::vector<double>& v);

         /** Sets as array of string, placed one after another in memory */
         bool SetArrStr(int64_t size, char* arr, bool owner = false);

         /** \return Total size, which is required to stream object */
         uint64_t StoreSize();
         bool Stream(iostream& s);

         static bool NeedJsonReformat(const std::string &str);
         static std::string JsonReformat(const std::string &str);

         static bool StrToStrVect(const char* str, std::vector<std::string>& vect, bool verbose = true);
   };

   class RecordFieldsMap {
      protected:
         typedef std::map<std::string, dabc::RecordField> FieldsMap;

         FieldsMap fMap;

         bool   fChanged{false};               ///< true when field was removed

         void clear() { fMap.clear(); }

         static bool match_prefix(const std::string &name, const std::string &prefix);

      public:

         RecordFieldsMap();
         virtual ~RecordFieldsMap();

         uint64_t StoreSize(const std::string &nameprefix = "");
         bool Stream(iostream& s, const std::string &nameprefix = "");

         bool HasField(const std::string &name) const;
         bool RemoveField(const std::string &name);

         unsigned NumFields() const { return fMap.size(); }
         std::string FieldName(unsigned n) const;

         /** \brief Direct access to the fields */
         RecordField& Field(const std::string &name) { return fMap[name]; }

         /** Save all field in json format */
         bool SaveTo(HStore& res);

         /** \brief Copy fields from source map */
         void CopyFrom(const RecordFieldsMap& src, bool overwrite = true);

         /** \brief Move fields from source map, delete no longer existing (except protected) */
         void MoveFrom(RecordFieldsMap& src);

         /** \brief  In the map only modified fields are remained
          * Also dabc:delete field can appear which marks all removed fields */
         void MakeAsDiffTo(const RecordFieldsMap& src);

         /** \brief Apply diff map
          * One should use fields, generated with MakeAsDiffTo call */
         void ApplyDiff(const RecordFieldsMap& diff);

         /** \brief Create complete copy of fields map */
         RecordFieldsMap* Clone();

         /** \brief Return true if any field was changed or map was modified (removed filed) */
         bool WasChanged() const;

         /** \brief Returns true when fields with specified prefix were changed */
         bool WasChangedWith(const std::string &prefix);

         /** \brief Clear all change flags */
         void ClearChangeFlags();
   };

   // ===========================================================================

   /** \brief Container for records fields
    *
    * \ingroup dabc_all_classes
    */

   class RecordContainer : public Object {
      friend class Record;
      friend class RecordField;
      friend class ConfigIO;
      friend class RecordFieldsMap;

      protected:
         RecordFieldsMap *fFields{nullptr};

         RecordContainer(const std::string &name, unsigned flags = flIsOwner);

         RecordContainer(Reference parent, const std::string &name, unsigned flags = flIsOwner);

         /** \brief Remove map and returns to the user.
          * It is user responsibility to correctly destroy it */
         RecordFieldsMap* TakeFieldsMap();

         /** \brief Replaces existing fields map */
         void SetFieldsMap(RecordFieldsMap* newmap);

         virtual bool HasField(const std::string &name) const
            { return Fields().HasField(name); }

         virtual bool RemoveField(const std::string &name)
            { return Fields().RemoveField(name); }

         virtual unsigned NumFields() const
            { return Fields().NumFields(); }

         virtual std::string FieldName(unsigned cnt) const
            { return Fields().FieldName(cnt); }

         virtual RecordField GetField(const std::string &name) const
            { return Fields().Field(name); }

         virtual bool SetField(const std::string &name, const RecordField& v)
            { return Fields().Field(name).SetValue(v); }

         virtual bool SaveTo(HStore& store, bool create_node = true);

      public:

         virtual ~RecordContainer();

         const char* ClassName() const override { return "Record"; }

         void Print(int lvl = 0) override;

         RecordFieldsMap& Fields() const { return *fFields; }
   };

   // ===================================================================================


   class Record : public Reference  {

      DABC_REFERENCE(Record, Reference, RecordContainer)

      bool HasField(const std::string &name) const
        { return null() ? false : GetObject()->HasField(name); }

      bool RemoveField(const std::string &name)
        { return null() ? false : GetObject()->RemoveField(name); }

      unsigned NumFields() const
        { return null() ? 0 : GetObject()->NumFields(); }

      std::string FieldName(unsigned cnt) const
        { return null() ? std::string() : GetObject()->FieldName(cnt); }

      RecordField GetField(const std::string &name) const
        { return null() ? RecordField() : GetObject()->GetField(name); }

      RecordField* GetFieldPtr(const std::string &name) const
        { return HasField(name) ?  &(GetObject()->Fields().Field(name)) : 0; }

      bool SetField(const std::string &name, const RecordField &v)
        { return null() ? false : GetObject()->SetField(name, v); }

      bool SetFieldModified(const std::string &name, bool on = true)
      {
         if (!HasField(name)) return false;
         GetObject()->Fields().Field(name).SetModified(on);
         return true;
      }

      bool SetFieldProtected(const std::string &name, bool on = true)
      {
         if (!HasField(name)) return false;
         GetObject()->Fields().Field(name).SetProtected(on);
         return true;
      }

      /** \brief Store hierarchy in json/xml form  */
      bool SaveTo(HStore &store, bool create_node = true)
      {  return null() ? false : GetObject()->SaveTo(store, create_node); }

      /** \brief Store record in JSON form */
      std::string SaveToJson(unsigned mask = 0);

      /** \brief Store record in XML form */
      std::string SaveToXml(unsigned mask = 0);

      bool Stream(iostream &s);

      dabc::Buffer SaveToBuffer();

      bool ReadFromBuffer(const dabc::Buffer &buf);

      virtual void CreateRecord(const std::string &name);

   };

}

#endif
