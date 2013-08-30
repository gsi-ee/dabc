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

#include "dabc/Record.h"

#include "dabc/threads.h"
#include "dabc/logging.h"
#include "dabc/XmlEngine.h"
#include "dabc/ConfigBase.h"
#include "dabc/Manager.h"
#include "dabc/Configuration.h"

#include <map>
#include <string.h>
#include <fnmatch.h>



// -----------------------------------------------

std::string dabc::RecordField::nullname = "";


const char* dabc::RecordField::Get(const char* dflt) const
{
   RecordContainer* r = rec ? rec : GetContainer();
   return r ? r->GetField(name) : dflt;
}

bool dabc::RecordField::IsEmpty() const
{
   RecordContainer* r = rec ? rec : GetContainer();
   return r ? !r->HasField(name) : true;
}

bool dabc::RecordField::Set(const char* val, const char* kind)
{
   RecordContainer* r = rec ? rec : GetContainer();
   return r ? r->SetField(name, val, kind) : false;
}

const std::string dabc::RecordField::AsStdStr(const std::string& dflt) const
{
   const char* res = Get();
   return res==0 ? dflt : std::string(res);
}

std::vector<std::string> dabc::RecordField::AsStrVector(const std::string& dflt) const
{
   std::string sarr = AsStdStr(dflt);

   std::vector<std::string> res;

   if (sarr.empty()) return res;

   if (sarr[0] != '[') { res.push_back(sarr); return res; }

   size_t pos = 1;

   while (pos < sarr.length()) {

      while (sarr[pos] == ' ') pos++;

      if (sarr[pos] != '\"') {
         // simple case, just find next ',' symbol
         size_t p0 = pos;
         while ((pos < sarr.length()) && (sarr[pos]!=',')) pos++;
         if (pos==sarr.length()) pos--;
         res.push_back(sarr.substr(p0, pos-p0));
         pos++;
      } else {
         std::string item;
         pos++;
         while (pos < sarr.length()) {
            if (sarr[pos] == '\"') { pos++; break; }
            if (sarr[pos] == '\\') pos++; // every first backslash is mark for special symbol
            item+=sarr[pos++]; // now just add any symbol
         }
         if (pos>=sarr.length()) {
            EOUT("Problem in array %s decoding", sarr.c_str());
            break;
         }
         if ((sarr[pos]!=',') && (sarr[pos]!=']')) {
            EOUT("Problem in array %s decoding", sarr.c_str());
            break;
         }
         res.push_back(item);
         pos++;
      }
   }

   return res;
}


int dabc::RecordField::AsInt(int dflt) const
{
   const char* val = Get();
   if (val==0) return dflt;
   int res(0);
   if (dabc::str_to_int(val, &res)) return res;
   return dflt;
}

#define GET_VECTOR_MACRO(typ,conv)                           \
   std::vector<std::string> svect = AsStrVector(dflt);       \
   std::vector<typ> res;                                     \
   for (unsigned n=0;n<svect.size();n++) {                   \
      typ val(0);                                            \
      if (!conv(svect[n].c_str(), &val)) val = 0;            \
      res.push_back(val);                                    \
   }                                                         \
   return res;



std::vector<int> dabc::RecordField::AsIntVector(const std::string& dflt) const
{
   GET_VECTOR_MACRO(int,dabc::str_to_int)
}


double dabc::RecordField::AsDouble(double dflt) const
{
   const char* val = Get();
   if (val==0) return dflt;
   double res = 0.;
   if (dabc::str_to_double(val, &res)) return res;
   return dflt;
}

std::vector<double> dabc::RecordField::AsDoubleVector(const std::string& dflt) const
{
   GET_VECTOR_MACRO(double,dabc::str_to_double)
}

bool dabc::RecordField::AsBool(bool dflt) const
{
   const char* val = Get();
   if (val==0) return dflt;

   bool res(false);
   if (dabc::str_to_bool(val, &res)) return res;
   return dflt;
}

std::vector<bool> dabc::RecordField::AsBoolVector(const std::string& dflt) const
{
   GET_VECTOR_MACRO(bool,dabc::str_to_bool)
}


unsigned dabc::RecordField::AsUInt(unsigned dflt) const
{
   const char* val = Get();
   if (val==0) return dflt;

   unsigned res = 0;
   if (dabc::str_to_uint(val, &res)) return res;
   return dflt;
}

std::vector<unsigned> dabc::RecordField::AsUIntVector(const std::string& dflt) const
{
   GET_VECTOR_MACRO(unsigned,dabc::str_to_uint)
}


bool dabc::RecordField::SetStr(const char* value)
{
   return Set(value, kind_str());
}

bool dabc::RecordField::SetStr(const std::string& value)
{
   return Set(value.c_str(), kind_str());
}

int dabc::RecordField::NeedQuotes(const char* value)
{
   if (value==0) return 0;
   if (*value==0) return 1;
   return strpbrk(value,"\"\\ ,[]")!=0 ? 2 : 1;
}

std::string dabc::RecordField::ExpandValue(const char* value)
{
   std::string res = "";
   const char* p = value;

   while (*p != 0) {
      switch(*p) {
         case '\\': res+="\\\\"; break;
         case '\"': res+="\\\""; break;
         default: res += *p;
      }
      p++;
   }

   return res;
}

bool dabc::RecordField::SetStrVector(const std::vector<std::string>& arr)
{
   if (arr.size()==0) return SetStr("");

   if (arr.size()==1) return SetStr(arr[0]);

   std::string sarr;
   for (unsigned n=0;n<arr.size();n++) {
      sarr += sarr.empty() ? "[" : ", ";
      switch (NeedQuotes(arr[n].c_str())) {
         case 1: sarr += "\""; sarr += arr[n]; sarr += "\""; break;
         case 2: sarr += "\""; sarr += ExpandValue(arr[n].c_str()); sarr += "\""; break;
         default: sarr+=arr[n]; break;
      }
   }
   sarr+=']';
   return SetStr(sarr);
}


bool dabc::RecordField::SetInt(int v)
{
   char buf[100];
   sprintf(buf,"%d",v);
   return Set(buf, kind_int());
}

#define SET_ARRAY_MACRO(SZ,FMT,func)        \
      std::string sarr;                     \
      char sbuf[100];                       \
      for (unsigned n=0;n<SZ;n++) {         \
         sarr += sarr.empty() ? "[" : ", "; \
         func(sbuf,FMT,arr[n]);             \
         sarr += sbuf;                      \
      }                                     \
      sarr+=']';                            \
      return SetStr(sarr);


bool dabc::RecordField::SetIntVector(const std::vector<int>& arr)
{
   if (arr.size()==0) return SetStr("");
   if (arr.size()==1) return SetInt(arr[0]);
   SET_ARRAY_MACRO(arr.size(), "%d", sprintf)
}

bool dabc::RecordField::SetIntArray(const int* arr, unsigned size)
{
   if ((arr==0) || (size==0)) return SetStr("");
   if (size==1) return SetInt(arr[0]);
   SET_ARRAY_MACRO(size, "%d", sprintf)
}

