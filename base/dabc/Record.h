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

#ifndef DABC_XmlEngine
#include "dabc/XmlEngine.h"
#endif

#include <vector>
#include <map>
#include <stdint.h>

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
         virtual char* tmpbuf() const { return 0; }

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

         virtual bool write(const void* src, uint64_t len) { return false; }
         virtual bool read(void* tgt, uint64_t len) { return false; }
         virtual bool shift(uint64_t len) { return false; }
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
         static uint64_t str_storesize(const std::string& str);

         /** \brief Store string in the stream */
         bool write_str(const std::string& str);

         /** \brief Restore string from the stream */
         bool read_str(std::string& str);
   };

   /** \brief special class only to define how many data will be written to the stream */
   class sizestream : public iostream {
      protected:
         uint64_t fLength;

      public:

         sizestream() :
            iostream(false),
            fLength(0)
         {
         }

         virtual ~sizestream() {}

         virtual uint64_t size() const { return fLength; }
         virtual uint64_t maxstoresize() const { return 0xffffffffLLU; }

         virtual bool is_real() const { return false; }

         virtual bool shift(uint64_t len) { fLength += len; return true; }

         virtual bool write(const void* src, uint64_t len) { fLength += len; return true; }

         virtual bool read(void* tgt, uint64_t len) { return true; }

   };


   /** \brief iostream class, which write and read data from memory */

   class memstream : public iostream {
      protected:
         char* fMem;
         uint64_t fLength;
         char* fCurr;
         uint64_t fRemains;

         virtual uint64_t tmpbuf_size() const { return fRemains; }
         virtual char* tmpbuf() const { return fCurr; }

      public:

         virtual uint64_t size() const { return fCurr - fMem; }
         virtual uint64_t maxstoresize() const { return fRemains; }

         virtual bool shift(uint64_t len);
         virtual bool write(const void* src, uint64_t len);
         virtual bool read(void* tgt, uint64_t len);

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
            kind_arrstr    = 10
         };

         ValueKind fKind;

         union {
            int64_t valueInt;      /// scalar int type
            uint64_t valueUInt;    /// scalar int type
            double valueDouble;    /// scalar double type
         };

         union {
            int64_t* arrInt;       ///! int array, size in valueInt
            uint64_t* arrUInt;     ///! uint array, size in valueInt
            double* arrDouble;     ///! double array, size in valueInt
            char* valueStr;        ///! string or array of strings
         };

         bool                fModified;   ///! when true, field was modified at least once
         bool                fTouched;    ///! flag, used to detect in streamer when field was touched at all

         void release();

         bool modified(bool reallychanged = true)
         {
            if (reallychanged) fModified = true;
            return true;
         }

         bool isreadonly() const { return false; }
         bool cannot_modify();

         void SetArrStrDirect(int64_t size, char* arr, bool owner = false);

         void constructor() { fKind = kind_none; fModified = false; fTouched = false; }

      public:
         RecordField();
         RecordField(const RecordField& src);
         RecordField(const char* v) { constructor(); SetStr(v); }
         RecordField(const std::string& v) { constructor(); SetStr(v); }
         RecordField(const int& v) { constructor(); SetInt(v); }
         RecordField(const unsigned& v) { constructor(); SetUInt(v); }
         RecordField(const double& v) { constructor(); SetDouble(v); }
         RecordField(const bool& v) { constructor(); SetBool(v); }
         RecordField(const DateTime& v) { constructor(); SetDatime(v); }
         RecordField(const std::vector<uint64_t>& v) { constructor(); SetVectUInt(v); }
         RecordField(const std::vector<std::string>& v) { constructor(); SetStrVect(v); }

         RecordField& operator=(const RecordField& src) { SetValue(src); return *this; }

         virtual ~RecordField();

         bool null() const { return fKind == kind_none; }

         bool IsModified() const { return fModified; }
         void SetModified(bool on = true) { fModified = on; }

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
         std::string AsStr(const std::string& dflt = "") const;
         std::string AsStdStr(const std::string& dflt = "") const { return AsStr(dflt); }
         std::vector<int64_t> AsIntVect() const;
         int64_t* GetIntArr() const { return fKind == kind_arrint ? arrInt : 0; }
         std::vector<uint64_t> AsUIntVect() const;
         uint64_t* GetUIntArr() const { return fKind == kind_arruint ? arrUInt : 0; }
         std::vector<double> AsDoubleVect() const;
         double* GetDoubleArr() const { return fKind == kind_arrdouble ? arrDouble : 0; }
         std::vector<std::string> AsStrVect() const;

         bool SetValue(const RecordField& src);
         bool SetNull();
         bool SetBool(bool v);
         bool SetInt(int64_t v);
         bool SetDatime(uint64_t v);
         bool SetDatime(const DateTime& v);
         bool SetUInt(uint64_t v);
         bool SetDouble(double v);
         bool SetStr(const std::string& v);
         bool SetStr(const char* v);
         bool SetStrVect(const std::vector<std::string>& vect);

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

         static int NeedQuotes(const std::string& str);
         static std::string ExpandValue(const std::string& str);
         static std::string CompressValue(const char* str, int len);
   };

   class RecordFieldsMap {
      protected:
         typedef std::map<std::string, dabc::RecordField> FieldsMap;

         FieldsMap fMap;

         bool   fChanged;               ///< true when field was removed

         void clear() { fMap.clear(); }

      public:

         class ResolveFunc {
            public:
               ResolveFunc(unsigned = 0) {}
               virtual ~ResolveFunc() {}

               virtual const char* Resolve(const char* arg) const { return arg; }
         };

         RecordFieldsMap();
         virtual ~RecordFieldsMap();

         uint64_t StoreSize(const std::string& nameprefix = "");
         bool Stream(iostream& s, const std::string& nameprefix = "");

         bool HasField(const std::string& name) const;
         bool RemoveField(const std::string& name);

         unsigned NumFields() const { return fMap.size(); }
         std::string FieldName(unsigned n) const;
         std::string FindFieldWichStarts(const std::string& name);

         RecordField& Field(const std::string& name) { return fMap[name]; }

         bool SaveInXml(XMLNodePointer_t node);
         bool ReadFromXml(XMLNodePointer_t node, bool overwrite = true, const ResolveFunc& func = 0);

         /** \brief Copy fields from source map */
         void CopyFrom(const RecordFieldsMap& src, bool overwrite = true);

         /** \brief Move fields from source map, delete no longer existing,
          * one could exclude field names with prefix - typically 'dabc:' */
         void MoveFrom(RecordFieldsMap& src, const std::string& eclude_prefix = "");

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
         bool WasChangedWith(const std::string& prefix);

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
         RecordFieldsMap* fFields;

         RecordContainer(const std::string& name, unsigned flags = flIsOwner);

         RecordContainer(Reference parent, const std::string& name, unsigned flags = flIsOwner);

         /** \brief Remove map and returns to the user.
          * It is user responsibility to correctly destroy it */
         RecordFieldsMap* TakeFieldsMap();

         /** \brief Replaces existing fields map */
         void SetFieldsMap(RecordFieldsMap* newmap);

         virtual bool HasField(const std::string& name) const
            { return Fields().HasField(name); }

         virtual bool RemoveField(const std::string& name)
            { return Fields().RemoveField(name); }

         virtual unsigned NumFields() const
            { return Fields().NumFields(); }

         virtual std::string FieldName(unsigned cnt) const
            { return Fields().FieldName(cnt); }

         virtual RecordField GetField(const std::string& name) const
            { return Fields().Field(name); }

         virtual bool SetField(const std::string& name, const RecordField& v)
            { return Fields().Field(name).SetValue(v); }

      public:

         virtual ~RecordContainer();

         virtual const char* ClassName() const { return "Record"; }

         virtual void Print(int lvl = 0);

         RecordFieldsMap& Fields() const { return *fFields; }

         virtual XMLNodePointer_t SaveInXmlNode(XMLNodePointer_t parent);
   };

   // ===================================================================================


   class Record : public Reference  {

      DABC_REFERENCE(Record, Reference, RecordContainer)

      bool HasField(const std::string& name) const
        { return null() ? false : GetObject()->HasField(name); }

      bool RemoveField(const std::string& name)
        { return null() ? false : GetObject()->RemoveField(name); }

      unsigned NumFields() const
        { return null() ? 0 : GetObject()->NumFields(); }

      std::string FieldName(unsigned cnt) const
        { return null() ? std::string() : GetObject()->FieldName(cnt); }

      RecordField GetField(const std::string& name) const
        { return null() ? RecordField() : GetObject()->GetField(name); }

      bool SetField(const std::string& name, const RecordField& v)
        { return null() ? false : GetObject()->SetField(name, v); }

      bool SetFieldModified(const std::string& name, bool on = true)
      {
         if (!HasField(name)) return false;
         GetObject()->Fields().Field(name).SetModified(on);
         return true;
      }
   };

}

#endif
