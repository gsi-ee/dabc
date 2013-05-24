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

      public:

         // TODO: one should try to 'hide' this constructor signature while user can think to keep record field for longer time
         RecordField(const RecordField& src) : rec(src.rec), name(src.name) {}
         virtual ~RecordField() {}

         /** Return true if any value exists for the field */
         bool IsValue() const { return !IsEmpty(); }

         const char* AsStr(const char* dflt = 0) const { return Get(dflt); }
         const std::string AsStdStr(const std::string& dflt = "") const;
         int AsInt(int dflt = 0) const;
         double AsDouble(double dflt = 0.) const;
         bool AsBool(bool dflt = false) const;
         unsigned AsUInt(unsigned dflt = 0) const;

         bool SetStr(const char* value);
         bool SetStr(const std::string& value);
         bool SetInt(int v);
         bool SetDouble(double v);
         bool SetBool(bool v);
         bool SetUInt(unsigned v);

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

         RecordContainer(const std::string& name);

         RecordContainer(Reference parent, const std::string& name);

         virtual const std::string DefaultFiledName() const;

         std::string FindField(const std::string& mask) const;

         virtual const char* GetField(const std::string& name, const char* dflt = 0);
         virtual bool SetField(const std::string& name, const char* value, const char* kind);

         bool HasField(const std::string& name) { return GetField(name)!=0; }

         unsigned NumFields() const;
         const std::string FieldName(unsigned cnt) const;

         RecordField Field(const std::string& name) { return RecordField(this, name); }
         const RecordField Field(const std::string& name) const { return RecordField((RecordContainer*) this, name); }

      public:
         virtual ~RecordContainer();

         virtual const char* ClassName() const { return "Record"; }

         virtual void Print(int lvl = 0);
   };

   // ______________________________________________________________________________

   /** \brief Structured data with possibility convert to/from xml
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
         bool ReadFromXml(const std::string& xml);

      protected:

         virtual bool CreateContainer(const std::string& name);
   };

}

#endif