bool dabc::RecordField::SetDouble(double v)
{
   char buf[100];
   sprintf(buf,"%g",v);
   return Set(buf, kind_double());
}

bool dabc::RecordField::SetDoubleVector(const std::vector<double>& arr)
{
   if (arr.size()==0) return SetStr("");
   if (arr.size()==1) return SetDouble(arr[0]);
   SET_ARRAY_MACRO(arr.size(), "%g", sprintf)
}

bool dabc::RecordField::SetDoubleArray(const double* arr, unsigned size)
{
   if ((arr==0) || (size==0)) return SetStr("");
   if (size==1) return SetDouble(arr[0]);
   SET_ARRAY_MACRO(size, "%g", sprintf)
}

bool dabc::RecordField::SetBool(bool v)
{
   return Set(v ? xmlTrueValue : xmlFalseValue, kind_bool());
}

void scopybool(char* tgt, const char* fmt, bool val)
{
   strcpy(tgt, val ? dabc::xmlTrueValue : dabc::xmlFalseValue);
}


bool dabc::RecordField::SetBoolVector(const std::vector<bool>& arr)
{
   if (arr.size()==0) return SetStr("");
   if (arr.size()==1) return SetBool(arr[0]);
   SET_ARRAY_MACRO(arr.size(), "%b", scopybool)
}

bool dabc::RecordField::SetBoolArray(const bool* arr, unsigned size)
{
   if ((arr==0) || (size==0)) return SetStr("");
   if (size==1) return SetBool(arr[0]);
   SET_ARRAY_MACRO(size, "%b", scopybool)
}


bool dabc::RecordField::SetUInt(unsigned v)
{
   char buf[100];
   sprintf(buf, "%u", v);
   return Set(buf, kind_unsigned());
}

bool dabc::RecordField::SetUIntVector(const std::vector<unsigned>& arr)
{
   if (arr.size()==0) return SetStr("");
   if (arr.size()==1) return SetDouble(arr[0]);
   SET_ARRAY_MACRO(arr.size(), "%u", sprintf)
}

bool dabc::RecordField::SetUIntArray(const unsigned* arr, unsigned size)
{
   if ((arr==0) || (size==0)) return SetStr("");
   if (size==1) return SetDouble(arr[0]);
   SET_ARRAY_MACRO(size, "%u", sprintf)
}


// --------------------------------------------------------------------------------------

const char* dabc::RecordValue::Get(const char* dflt) const
{
   return name.c_str();
}


// ------------------------------------------------------------------------------------

namespace dabc {
   class RecordContainerMap : public std::map<std::string, std::string> {};
}


dabc::RecordContainer::RecordContainer(const std::string& name, unsigned flags) :
   Object(0, name, flags | flAutoDestroy),
   fPars(new RecordContainerMap)
{
}

dabc::RecordContainer::RecordContainer(Reference parent, const std::string& name, unsigned flags) :
   Object(MakePair(parent, name), flags | flAutoDestroy),
   fPars(new RecordContainerMap)
{
}

dabc::RecordContainer::~RecordContainer()
{
   delete fPars; fPars = 0;
}

dabc::RecordContainerMap* dabc::RecordContainer::GetFieldsMap()
{
   return fPars;
}


dabc::RecordContainerMap* dabc::RecordContainer::TakeFieldsMap()
{
   LockGuard lock(ObjectMutex());
   dabc::RecordContainerMap* res = fPars;
   fPars = new RecordContainerMap;
   return res;
}

void dabc::RecordContainer::CopyFieldsMap(RecordContainerMap* src)
{
   if (src==0) return;

   for (RecordContainerMap::iterator iter = src->begin(); iter != src->end(); iter++)
      (*fPars)[iter->first] = iter->second;
}


bool dabc::RecordContainer::CompareFields(RecordContainerMap* newmap, const char* extra_field)
{
   if (newmap == 0) return (fPars==0) || (fPars->size()==0);

   // first check that all fields in new map are present and do not changed
   for (RecordContainerMap::iterator iter = newmap->begin(); iter != newmap->end(); iter++) {

      RecordContainerMap::iterator iter2 = fPars->find(iter->first);

      if (iter2==fPars->end()) return false;

      if (iter->second != iter2->second) return false;
   }

   unsigned sz1 = fPars->size();
   unsigned sz2 = newmap->size();

   // in case of extra field like "time", check that it only present in the old map
   if (extra_field && (fPars->find(extra_field)!=fPars->end()) && (newmap->find(extra_field)==newmap->end())) sz1--;

   return sz1==sz2;
}



std::string dabc::RecordContainer::BuildDiff(RecordContainerMap* newmap)
{
   XMLNodePointer_t node = Xml::NewChild(0, 0, "h", 0);

   if (newmap!=0) {

      // one will need to put fields marking them as deleted
      if (fPars==0) fPars = new RecordContainerMap();

      for (RecordContainerMap::iterator iter = newmap->begin(); iter != newmap->end(); iter++) {
         RecordContainerMap::iterator iter2 = fPars->find(iter->first);

         // first of all just exclude all fields, which are the same
         if ((iter2!=fPars->end()) && (iter->second == iter2->second)) {
            fPars->erase(iter2);
            continue;
         }

         // second, mark fields which are new - in reverse direction it means delete
         if (iter2 == fPars->end()) {
            (*fPars)[iter->first] = "dabc_del";
         }
      }

      SaveAttributesInXmlNode(node);
   }

   std::string res;

   Xml::SaveSingleNode(node, &res, 0);
   Xml::FreeNode(node);

   return res;
}


void dabc::RecordContainer::SetFieldsMap(RecordContainerMap* newmap)
{
   LockGuard lock(ObjectMutex());
   delete fPars;
   fPars = newmap;
   if (fPars==0) fPars = new RecordContainerMap;
}


void dabc::RecordContainer::Print(int lvl)
{
   DOUT1("%s : %s", ClassName(), GetName());

   RecordContainerMap::const_iterator iter = fPars->begin();

   while (iter!=fPars->end()) {

      DOUT1("   %s = %s", iter->first.c_str(), iter->second.c_str());

      iter++;
   }
}

const std::string dabc::RecordContainer::DefaultFiledName() const
{
   return xmlValueAttr;
}

const char* dabc::RecordContainer::GetField(const std::string& name, const char* dflt)
{
   const std::string usename = name.empty() ? DefaultFiledName() : name;

   if (usename.empty()) return dflt;

   LockGuard lock(ObjectMutex());

   RecordContainerMap::const_iterator iter = fPars->find(usename);

   if (iter==fPars->end()) return dflt;

   return iter->second.c_str();

}

bool dabc::RecordContainer::SetField(const std::string& name, const char* value, const char* kind)
{
   const std::string usename = name.empty() ? DefaultFiledName() : name;

   if (usename.empty()) return false;

   LockGuard lock(ObjectMutex());

   if (value!=0) {
      (*fPars)[usename] = value;
      return true;
   }

   RecordContainerMap::iterator iter = fPars->find(usename);
   if (iter==fPars->end()) return false;

   fPars->erase(iter);
   return true;
}

