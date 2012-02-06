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

int dabc::RecordField::AsInt(int dflt) const
{
   const char* val = Get();
   if (val==0) return dflt;
   int res(0);
   if (dabc::str_to_int(val, &res)) return res;
   return dflt;
}

double dabc::RecordField::AsDouble(double dflt) const
{
   const char* val = Get();
   if (val==0) return dflt;
   double res = 0.;
   if (dabc::str_to_double(val, &res)) return res;
   return dflt;
}

bool dabc::RecordField::AsBool(bool dflt) const
{
   const char* val = Get();
   if (val==0) return dflt;

   if (strcmp(val, xmlTrueValue)==0) return true; else
   if (strcmp(val, xmlFalseValue)==0) return false;

   return dflt;

}

unsigned dabc::RecordField::AsUInt(unsigned dflt) const
{
   const char* val = Get();
   if (val==0) return dflt;

   unsigned res = 0;
   if (dabc::str_to_uint(val, &res)) return res;
   return dflt;
}

bool dabc::RecordField::SetStr(const char* value)
{
   return Set(value, kind_str());
}


bool dabc::RecordField::SetStr(const std::string& value)
{
   return Set(value.c_str(), kind_str());
}

bool dabc::RecordField::SetInt(int v)
{
   char buf[100];
   sprintf(buf,"%d",v);
   return Set(buf, kind_int());
}

bool dabc::RecordField::SetDouble(double v)
{
   char buf[100];
   sprintf(buf,"%g",v);
   return Set(buf, kind_double());
}

bool dabc::RecordField::SetBool(bool v)
{
   return Set(v ? xmlTrueValue : xmlFalseValue, kind_bool());
}

bool dabc::RecordField::SetUInt(unsigned v)
{
   char buf[100];
   sprintf(buf, "%u", v);
   return Set(buf, kind_unsigned());
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


dabc::RecordContainer::RecordContainer(const std::string& name) :
   Object(0, name.c_str()),
   fPars(new RecordContainerMap)
{
   SetFlag(flAutoDestroy, true);
}

dabc::RecordContainer::RecordContainer(Reference parent, const std::string& name) :
   Object(MakePair(parent, name.c_str())),
   fPars(new RecordContainerMap)
{
   SetFlag(flAutoDestroy, true);
}

dabc::RecordContainer::~RecordContainer()
{
   delete fPars; fPars = 0;
}

void dabc::RecordContainer::Print(int lvl)
{
   DOUT1(("%s : %s", ClassName(), GetName()));

   RecordContainerMap::const_iterator iter = fPars->begin();

   while (iter!=fPars->end()) {

      DOUT1(("   %s = %s", iter->first.c_str(), iter->second.c_str()));

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

bool dabc::RecordContainer::ReadFieldsFromNode(XMLNodePointer_t node, bool overwrite)
{
   XMLAttrPointer_t attr = Xml::GetFirstAttr(node);

   while (attr!=0) {
      const char* attrname = Xml::GetAttrName(attr);

      DOUT3(("Cont:%p  attribue:%s overwrite:%s", this, attrname, DBOOL(overwrite)));

      // TODO: do we really should use RecordContainer::GetField call here ???

      if (overwrite || (RecordContainer::GetField(attrname)==0)) {
         const char* vattr = Xml::GetAttrValue(attr);

         DOUT3(("Cont:%p  attribue:%s value:%s", this, attrname, (vattr ? vattr : "---")));

         if ((vattr!=0) && (strstr(vattr,"${")!=0))
            SetField(attrname, dabc::mgr()->cfg()->ResolveEnv(vattr).c_str(), 0);
         else
            SetField(attrname, vattr, 0);
      }

      attr = Xml::GetNextAttr(attr);
   }

   XMLNodePointer_t child = Xml::GetChild(node);

   while (child!=0) {

      const char* vname = Xml::GetNodeName(child);
      const char* vattr = Xml::GetAttr(child,"value");

      if (vname && vattr) {
         std::string field_name;

         if (strcmp(vname,"_field")!=0)
            field_name = std::string("_")+vname;
         else
            field_name = Xml::GetAttr(child,"name");

         // TODO: do we really should use RecordContainer::GetField call here ???

         if (overwrite || (RecordContainer::GetField(field_name)==0)) {
            if ((vattr!=0) && (strstr(vattr,"${")!=0))
               SetField(field_name, dabc::mgr()->cfg()->ResolveEnv(vattr).c_str(), 0);
            else
               SetField(field_name, vattr, 0);
         }
      }

      child = Xml::GetNext(child);
   }
   return true;
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

//-------------------------------------------------------------------------

bool dabc::Record::CreateContainer(const std::string& name)
{
   SetObject(new dabc::RecordContainer(name), false);
   SetTransient(false);
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
   if (null()) return std::string();

   XMLNodePointer_t node = Xml::NewChild(0,0,GetName(),0);

   for(RecordContainerMap::const_iterator iter = GetObject()->fPars->begin();
         iter!=GetObject()->fPars->end();iter++) {

      if (iter->first.empty()) continue;

      if (iter->first[0]=='#') continue;

      // if somebody use wrong symbol in parameter name, pack it differently
      if (iter->first.find_first_of(" #&\"\'!@%^*()=-\\/|~.,")!=iter->first.npos) {
         XMLNodePointer_t child = Xml::NewChild(node,0,"_field",0);
         Xml::NewAttr(child, 0, "name", iter->first.c_str());
         Xml::NewAttr(child, 0, "value", iter->second.c_str());
      } else
      if (iter->first[0]!='_') {
         // add attribute
         Xml::NewAttr(node,0,iter->first.c_str(),iter->second.c_str());
      } else {
         // add child node
         XMLNodePointer_t child = Xml::NewChild(node,0,iter->first.c_str()+1,0);
         Xml::NewAttr(child, 0, "value", iter->second.c_str());
      }
   }

   std::string res;

   Xml::SaveSingleNode(node, &res, compact ? 0 : 1);
   Xml::FreeNode(node);

   return res;
}

bool dabc::Record::ReadFromXml(const std::string& v)
{
   if (v.empty()) return false;

   XMLNodePointer_t node = Xml::ReadSingleNode(v.c_str());

   if (node==0) return false;

   CreateContainer(Xml::GetNodeName(node));

   GetObject()->ReadFieldsFromNode(node, true);

   Xml::FreeNode(node);

   return true;
}
