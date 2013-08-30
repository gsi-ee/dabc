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

   class RecordContainerMap;
   class Record;
   class RecordField;
   class RecordContainer;
   class ConfigIO;

   /** \brief Represent single field of the record
    *
    * \ingroup dabc_all_classes
    */

   class RecordField {
      friend class Record;
      friend class RecordContainer;

      protected:
         RecordContainer* rec;
         const std::string& name;
         static std::string nullname;

         RecordField() : rec(0), name(nullname) {}
         RecordField(RecordContainer* _rec, const std::string& _name) : rec(_rec), name(_name) {}

         virtual RecordContainer* GetContainer() const { return 0; }

         virtual const char* Get(const char* dflt = 0) const;
         virtual bool IsEmpty() const;
         virtual bool Set(const char* val, const char* kind);

         /** \brief Method analyzes which kind of quotes is needed to add element to the array:
          * returns 0 - when no quotes is needed
          *         1 - when just quotes in the begin and in the end
          *         2 - when with quotes content should be reformatted */
         static int NeedQuotes(const char* value);

         /** \brief Expands value, that every special symbol like , or \ or " marked with preceding \
          * Later such special marks will be removed */
         static std::string ExpandValue(const char* value);


      public:

         // TODO: one should try to 'hide' this constructor signature while user can think to keep record field for longer time
         RecordField(const RecordField& src) : rec(src.rec), name(src.name) {}
         virtual ~RecordField() {}

         /** Return true if any value exists for the field */
         bool IsValue() const { return !IsEmpty(); }

         const char* AsStr(const char* dflt = 0) const { return Get(dflt); }
         const std::string AsStdStr(const std::string& dflt = "") const;
         std::vector<std::string> AsStrVector(const std::string& dflt = "") const;
         int AsInt(int dflt = 0) const;
         std::vector<int> AsIntVector(const std::string& dflt = "") const;
         double AsDouble(double dflt = 0.) const;
         std::vector<double> AsDoubleVector(const std::string& dflt = "") const;
         bool AsBool(bool dflt = false) const;
         std::vector<bool> AsBoolVector(const std::string& dflt = "") const;
         unsigned AsUInt(unsigned dflt = 0) const;
         std::vector<unsigned> AsUIntVector(const std::string& dflt = "") const;

         bool SetStr(const char* value);
         bool SetStr(const std::string& value);
         bool SetStrVector(const std::vector<std::string>& vect);
         bool SetInt(int v);
         bool SetIntVector(const std::vector<int>& vect);
         bool SetIntArray(const int* arr, unsigned size);
         bool SetDouble(double v);
         bool SetDoubleVector(const std::vector<double>& vect);
         bool SetDoubleArray(const double* arr, unsigned size);
         bool SetBool(bool v);
         bool SetBoolVector(const std::vector<bool>& vect);
         bool SetBoolArray(const bool* arr, unsigned size);
         bool SetUInt(unsigned v);
         bool SetUIntVector(const std::vector<unsigned>& vect);
         bool SetUIntArray(const unsigned* arr, unsigned size);

         bool DfltStr(const char* value) { return IsEmpty() ? SetStr(value) : false; }
         bool DfltStr(const std::string& value) { return IsEmpty() ? SetStr(value) : false; }
         bool DfltInt(int v) { return IsEmpty() ? SetInt(v) : false; }
         bool DfltDouble(double v) { return IsEmpty() ? SetDouble(v) : false; }
         bool DfltBool(bool v) { return IsEmpty() ? SetBool(v) : false; }
         bool DfltUInt(unsigned v) { return IsEmpty() ? SetUInt(v) : false; }

         static const char* kind_int() { return "int"; }
         static const char* kind_double() { return "double"; }
         static const char* kind_bool() { return "bool"; }
         static const char* kind_unsigned() { return "unsigned"; }
         static const char* kind_str() { return "str"; }
   };

   // __________________________________________________________________________________

   /** \brief Helper class to interpret value as record field
    *
    * \ingroup dabc_all_classes
    */

   class RecordValue : public RecordField {
      protected:
         virtual const char* Get(const char* dflt = 0) const;
         virtual bool IsEmpty() const { return false; }
         virtual bool Set(const char* val, const char* kind) { return false; }
      public:
         RecordValue(const std::string& value) : RecordField(0, value) {}
         virtual ~RecordValue() {}
   };

   // __________________________________________________________________________________

   /** \brief Container for records fields
    *
    * \ingroup dabc_all_classes
    */

   class RecordContainer : public Object {
      friend class Record;
      friend class RecordField;
      friend class ConfigIO;

      protected:
         RecordContainerMap* fPars;

         RecordContainer(const std::string& name, unsigned flags = flIsOwner);

         RecordContainer(Reference parent, const std::string& name, unsigned flags = flIsOwner);

         virtual const std::string DefaultFiledName() const;

         std::string FindField(const std::string& mask) const;

         virtual const char* GetField(const std::string& name, const char* dflt = 0);
         virtual bool SetField(const std::string& name, const char* value, const char* kind);
         virtual void ClearFields();

         /** \brief Returns pointer on field maps*/
         RecordContainerMap* GetFieldsMap();

         /** \brief Remove map and returns to the user.
          * It is user responsibility to correctly destroy it */
         RecordContainerMap* TakeFieldsMap();

         /** \brief Compare if field changed
          * \returns true when either number of fields or any field value is changed */
         bool CompareFields(RecordContainerMap* newmap, const char* extra_field = 0);

         /** Produces diff between old and new map */
         std::string BuildDiff(RecordContainerMap* newmap);

         /** \brief Copy all fields from specified map */
         void CopyFieldsMap(RecordContainerMap* src);

         /** \brief Replaces existing fields map */
         void SetFieldsMap(RecordContainerMap* newmap);

         /** \brief returns true when object field is exists */
         bool HasField(const std::string& name) { return GetField(name)!=0; }

         unsigned NumFields() const;
         const std::string FieldName(unsigned cnt) const;

         RecordField Field(const std::string& name) { return RecordField(this, name); }
         const RecordField Field(const std::string& name) const { return RecordField((RecordContainer*) this, name); }

      public:

         class ResolveFunc {
            public:
               ResolveFunc(unsigned = 0) {}
               virtual ~ResolveFunc() {}

               virtual const char* Resolve(const char* arg) const { return arg; }
         };


         virtual ~RecordContainer();

         virtual const char* ClassName() const { return "Record"; }

         virtual void Print(int lvl = 0);

         XMLNodePointer_t SaveInXmlNode(XMLNodePointer_t parent, bool withattr = true);

         bool SaveAttributesInXmlNode(XMLNodePointer_t node);

         bool ReadFieldsFromNode(XMLNodePointer_t node, bool overwrite = true, bool checkarray = false, const ResolveFunc& func = 0);

         std::string ReadArrayFromNode(XMLNodePointer_t node, const ResolveFunc& func);
   };

   // ______________________________________________________________________________

   /** \brief Structured data with possibility of converting to/from xml
    *
    * \ingroup dabc_core_classes
    * \ingroup dabc_all_classes
    *
    * Naming convention for record fields :
    *    1. Any field started with `#` will not be stored to the xml string
    *    2. Any field started with `_` will be stored as extra node in xml
    *    3. All other fields will be present attribute in main xml tag  `<Record attr=""/>`
    */

   class Record : public Reference, public RecordField  {

      DABC_REFERENCE(Record, Reference, RecordContainer)

      protected:

         virtual RecordContainer* GetContainer() const { return GetObject(); }

      public:

         virtual const char* GetField(const std::string& name, const char* dflt = 0) const;
         virtual bool SetField(const std::string& name, const char* value, const char* kind = 0);

         bool HasField(const std::string& name) const { return GetField(name) != 0; }
         bool RemoveField(const std::string& name) { return SetField(name, 0); }

         unsigned NumFields() const;
         const std::string FieldName(unsigned cnt) const;

         RecordField Field(const std::string& name) { return RecordField(GetObject(), name); }
         const RecordField Field(const std::string& name) const { return RecordField(GetObject(), name); }

         void AddFieldsFrom(const Record& src, bool can_owerwrite = true);

         std::string SaveToXml(bool compact=true);
         bool ReadFromXml(const char* xmlcode);

      protected:

         virtual bool CreateContainer(const std::string& name);
   };

   // =================================================================================

   class RecordFieldsMap;

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
         bool read_int64(int64_t v) { return read(&v, sizeof(int64_t)); }
         bool read_uint64(uint64_t v) { return read(&v, sizeof(uint64_t)); }
         bool read_double(double v) { return read(&v, sizeof(double)); }


         /** \brief Returns bytes count, required to store string. Bytes count rounded to 64 bit */
         static uint64_t str_storesize(const std::string& str);

         /** \brief Store string in the stream */
         bool write_str(const std::string& str);

         /** \brief Restore string from the stream */
         bool read_str(std::string& str);
   };

   class memstream : public iostream {
      protected:
         char* fMem;
         uint64_t fLength;
         char* fCurr;
         uint64_t fRemains;

         virtual uint64_t size() const { return fCurr - fMem; }
         virtual uint64_t maxstoresize() const { return fRemains; }
         virtual uint64_t tmpbuf_size() const { return fRemains; }
         virtual char* tmpbuf() const { return fCurr; }

         virtual bool shift(uint64_t len);

         virtual bool write(void* src, uint64_t len);

         virtual bool read(void* tgt, uint64_t len);

      public:

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


   class RecordFieldNew;

   class RecordFieldWatcher {
      public:
         virtual ~RecordFieldWatcher() {}
         virtual void FieldModified(RecordFieldNew* field) {}
   };


   class RecordFieldNew {
      friend class RecordFieldsMap;

      protected:
         enum ValueKind {
            kind_none      = 0,
            kind_bool      = 1,
            kind_int       = 2,
            kind_uint      = 3,
            kind_double    = 4,
            kind_arrint    = 5,
            kind_arruint   = 6,
            kind_arrdouble = 7,
            kind_string    = 8,
            kind_arrstr    = 9
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

         RecordFieldWatcher* fWatcher;
         bool                fReadonly;   ///! if true, field cannot be changed
         bool                fModified;   ///! when true, field was modified at least once

         void release();

         bool modified(bool reallychanged = true)
         {
            if (reallychanged) fModified = true;
            if (fWatcher) fWatcher->FieldModified(this);
            return true;
         }

         bool isreadonly() const { return fReadonly; }

         void SetArrStrDirect(int64_t size, char* arr, bool owner = false);

         /** \return Total size, which is required to stream object */
         uint64_t StoreSize() const;

         bool Stream(iostream& s);

      public:
         RecordFieldNew();
         RecordFieldNew(const RecordFieldNew& src);
         virtual ~RecordFieldNew();

         bool null() const { return fKind == kind_none; }

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
         std::vector<int64_t> AsIntVect() const;
         std::vector<uint64_t> AsUIntVect() const;
         std::vector<double> AsDoubleVect() const;
         std::vector<std::string> AsStrVect() const;

         bool SetNull();
         bool SetBool(bool v);
         bool SetUInt(uint64_t v);
         bool SetInt(int64_t v);
         bool SetDouble(double v);
         bool SetStr(const std::string& v);
         bool SetStr(const char* v);
         bool SetStrVect(const std::vector<std::string>& vect);

         /** Set as array, if owner flag specified, one get ownership over array and do not need to create copy */
         bool SetArrInt(int64_t size, int64_t* arr, bool owner = false);
         bool SetVectInt(const std::vector<int64_t>& v);
         /** Set as array, if owner flag specified, one get ownership over array and do not need to create copy */
         bool SetArrUInt(int64_t size, uint64_t* arr, bool owner = false);
         /** Set as array, if owner flag specified, one get ownership over array and do not need to create copy */
         bool SetArrDouble(int64_t size, double* arr, bool owner = false);
         bool SetVectDouble(const std::vector<double>& v);

         static int NeedQuotes(const std::string& str);
         static std::string ExpandValue(const std::string& str);
         static std::string CompressValue(const char* str, int len);
   };

   class RecordFieldsMap {
      protected:
         typedef std::map<std::string, dabc::RecordFieldNew> FieldsMap;

         FieldsMap fMap;

         uint64_t StoreSize() const;

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

         bool Stream(iostream& s);

         bool HasField(const std::string& name) const;
         bool RemoveField(const std::string& name);

         unsigned NumFields() const { return fMap.size(); }
         std::string FieldName(unsigned n) const;
         std::string FindFieldWichStarts(const std::string& name);

         RecordFieldNew& Field(const std::string& name);

         bool SaveInXml(XMLNodePointer_t node);
         bool ReadFromXml(XMLNodePointer_t node, bool overwrite = true, const ResolveFunc& func = 0);

         void CopyFrom(const RecordFieldsMap& src, bool overwrite = true);

         /** In the map only modified fields are remained
          * Also dabc:delete field can appear which marks all removed fields */
         void MakeAsDiffTo(const RecordFieldsMap& src);
   };

   // ===========================================================================

   /** \brief Container for records fields
    *
    * \ingroup dabc_all_classes
    */

   class RecordContainerNew : public Object {
      friend class RecordNew;
      friend class RecordFieldNew;
      friend class ConfigIO;
      friend class RecordFieldsMap;

      protected:
         RecordFieldsMap* fFields;

         RecordContainerNew(const std::string& name, unsigned flags = flIsOwner);

         RecordContainerNew(Reference parent, const std::string& name, unsigned flags = flIsOwner);

         /** \brief Remove map and returns to the user.
          * It is user responsibility to correctly destroy it */
         RecordFieldsMap* TakeFieldsMap();

         /** \brief Replaces existing fields map */
         void SetFieldsMap(RecordFieldsMap* newmap);

      public:

         virtual ~RecordContainerNew();

         virtual const char* ClassName() const { return "Record"; }

         virtual void Print(int lvl = 0);

         RecordFieldsMap& Fields() const { return *fFields; }

         XMLNodePointer_t SaveInXmlNode(XMLNodePointer_t parent);
   };

   // ===================================================================================


   class RecordNew : public Reference  {

      DABC_REFERENCE(RecordNew, Reference, RecordContainerNew)

      public:

         bool HasField(const std::string& name) const { return GetObject() ? GetObject()->Fields().HasField(name) : false; }
         bool RemoveField(const std::string& name) { return GetObject() ? GetObject()->Fields().RemoveField(name) : false; }

         unsigned NumFields() const { return GetObject() ? GetObject()->Fields().NumFields() : 0; }
         const std::string FieldName(unsigned cnt) const { return GetObject() ? GetObject()->Fields().FieldName(cnt) : std::string(); }

         RecordFieldNew& Field(const std::string& name) { return GetObject()->Fields().Field(name); }
         const RecordFieldNew& Field(const std::string& name) const { return GetObject()->Fields().Field(name); }

         void AddFieldsFrom(const RecordNew& src, bool can_owerwrite = true);

         std::string SaveToXml(bool compact=true);
         bool ReadFromXml(const char* xmlcode);

      protected:

         virtual bool CreateContainer(const std::string& name)
         {
            SetObject(new dabc::RecordContainerNew(name));
            return true;
         }
   };




}

#endif