void dabc::RecordContainer::ClearFields()
{
   LockGuard lock(ObjectMutex());
   fPars->clear();
}



std::string dabc::RecordContainer::FindField(const std::string& mask) const
{
   RecordContainerMap::const_iterator iter = fPars->begin();

   while (iter!=fPars->end()) {
      if (mask.empty() || (mask=="*") || (mask==iter->first)) return iter->first;

      if (mask.find_first_of("*?")!=mask.npos)
         if (fnmatch(mask.c_str(), iter->first.c_str(), FNM_NOESCAPE)==0) return iter->first;

      iter++;
   }

   return std::string();
}

unsigned dabc::RecordContainer::NumFields() const
{
   return fPars ? fPars->size() : 0;
}

const std::string dabc::RecordContainer::FieldName(unsigned cnt) const
{
   RecordContainerMap::const_iterator iter = fPars->begin();

   while (iter != fPars->end()) {
      if (cnt--==0) return iter->first;
      iter++;
   }
   return std::string();
}

bool dabc::RecordContainer::SaveAttributesInXmlNode(XMLNodePointer_t node)
{
   if (node == 0) return false;

   for(RecordContainerMap::const_iterator iter = fPars->begin();
         iter!=fPars->end();iter++) {

      if (iter->first.empty()) continue;

      if (iter->first[0]=='#') continue;

      // if somebody use wrong symbol in parameter name, pack it differently
      if (iter->first.find_first_of(" #&\"\'!@%^*()=-\\/|~.,")!=iter->first.npos) {
         XMLNodePointer_t child = Xml::NewChild(node, 0, "dabc:field", 0);
         Xml::NewAttr(child, 0, "name", iter->first.c_str());
         Xml::NewAttr(child, 0, "value", iter->second.c_str());
      } else {
         // add attribute
         Xml::NewAttr(node,0,iter->first.c_str(),iter->second.c_str());
      }
   }

   return true;
}


dabc::XMLNodePointer_t dabc::RecordContainer::SaveInXmlNode(XMLNodePointer_t parent, bool withattr)
{
   XMLNodePointer_t node = Xml::NewChild(parent, 0, GetName(), 0);

   if (withattr) SaveAttributesInXmlNode(node);

   return node;
}

bool dabc::RecordContainer::ReadFieldsFromNode(XMLNodePointer_t node, bool overwrite,  bool checkarray, const ResolveFunc& func)
{
   if (node==0) return false;

   XMLAttrPointer_t attr = Xml::GetFirstAttr(node);

   while (attr!=0) {
      const char* attrname = Xml::GetAttrName(attr);

      DOUT3("Cont:%p  attribute:%s overwrite:%s", this, attrname, DBOOL(overwrite));

      if (overwrite || (GetField(attrname)==0)) {
         SetField(attrname, func.Resolve(Xml::GetAttrValue(attr)), 0);
      }

      attr = Xml::GetNextAttr(attr);
   }

   if (checkarray) {
      std::string arr = ReadArrayFromNode(node, func);
      if (!arr.empty()) SetField("", arr.c_str(), 0);
   }

   XMLNodePointer_t child = Xml::GetChild(node);

   while (child!=0) {

      if (strcmp(Xml::GetNodeName(child), "dabc:field")==0) {

         const char* attrname = Xml::GetAttr(child,"name");

         if (attrname!=0)
            if (overwrite || (GetField(attrname)==0)) {
               const char* attrvalue = Xml::GetAttr(child,"value");

               if (attrvalue!=0)
                  SetField(attrname, func.Resolve(attrvalue), 0);
               else {
                  std::string arr = ReadArrayFromNode(child, func);
                  if (!arr.empty()) SetField(attrname, arr.c_str(), 0);
               }
            }
      }

      child = Xml::GetNext(child);
   }

   return true;
}

std::string dabc::RecordContainer::ReadArrayFromNode(XMLNodePointer_t node, const ResolveFunc& func)
{
   std::string arr;
   int size(0);

   XMLNodePointer_t child = Xml::GetChild(node);

   while (child!=0) {

      if (strcmp(Xml::GetNodeName(child), "item")==0) {

         const char* itemvalue = Xml::GetAttr(child,"value");

         if (itemvalue!=0) {
            itemvalue = func.Resolve(itemvalue);
            size++;

            if (arr.empty())
               arr += "[";
            else
               arr += ", ";

            switch (RecordField::NeedQuotes(itemvalue)) {
               case 1:
                  arr +="\"";
                  arr += itemvalue;
                  arr +="\"";
                  break;
               case 2:
                  arr +="\"";
                  arr += RecordField::ExpandValue(itemvalue);
                  arr +="\"";
               default:
                  arr += itemvalue;
            }
         }
      }

      child = Xml::GetNext(child);
   }

   if (size==1) arr.erase(0,1); else
   if (size>1) arr += "]";

   return arr;
}

//-------------------------------------------------------------------------

bool dabc::Record::CreateContainer(const std::string& name)
{
   SetObject(new dabc::RecordContainer(name));
   return true;
}

bool dabc::Record::SetField(const std::string& name, const char* value, const char* kind)
{
   return null() ? false : GetObject()->SetField(name, value, kind);
}

const char* dabc::Record::GetField(const std::string& name, const char* dflt) const
{
   return null() ? dflt : GetObject()->GetField(name, dflt);
}

unsigned dabc::Record::NumFields() const
{
   return null() ? 0 : GetObject()->NumFields();
}

const std::string dabc::Record::FieldName(unsigned cnt) const
{
   return null() ? std::string("") : GetObject()->FieldName(cnt);
}

void dabc::Record::AddFieldsFrom(const Record& src, bool can_owerwrite)
{
   if (null() || src.null()) return;

   RecordContainerMap::const_iterator iter = src()->fPars->begin();

   while (iter!=src()->fPars->end()) {

      if (can_owerwrite || !HasField(iter->first))
         SetField(iter->first, iter->second.c_str());

      iter++;
   }
}


std::string dabc::Record::SaveToXml(bool compact)
{
   XMLNodePointer_t node = GetObject() ? GetObject()->SaveInXmlNode(0, true) : 0;

   std::string res;

   if (node) {
      Xml::SaveSingleNode(node, &res, compact ? 0 : 1);
      Xml::FreeNode(node);
   }

   return res;
}


bool dabc::Record::ReadFromXml(const char* xmlcode)
{
   if ((xmlcode==0) || (*xmlcode==0)) return false;

   XMLNodePointer_t node = Xml::ReadSingleNode(xmlcode);

   if (node==0) return false;

   CreateContainer(Xml::GetNodeName(node));

   GetObject()->ReadFieldsFromNode(node, true, true);

   Xml::FreeNode(node);

   return true;
}


// ===========================================================================


