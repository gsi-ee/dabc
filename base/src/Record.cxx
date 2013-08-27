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