bool dabc::iostream::verify_size(uint64_t pos, uint64_t sz)
{
   uint64_t actualsz = size() - pos;

   if (actualsz==sz) return true;

   if (actualsz > sz) {
      EOUT("Too many bytes %lu was accessed in stream compared to provided %lu", (long unsigned) actualsz, (long unsigned) sz);
      return false;
   }

   if (is_input()) return shift(sz-actualsz);

   while (actualsz < sz) {
      uint64_t buf(0);
      uint64_t portion = sz - actualsz;
      if (portion>8) portion = 8;
      write(&buf, portion);
      actualsz += portion;
   }
   return true;
}


uint64_t dabc::iostream::str_storesize(const std::string& str)
{
   uint64_t res = sizeof(int32_t);
   res += str.length() + 1; // null-terminated string will be stored
   while (res % 8 != 0) res++;
   return res;
}

bool dabc::iostream::write_str(const std::string& str)
{
   uint64_t sz = str_storesize(str);
   if (sz > maxstoresize()) {
      EOUT("Cannot store string in the buffer");
      return 0;
   }

   uint64_t pos = size();

   uint32_t storesz = sz / 8;
   write_uint32(storesz);
   write(str.c_str(), str.length()+1);

   return verify_size(pos, sz);
}

bool dabc::iostream::read_str(std::string& str)
{
   uint64_t pos = size();

   uint32_t storesz(0);
   read_uint32(storesz);
   uint64_t sz = ((uint64_t) storesz) * 8;

   if (sz > tmpbuf_size()) {
      EOUT("Cannot read complete string!!!");
      return false;
   }

   str.clear();
   str.append(tmpbuf()); // null-terminated string
   return verify_size(pos, sz);
}

// ===========================================================================

bool dabc::memstream::shift(uint64_t len)
{
   if (len > fRemains) return false;
   fCurr+=len;
   fRemains-=len;
   return true;
}

bool dabc::memstream::write(void* src, uint64_t len)
{
   if (len > fRemains) return false;
   memcpy(fCurr, src, len);
   fCurr+=len;
   fRemains-=len;
   return true;
}


bool dabc::memstream::read(void* tgt, uint64_t len)
{
   if (len > fRemains) return false;
   memcpy(tgt, fCurr, len);
   fCurr+=len;
   fRemains-=len;
   return true;
}

// ===========================================================================




dabc::RecordFieldNew::RecordFieldNew() :
   fKind(kind_none),
   fWatcher(0),
   fReadonly(false),
   fModified(false)
{
}

dabc::RecordFieldNew::RecordFieldNew(const RecordFieldNew& src)
{
   fKind = kind_none;
   fWatcher = 0; // watcher is not copied
   fReadonly = false; // readonly also not copied
   fModified = false;

   switch (src.fKind) {
      case kind_none: break;
      case kind_bool: SetBool(src.valueInt!=0); break;
      case kind_int: SetInt(src.valueInt); break;
      case kind_uint: SetUInt(src.valueUInt); break;
      case kind_double: SetDouble(src.valueDouble); break;
      case kind_arrint: SetArrInt(src.valueInt, (int64_t*) src.arrInt); break;
      case kind_arruint: SetArrUInt(src.valueInt, (uint64_t*) src.arrUInt); break;
      case kind_arrdouble: SetArrDouble(src.valueInt, (double*) src.arrDouble); break;
      case kind_string: SetStr(src.valueStr); break;
      case kind_arrstr: SetArrStrDirect(src.valueInt, src.valueStr); break;
   }
}


dabc::RecordFieldNew::~RecordFieldNew()
{
   release();
}

uint64_t dabc::RecordFieldNew::StoreSize() const
{
   uint64_t sz = sizeof(int32_t) + sizeof(int32_t); // this is required for the size and kind

   switch (fKind) {
      case kind_none: break;
      case kind_bool:
      case kind_int: sz += sizeof(int64_t); break;
      case kind_uint: sz += sizeof(uint64_t); break;
      case kind_double: sz += sizeof(double); break;
      case kind_arrint: sz += sizeof(int64_t) + valueInt*sizeof(int64_t); break;
      case kind_arruint: sz += sizeof(int64_t) + valueInt*sizeof(uint64_t); break;
      case kind_arrdouble: sz += sizeof(int64_t) + valueInt*sizeof(double); break;
      case kind_string: sz += strlen(valueStr)+1; break;
      case kind_arrstr: {
         sz += sizeof(int64_t);
         char* p = valueStr;
         for (int64_t n=0;n<valueInt;n++) {
            int len = strlen(p) + 1;
            sz += len; p += len;
         }
         break;
      }
   }

   // all sizes round to 8 bytes boundary, such size value will be stored
   while (sz % 8 != 0) sz++;

   return sz;
}

bool dabc::RecordFieldNew::Stream(iostream& s)
{
   uint64_t pos = s.size();
   uint64_t sz = 0;

   uint32_t storesz(0), storekind(0), storeversion(0);

   if (s.is_output()) {
      sz = StoreSize();
      storesz = sz / 8;
      storekind = ((uint32_t)fKind & 0xffffff) | (storeversion << 24);
      s.write_uint32(storesz);
      s.write_uint32(storekind);
      switch (fKind) {
         case kind_none: break;
         case kind_bool:
         case kind_int: s.write_int64(valueInt); break;
         case kind_uint: s.write_uint64(valueUInt); break;
         case kind_double: s.write_double(valueDouble); break;
         case kind_arrint:
            s.write_int64(valueInt);
            s.write(arrInt, valueInt*sizeof(int64_t));
            break;
         case kind_arruint:
            s.write_int64(valueInt);
            s.write(arrUInt, valueInt*sizeof(uint64_t));
            break;
         case kind_arrdouble:
            s.write_int64(valueInt);
            s.write(arrDouble, valueInt*sizeof(double));
            break;
         case kind_string:
            s.write(valueStr, strlen(valueStr)+1);
            break;
         case kind_arrstr: {
            int64_t fulllen = 0;
            char* ps = valueStr;
            for (int64_t n=0;n<valueInt;n++) {
               int len = strlen(ps) + 1;
               fulllen += len; ps += len;
            }
            s.write_int64(valueInt);
            s.write(valueStr, fulllen);
            break;
         }
      }
   } else {
      release();
      s.read_uint32(storesz);
      s.read_uint32(storekind);
      sz = ((uint64_t) storesz) * 8;
      // storeversion = storekind >> 24;

      fKind = (ValueKind) (storekind & 0xffffff);
      switch (fKind) {
         case kind_none: break;
         case kind_bool:
         case kind_int: s.read_int64(valueInt); break;
         case kind_uint: s.read_uint64(valueUInt); break;
         case kind_double: s.read_double(valueDouble); break;
         case kind_arrint:
            s.read_int64(valueInt);
            arrInt = new int64_t[valueInt];
            s.read(arrInt, valueInt*sizeof(int64_t));
            break;
         case kind_arruint:
            s.read_int64(valueInt);
            arrUInt = new uint64_t[valueInt];
            s.read(arrUInt, valueInt*sizeof(uint64_t));
            break;
         case kind_arrdouble:
            s.read_int64(valueInt);
            arrDouble = new double[valueInt];
            s.read(arrDouble, valueInt*sizeof(double));
            break;
         case kind_string:
            valueStr = (char*) malloc((storesz-1)*8);
            s.read(valueStr, (storesz-1)*8);
            break;
         case kind_arrstr: {
            s.read_int64(valueInt);
            valueStr = (char*) malloc((storesz-2)*8);
            s.read(valueStr, (storesz-2)*8);
            break;
         }
      }
   }

   return s.verify_size(pos, sz);
}

void dabc::RecordFieldNew::release()
{
   switch (fKind) {
      case kind_none: break;
      case kind_bool: break;
      case kind_int: break;
      case kind_uint: break;
      case kind_double: break;
      case kind_arrint: delete [] arrInt; break;
      case kind_arruint: delete [] arrUInt; break;
      case kind_arrdouble: delete [] arrDouble; break;
      case kind_string:
      case kind_arrstr: free(valueStr); break;
   }

   fKind = kind_none;
}

bool dabc::RecordFieldNew::AsBool(bool dflt) const
{
   switch (fKind) {
      case kind_none: return dflt;
      case kind_bool:
      case kind_int: return valueInt!=0;
      case kind_uint: return valueUInt!=0;
      case kind_double: return valueDouble!=0.;
      case kind_arrint: if (valueInt>0) return arrInt[0]!=0; break;
      case kind_arruint: if (valueInt>0) return arrUInt[0]!=0; break;
      case kind_arrdouble: if (valueInt>0) return arrDouble[0]!=0; break;
      case kind_string:
      case kind_arrstr: {
         if (strcmp(valueStr,xmlTrueValue)) return true;
         if (strcmp(valueStr,xmlFalseValue)) return false;
         break;
      }
   }
   return dflt;
}

int64_t dabc::RecordFieldNew::AsInt(int64_t dflt) const
{
   switch (fKind) {
      case kind_none: return dflt;
      case kind_bool:
      case kind_int: return valueInt;
      case kind_uint: return (int64_t) valueUInt;
      case kind_double: return (int64_t) valueDouble;
      case kind_arrint: if (valueInt>0) return arrInt[0]; break;
      case kind_arruint: if (valueInt>0) return (int64_t) arrUInt[0]; break;
      case kind_arrdouble: if (valueInt>0) return (int64_t) arrDouble[0]; break;
      case kind_string:
      case kind_arrstr: {
         long res;
         if (str_to_lint(valueStr, &res)) return res;
         break;
      }
   }
   return dflt;
}

uint64_t dabc::RecordFieldNew::AsUInt(uint64_t dflt) const
{
   switch (fKind) {
      case kind_none: return dflt;
      case kind_bool:
      case kind_int: return (uint64_t) valueInt;
      case kind_uint: return valueUInt;
      case kind_double: return (uint64_t) valueDouble;
      case kind_arrint: if (valueInt>0) return (uint64_t) arrInt[0]; break;
      case kind_arruint: if (valueInt>0) return arrUInt[0]; break;
      case kind_arrdouble: if (valueInt>0) return (uint64_t) arrDouble[0]; break;
      case kind_string:
      case kind_arrstr: {
         long unsigned res;
         if (str_to_luint(valueStr, &res)) return res;
         break;
      }
   }
   return dflt;
}

double dabc::RecordFieldNew::AsDouble(double dflt) const
{
   switch (fKind) {
      case kind_none: return dflt;
      case kind_bool:
      case kind_int: return (double) valueInt;
      case kind_uint: return (double) valueUInt;
      case kind_double: return valueDouble;
      case kind_arrint: if (valueInt>0) return (double) arrInt[0]; break;
      case kind_arruint: if (valueInt>0) return (double) arrUInt[0]; break;
      case kind_arrdouble: if (valueInt>0) return arrDouble[0]; break;
      case kind_string:
      case kind_arrstr: {
         double res;
         if (str_to_double(valueStr, &res)) return res;
         break;
      }
   }
   return dflt;
}

std::vector<int64_t> dabc::RecordFieldNew::AsIntVect() const
{
   std::vector<int64_t> res;

   switch (fKind) {
      case kind_none: break;
      case kind_bool:
      case kind_int: res.push_back(valueInt); break;
      case kind_uint: res.push_back((int64_t) valueUInt); break;
      case kind_double: res.push_back((int64_t) valueDouble); break;
      case kind_arrint:
         res.reserve(valueInt);
         for (int64_t n=0;n<valueInt;n++)
            res.push_back(arrInt[n]);
         break;
      case kind_arruint:
         res.reserve(valueInt);
         for (int64_t n=0;n<valueInt;n++)
            res.push_back((int64_t) arrUInt[n]);
         break;
      case kind_arrdouble:
         res.reserve(valueInt);
         for (int64_t n=0;n<valueInt;n++)
            res.push_back((int64_t) arrDouble[n]);
         break;
      case kind_string: {
         long res0;
         if (str_to_lint(valueStr, &res0)) res.push_back(res0);
         break;
      }
      case kind_arrstr: {
         res.reserve(valueInt);
         long res0;
         char* p = valueStr;

         for (int64_t n=0;n<valueInt;n++) {
            if (!str_to_lint(p, &res0)) res0 = 0;
            res.push_back(res0);
            p += strlen(p)+1;
         }
         break;
      }

   }

   return res;
}

std::vector<uint64_t> dabc::RecordFieldNew::AsUIntVect() const
{
   std::vector<uint64_t> res;

   switch (fKind) {
      case kind_none: break;
      case kind_bool:
      case kind_int: res.push_back((uint64_t) valueInt); break;
      case kind_uint: res.push_back(valueUInt); break;
      case kind_double: res.push_back((uint64_t) valueDouble); break;
      case kind_arrint:
         res.reserve(valueInt);
         for (int64_t n=0;n<valueInt;n++)
            res.push_back((uint64_t) arrInt[n]);
         break;
      case kind_arruint:
         res.reserve(valueInt);
         for (int64_t n=0;n<valueInt;n++)
            res.push_back(arrUInt[n]);
         break;
      case kind_arrdouble:
         res.reserve(valueInt);
         for (int64_t n=0;n<valueInt;n++)
            res.push_back((uint64_t) arrDouble[n]);
         break;
      case kind_string: {
         long unsigned res0;
         if (str_to_luint(valueStr, &res0)) res.push_back(res0);
         break;
      }
      case kind_arrstr: {
         res.reserve(valueInt);
         long unsigned res0;
         char* p = valueStr;

         for (int64_t n=0;n<valueInt;n++) {
            if (!str_to_luint(p, &res0)) res0 = 0;
            res.push_back(res0);
            p += strlen(p)+1;
         }
         break;
      }
   }

   return res;
}


std::vector<double> dabc::RecordFieldNew::AsDoubleVect() const
{
   std::vector<double> res;

   switch (fKind) {
      case kind_none: break;
      case kind_bool:
      case kind_int: res.push_back((double) valueInt); break;
      case kind_uint: res.push_back((double) valueUInt); break;
      case kind_double: res.push_back(valueDouble); break;
      case kind_arrint:
         res.reserve(valueInt);
         for (int64_t n=0;n<valueInt;n++)
            res.push_back((double) arrInt[n]);
         break;
      case kind_arruint:
         res.reserve(valueInt);
         for (int64_t n=0;n<valueInt;n++)
            res.push_back((double) arrUInt[n]);
         break;
      case kind_arrdouble:
         res.reserve(valueInt);
         for (int64_t n=0;n<valueInt;n++)
            res.push_back(arrDouble[n]);
         break;
      case kind_string: {
         double res0;
         if (str_to_double(valueStr, &res0)) res.push_back(res0);
         break;
      }
      case kind_arrstr: {
         res.reserve(valueInt);
         double res0;
         char* p = valueStr;

         for (int64_t n=0;n<valueInt;n++) {
            if (!str_to_double(p, &res0)) res0 = 0.;
            res.push_back(res0);
            p += strlen(p)+1;
         }
         break;
      }
   }

   return res;
}

int dabc::RecordFieldNew::NeedQuotes(const std::string& str)
{
   if (str.length()==0) return 1;

   if (str.find_first_of("\'\"<>&")!=std::string::npos) return 2;

   return (str.find_first_of(" ,[]")!=std::string::npos) ? 1 : 0;
}

std::string dabc::RecordFieldNew::ExpandValue(const std::string& str)
{
   std::string res;

   for (unsigned n=0;n<str.length();n++)
      switch(str[n]) {
         case '\"': res.append("&quout;"); break;
         case '\'': res.append("&apos;"); break;
         case '<': res.append("&lt;"); break;
         case '>': res.append("&gt;"); break;
         case '&': res.append("&amp;"); break;
         default: res+=str[n]; break;
      }

   return res;
}

std::string dabc::RecordFieldNew::CompressValue(const char* str, int len)
{
   std::string res;
   int n=0;
   while (n<len) {
      if (str[n]!='&') { res+=str[n++]; continue; }

      if ((len - n >= 7) && (strncmp(str+n, "&quout;", 7)==0)) { res+="\""; n+=7; continue; }
      if ((len - n >= 6) && (strncmp(str+n, "&apos;", 6)==0)) { res+="\'"; n+=6; continue; }
      if ((len - n >= 4) && (strncmp(str+n, "&lt;", 4)==0)) { res+="<"; n+=4; continue; }
      if ((len - n >= 4) && (strncmp(str+n, "&gt;", 4)==0)) { res+=">"; n+=4; continue; }
      if ((len - n >= 5) && (strncmp(str+n, "&amp;", 5)==0)) { res+="&"; n+=5; continue; }

      res+=str[n++];
   }

   return res;

}


std::string dabc::RecordFieldNew::AsStr(const std::string& dflt) const
{
   switch (fKind) {
      case kind_none: return dflt;
      case kind_bool: return valueInt!=0 ? xmlTrueValue : xmlFalseValue;
      case kind_int: return dabc::format("%ld", (long) valueInt);
      case kind_uint: return dabc::format("%lu", (long unsigned) valueUInt);
      case kind_double: return dabc::format("%g", valueDouble);
      case kind_arrint:
      case kind_arruint:
      case kind_arrdouble:
      case kind_arrstr: {

         std::vector<std::string> vect = AsStrVect();
         if (vect.size()==0) return dflt;

         std::string res("[");
         for (unsigned n=0; n<vect.size();n++) {
            if (n>0) res.append(",");
            switch (NeedQuotes(vect[n])) {
               case 1:
                  res.append("\'");
                  res.append(vect[n]);
                  res.append("\'");
                  break;
               case 2:
                  res.append("\'");
                  res.append(ExpandValue(vect[n]));
                  res.append("\'");
                  break;
               default:
                  res.append(vect[n]);
                  break;
            }
         }
         res.append("]");
         return res;

         break;
      }
      case kind_string: {
         if (valueStr!=0) return valueStr;
         break;
      }
   }
   return dflt;
}

std::vector<std::string> dabc::RecordFieldNew::AsStrVect() const
{
   std::vector<std::string> res;

   switch (fKind) {
      case kind_none: break;
      case kind_bool:
      case kind_int:
      case kind_uint:
      case kind_double: res.push_back(AsStr()); break;
      case kind_arrint: {
         for (int64_t n=0; n<valueInt;n++)
            res.push_back(dabc::format("%ld", (long) arrInt[n]));
         break;
      }
      case kind_arruint: {
         for (int64_t n=0; n<valueInt;n++)
            res.push_back(dabc::format("%lu", (long unsigned) arrUInt[n]));
         break;
      }
      case kind_arrdouble: {
         for (int64_t n=0; n<valueInt;n++)
            res.push_back(dabc::format("%g", arrDouble[n]));
         break;
      }
      case kind_string: {
         // TODO: check that valueStr correspond to vector syntax
         int len = strlen(valueStr);
         if ((len>1) && (valueStr[0]=='[') && (valueStr[len-1]==']')) {
            // try to extract string vector
            char* pos = valueStr + 1;
            while (*pos==' ') pos++; // exclude possible spaces in the begin
            if (*pos=='\'') {
               char* p1 = strchr(pos+1, '\'');
               if (p1==0) {
                  EOUT("Syntax error in array");
                  break;
               }
               res.push_back(CompressValue(pos+1, p1 - pos - 1));
            } else {
               char* p1 = strpbrk(pos+1, ",]");
               if (p1==0) {
                  EOUT("Syntax error in array");
               }
               int spaces = 0;
               while ((p1 - spaces > pos + 1) && (*(p1-spaces-1)==' ')) spaces++;
               res.push_back(std::string(pos, p1 - pos - spaces));
            }
            break;
         }
         res.push_back(valueStr);
         break;
      }
      case kind_arrstr: {
         char* p = valueStr;
         for (int64_t n=0;n<valueInt;n++) {
            res.push_back(p);
            p += strlen(p)+1;
         }
         break;
      }
   }

   return res;
}


bool dabc::RecordFieldNew::SetNull()
{
   if (isreadonly()) return false;
   if (fKind == kind_none) return modified(false);
   release();
   return modified();
}


bool dabc::RecordFieldNew::SetBool(bool v)
{
   if (isreadonly()) return false;

   if ((fKind == kind_bool) && (valueInt == (v ? 1 : 0))) return modified(false);

   release();
   fKind = kind_bool;
   valueInt = v ? 1 : 0;

   return modified();
}

bool dabc::RecordFieldNew::SetInt(int64_t v)
{
   if (isreadonly()) return false;
   if ((fKind == kind_int) && (valueInt == v)) return modified(false);
   release();
   fKind = kind_int;
   valueInt = v;
   return modified();
}

bool dabc::RecordFieldNew::SetUInt(uint64_t v)
{
   if (isreadonly()) return false;

   if ((fKind == kind_uint) &&  (valueUInt == v)) return modified(false);

   release();
   fKind = kind_uint;
   valueUInt = v;

   return modified();
}

bool dabc::RecordFieldNew::SetDouble(double v)
{
   if (isreadonly()) return false;

   if ((fKind == kind_double) && (valueDouble == v)) return modified(false);

   release();
   fKind = kind_double;
   valueDouble = v;

   return modified();
}

bool dabc::RecordFieldNew::SetStr(const std::string& v)
{
   if (isreadonly()) return false;

   if ((fKind == kind_string) && (v==valueStr)) return modified(false);

   release();
   fKind = kind_string;
   valueStr = (char*) malloc(v.length()+1);
   strcpy(valueStr, v.c_str());

   return modified();
}

bool dabc::RecordFieldNew::SetStr(const char* v)
{
   if (isreadonly()) return false;

   if ((fKind == kind_string) && (v!=0) && (strcmp(v,valueStr)==0)) return modified(false);

   release();
   fKind = kind_string;
   size_t len = v==0 ? 0 : strlen(v);
   valueStr = (char*) malloc(len+1);
   if (v!=0) strcpy(valueStr, v);
        else *valueStr = 0;

   return modified();
}

bool dabc::RecordFieldNew::SetStrVect(const std::vector<std::string>& vect)
{
   if (isreadonly()) return false;

   if ((fKind == kind_arrstr) && (valueInt == (int64_t) vect.size())) {
      std::vector<std::string> vect0 = AsStrVect();
      if (vect0.size() == vect.size()) {
         bool diff = false;
         for (unsigned n=0;n<vect.size();n++)
            if (vect[n]!=vect0[n]) diff = true;
         if (!diff) return modified(false);
      }
   }

   release();
   fKind = kind_arrstr;

   valueInt = (int64_t) vect.size();

   size_t len = 0;
   for (unsigned n=0;n<vect.size();n++)
      len+=vect[n].length()+1;
   valueStr = (char*) malloc(len);
   char* p = valueStr;

   for (unsigned n=0;n<vect.size();n++) {
      strcpy(p, vect[n].c_str());
      p+=vect[n].length()+1;
   }

   return modified();

}

bool dabc::RecordFieldNew::SetVectInt(const std::vector<int64_t>& v)
{
   int64_t* arr = 0;
   if (v.size()>0) {
      arr = new int64_t[v.size()];
      for (unsigned n=0;n<v.size();n++)
         arr[n] = v[n];
   }
   return SetArrInt(v.size(), arr, true);
}

bool dabc::RecordFieldNew::SetArrInt(int64_t size, int64_t* arr, bool owner)
{
   if (isreadonly() || (size<=0)) {
      if (owner) delete[] arr;
      return false;
   }

   if ((fKind == kind_arrint) && (size==valueInt))
      if (memcmp(arrInt, arr, size*sizeof(int64_t))==0)
      {
         if (owner) delete[] arr;
         return modified(false);
      }

   release();
   fKind = kind_arrint;
   valueInt = size;
   if (owner) {
      arrInt = arr;
   } else {
      arrInt = new int64_t[size];
      memcpy(arrInt, arr, size*sizeof(int64_t));
   }

   return modified();
}

bool dabc::RecordFieldNew::SetArrUInt(int64_t size, uint64_t* arr, bool owner)
{
   if (isreadonly()) return false;
   if (size<=0) return false;

   if ((fKind == kind_arruint) && (valueInt == size))
      if (memcmp(arrUInt, arr, size*sizeof(uint64_t))==0) {
         if (owner) delete [] arr;
         return modified(false);
      }

   release();
   fKind = kind_arruint;
   valueInt = size;
   if (owner) {
      arrUInt = arr;
   } else {
      arrUInt = new uint64_t[size];
      memcpy(arrUInt, arr, size*sizeof(uint64_t));
   }

   return modified();
}

bool dabc::RecordFieldNew::SetVectDouble(const std::vector<double>& v)
{
   double* arr = 0;
   if (v.size()>0) {
      arr = new double[v.size()];
      for (unsigned n=0;n<v.size();n++)
         arr[n] = v[n];
   }

   return SetArrDouble(v.size(), arr, true);
}


bool dabc::RecordFieldNew::SetArrDouble(int64_t size, double* arr, bool owner)
{
   if (isreadonly()) return false;
   if (size<=0) return false;

   if ((fKind == kind_arrdouble) && (valueInt == size))
      if (memcmp(arrDouble, arr, size*sizeof(double))==0) {
         if (owner) delete[] arr;
         return modified(false);
      }

   release();
   fKind = kind_arrdouble;
   valueInt = size;
   if (owner) {
      arrDouble = arr;
   } else {
      arrDouble = new double[size];
      memcpy(arrDouble, arr, size*sizeof(double));
   }

   return modified();
}

void dabc::RecordFieldNew::SetArrStrDirect(int64_t size, char* arr, bool owner)
{
   fKind = kind_arrstr;
   valueInt = size;
   if (owner) {
      valueStr = arr;
   } else {
      int fullsize = 0;
      const char* p = arr;
      for (unsigned n=0;n<size;n++) {
         int len = strlen(p);
         fullsize += len+1;
         p+=len+1;
      }
      valueStr = (char*) malloc(fullsize);
      memcpy(valueStr, arr, fullsize);
   }
}


// =========================================================================

dabc::RecordFieldsMap::RecordFieldsMap() :
   fMap()
{
}

dabc::RecordFieldsMap::~RecordFieldsMap()
{
}

bool dabc::RecordFieldsMap::HasField(const std::string& name) const
{
   return fMap.find(name) != fMap.end();
}

bool dabc::RecordFieldsMap::RemoveField(const std::string& name)
{
   FieldsMap::iterator iter = fMap.find(name);
   if (iter==fMap.end()) return false;
   fMap.erase(iter);
   return true;
}


std::string dabc::RecordFieldsMap::FieldName(unsigned n) const
{
   if (n>=fMap.size()) return "";

   for (FieldsMap::const_iterator iter = fMap.begin(); iter!=fMap.end(); iter++) {
      if (n==0) return iter->first;
      n--;
   }

   return "";
}

std::string dabc::RecordFieldsMap::FindFieldWichStarts(const std::string& name)
{
   for (FieldsMap::const_iterator iter = fMap.begin(); iter!=fMap.end(); iter++) {
      if (iter->first.length() >= name.length())
         if (iter->first.compare(0, name.length(), name)==0) return iter->first;
   }

   return "";
}



dabc::RecordFieldNew& dabc::RecordFieldsMap::Field(const std::string& name)
{
   dabc::RecordFieldNew& res = fMap[name];

   return res;
}

uint64_t dabc::RecordFieldsMap::StoreSize() const
{

   uint64_t sz = sizeof(uint32_t) + sizeof(uint32_t); // for full length, num fields and version

   for (FieldsMap::const_iterator iter = fMap.begin(); iter!=fMap.end(); iter++) {
      sz += iostream::str_storesize(iter->first);
      sz += iter->second.StoreSize();
   }

   return sz;
}

bool dabc::RecordFieldsMap::Stream(iostream& s)
{
   uint32_t storesz(0), storenum(0), storevers(0);
   uint64_t sz;

   uint64_t pos = s.size();

   if (s.is_output()) {
      sz = StoreSize();
      storesz = sz/8;
      storenum = ((uint32_t) fMap.size()) | (storevers<<24);

      s.write_uint32(storesz);
      s.write_uint32(storenum);

      for (FieldsMap::iterator iter = fMap.begin(); iter!=fMap.end(); iter++) {
         s.write_str(iter->first);
         iter->second.Stream(s);
      }

   } else {
      s.read_uint32(storesz);
      sz = ((uint64_t) storesz)*8;
      s.read_uint32(storenum);
      // storevers = storenum >> 24;
      storenum = storenum & 0xffffff;
      for (uint32_t n=0;n<storenum;n++) {
         std::string name;
         s.read_str(name);
         fMap[name].Stream(s);
      }
   }

   return s.verify_size(pos, sz);
}


bool dabc::RecordFieldsMap::SaveInXml(XMLNodePointer_t node)
{
   for (FieldsMap::const_iterator iter = fMap.begin(); iter!=fMap.end(); iter++) {

      if (iter->first.empty()) continue;

      if (iter->first[0]=='#') continue;

      std::string value = iter->second.AsStr();

      // if somebody use wrong symbol in parameter name, pack it differently
      if (iter->first.find_first_of(" #&\"\'!@%^*()=-\\/|~.,")!=iter->first.npos) {
         XMLNodePointer_t child = Xml::NewChild(node, 0, "dabc:field", 0);
         Xml::NewAttr(child, 0, "name", iter->first.c_str());
         Xml::NewAttr(child, 0, "value", value.c_str());
      } else {
         // add attribute
         Xml::NewAttr(node,0,iter->first.c_str(), value.c_str());
      }

   }
   return true;
}

bool dabc::RecordFieldsMap::ReadFromXml(XMLNodePointer_t node, bool overwrite, const ResolveFunc& func)
{
   if (node==0) return false;

   for (XMLAttrPointer_t attr = Xml::GetFirstAttr(node); attr!=0; attr = Xml::GetNextAttr(attr)) {
      const char* attrname = Xml::GetAttrName(attr);
      const char* attrvalue = Xml::GetAttrValue(attr);
      if ((attrname==0) || (attrvalue==0)) continue;

      DOUT3("Cont:%p  attribute:%s overwrite:%s", this, attrname, DBOOL(overwrite));

      if (overwrite || !HasField(attrname))
         fMap[attrname].SetStr(func.Resolve(attrvalue));
   }

   for (XMLNodePointer_t child = Xml::GetChild(node); child!=0; child = Xml::GetNext(child)) {

      if (strcmp(Xml::GetNodeName(child), "dabc:field")!=0) continue;

      const char* attrname = Xml::GetAttr(child,"name");
      const char* attrvalue = Xml::GetAttr(child,"value");
      if ((attrname==0) || (attrvalue==0)) continue;

      if (overwrite || !HasField(attrname))
        fMap[attrname].SetStr(func.Resolve(attrvalue));
   }

   return true;
}

void dabc::RecordFieldsMap::CopyFrom(const RecordFieldsMap& src, bool overwrite)
{
   for (FieldsMap::const_iterator iter = src.fMap.begin(); iter!=src.fMap.end(); iter++)
      if (overwrite || !HasField(iter->first))
         fMap[iter->first] = iter->second;
}

void dabc::RecordFieldsMap::MakeAsDiffTo(const RecordFieldsMap& current)
{
   std::vector<std::string> delfields;

   for (FieldsMap::const_iterator iter = current.fMap.begin(); iter!=current.fMap.end(); iter++) {
      if (!HasField(iter->first))
         delfields.push_back(iter->first);
      else
         if (!iter->second.fModified) RemoveField(iter->first);
   }

   // we remember fields, which should be delete when we start to reconstruct history
   if (delfields.size()>0)
      Field("dabc:del").SetStrVect(delfields);
}

// ===============================================================================



dabc::RecordContainerNew::RecordContainerNew(const std::string& name, unsigned flags) :
   Object(0, name, flags | flAutoDestroy),
   fFields(new RecordFieldsMap)
{
}

dabc::RecordContainerNew::RecordContainerNew(Reference parent, const std::string& name, unsigned flags) :
   Object(MakePair(parent, name), flags | flAutoDestroy),
   fFields(new RecordFieldsMap)
{
}

dabc::RecordContainerNew::~RecordContainerNew()
{
   delete fFields; fFields = 0;
}

dabc::RecordFieldsMap* dabc::RecordContainerNew::TakeFieldsMap()
{
   dabc::RecordFieldsMap* res = fFields;
   fFields = new RecordFieldsMap;
   return res;
}

void dabc::RecordContainerNew::SetFieldsMap(RecordFieldsMap* newmap)
{
   delete fFields;
   fFields = newmap;
   if (fFields==0) fFields = new RecordFieldsMap;
}

void dabc::RecordContainerNew::Print(int lvl)
{
   DOUT1("%s : %s", ClassName(), GetName());

   for (unsigned n=0;n<fFields->NumFields();n++) {
      std::string name = fFields->FieldName(n);
      std::string value = fFields->Field(name).AsStr();
      DOUT1("   %s = %s", name.c_str(), value.c_str());
   }
}

dabc::XMLNodePointer_t dabc::RecordContainerNew::SaveInXmlNode(XMLNodePointer_t parent)
{
   XMLNodePointer_t node = Xml::NewChild(parent, 0, GetName(), 0);

   fFields->SaveInXml(node);

   return node;
}


// =======================================================


void dabc::RecordNew::AddFieldsFrom(const RecordNew& src, bool can_owerwrite)
{
   if (!null() && !src.null())
      GetObject()->Fields().CopyFrom(src.GetObject()->Fields(), can_owerwrite);
}

std::string dabc::RecordNew::SaveToXml(bool compact)
{
   XMLNodePointer_t node = GetObject() ? GetObject()->SaveInXmlNode(0) : 0;

   std::string res;

   if (node) {
      Xml::SaveSingleNode(node, &res, compact ? 0 : 1);
      Xml::FreeNode(node);
   }

   return res;
}

bool dabc::RecordNew::ReadFromXml(const char* xmlcode)
{
   if ((xmlcode==0) || (*xmlcode==0)) return false;

   XMLNodePointer_t node = Xml::ReadSingleNode(xmlcode);

   if (node==0) return false;

   CreateContainer(Xml::GetNodeName(node));

   GetObject()->Fields().ReadFromXml(node, true);

   Xml::FreeNode(node);

   return true;
}
