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

#include "dabc/ConfigIO.h"

#include "dabc/Configuration.h"
#include "dabc/Manager.h"

#include <fnmatch.h>
#include <cstring>

dabc::ConfigIO::ConfigIO(Configuration* cfg, int id) :
   fCfg(cfg),
   fCurrItem(nullptr),
   fCurrChld(nullptr),
   fCurrStrict(true),
   fCgfId(id)
{
}

dabc::ConfigIO::ConfigIO(const ConfigIO& src) :
   fCfg(src.fCfg),
   fCurrItem(src.fCurrItem),
   fCurrChld(src.fCurrChld),
   fCurrStrict(src.fCurrStrict),
   fCgfId(src.fCgfId)
{
}

bool dabc::ConfigIO::FindItem(const char *name)
{
   if (!fCurrItem) return false;

   if (!fCurrChld)
      fCurrChld = Xml::GetChild(fCurrItem);
   else
      fCurrChld = Xml::GetNext(fCurrChld);

   while (fCurrChld) {
      if (fCfg->IsNodeName(fCurrChld, name)) {
         fCurrItem = fCurrChld;
         fCurrChld = nullptr;
         return true;
      }
      fCurrChld = Xml::GetNext(fCurrChld);
   }

   return false;
}

bool dabc::ConfigIO::CheckAttr(const char *name, const char *value)
{
   // make extra check - if fCurrChld != 0 something was wrong already
   if (fCurrChld || !fCurrItem) return false;

   bool res = true;

   if (!value) {
      res = Xml::HasAttr(fCurrItem, name);
   } else {
      const char *attr = Xml::GetAttr(fCurrItem, name);

      std::string sattr = attr ? fCfg->ResolveEnv(attr) : std::string("");

      if (fCurrStrict) {
         res = sattr.empty() ? false : (sattr == value);
      } else {
         if (!sattr.empty()) {
            res = (sattr == value);
            if (!res) res = fnmatch(sattr.c_str(), value, FNM_NOESCAPE) == 0;
         }
      }
   }

   // if attribute was not identified in the structure,
   // current node pointer automatically ralled-back to parent that
   // Worker::Find() method could try to find next item with probably correct attribute

   if (!res) {
      fCurrChld = fCurrItem;
      fCurrItem = Xml::GetParent(fCurrItem);
   }

   return res;
}

dabc::Object* dabc::ConfigIO::GetObjParent(Object* obj, int lvl)
{
   // make hack for the objects, which are not registered in the object manager,
   // but which are want to use config files. Make special case:

   if (obj && !obj->GetParent() && (lvl == 1) && (obj != dabc::mgr())) return dabc::mgr();

   if (lvl == 0) return obj;

   while (obj&& (lvl-- > 0))
      obj = obj->GetParent();
   return obj;
}

dabc::XMLNodePointer_t dabc::ConfigIO::FindSubItem(XMLNodePointer_t node, const char *name)
{
   if (!node || !name || (strlen(name) == 0)) return node;

   const char *pos = strchr(name, '/');
   if (!pos) return fCfg->FindChild(node, name);

   std::string subname(name, pos-name);
   return FindSubItem(fCfg->FindChild(node, subname.c_str()), pos+1);
}

std::string dabc::ConfigIO::ResolveEnv(const char *value)
{
   if (!value || (*value == 0)) return std::string();

   if ((strstr(value,"${") == nullptr) || !fCfg)
      return std::string(value);

   return fCfg->ResolveEnv(value, fCgfId);
}

bool dabc::ConfigIO::ReadRecordField(Object* obj, const std::string &itemname, RecordField* field, RecordFieldsMap* fieldsmap)
{
   // here we search all nodes in config file which are compatible with for specified object
   // and config name. From all places we reconstruct all fields and attributes which can belong
   // to the record

   if (!fCfg || !obj) return false;

   int maxlevel = 0;
   Object* prnt = nullptr;
   while ((prnt = GetObjParent(obj, maxlevel)) != nullptr) {
      if (prnt == dabc::mgr()) break;
      maxlevel++;
      // if object want to be on the top xml level, count maxlevel+1 to add manager as artificial top parent
      if (prnt->IsTopXmlLevel()) break;
   }
   // TODO: could we read objects which are not in manager?
   if (!prnt) return false;

   DOUT3("Start reading of obj %s item %s maxlevel %d", obj->ItemName().c_str(),  itemname.c_str(), maxlevel);

   bool isany = false;

   // DOUT0("Start read of record maxlvl %d", maxlevel);

   // first loop - strict syntax in the selected context
   // second loop - wildcard syntax in all contexts

   for (int dcnt=0;dcnt<2;dcnt++) {

      DOUT3("Switch to search mode %d", dcnt);

      // only in first case we skipping context node
      int max_depth = 0;
      fCurrItem = nullptr;
      fCurrStrict = true;

      switch (dcnt) {
         case 0:
            fCurrItem = fCfg->fSelected;
            max_depth = maxlevel - 1;
            fCurrStrict = true;
            break;
         case 1:
            fCurrItem = fCfg->RootNode();
            max_depth = maxlevel;
            fCurrStrict = false;
            break;
         default: EOUT("INTERNAL ERROR");  break;
      }

      fCurrChld = nullptr;
      int level = max_depth;

      while (level >= 0) {
         // maximal level is manager itself, all other is several parents (if any)
         prnt = (level == maxlevel) ? dabc::mgr() :  GetObjParent(obj, level);

         DOUT3("Search with loop %d path level = %d obj = %s class %s", dcnt,  level, DNAME(prnt), (prnt ? prnt->ClassName() : "---"));

//         DOUT3("Search with level = %d prnt = %p %s", level, prnt, DNAME(prnt));

         if (!prnt) return false;

         // DOUT0("+++ Search parent %s class %s level %d", prnt->GetName(), prnt->ClassName(), level);

         XMLNodePointer_t curr = fCurrItem;
         //         DOUT0("Search for parent %s level %d loop = %d", prnt->GetName(), level, dcnt);

         if (prnt->Find(*this)) {
            DOUT3("Find parent of level:%d", level);
            if (level-->0) continue;

            // in case of single field (configuration) we either check attribute in node itself
            // or "value" attribute in child node in provided name

            XMLNodePointer_t itemnode = FindSubItem(fCurrItem, itemname.c_str());

            if (field) {
               const char *attrvalue = nullptr;
               if (itemnode) attrvalue = Xml::GetAttr(itemnode, "value");
               if (!attrvalue) attrvalue = Xml::GetAttr(fCurrItem, itemname.c_str());

               if (attrvalue) {
                  field->SetStr(ResolveEnv(attrvalue));
                  DOUT3("For item %s find value %s", itemname.c_str(), attrvalue);
                  return true;
               } else {
                  // try to find array of items
                  std::vector<std::string> arr;

                  for (XMLNodePointer_t child = Xml::GetChild(itemnode); child != nullptr; child = Xml::GetNext(child)) {
                     if (strcmp(Xml::GetNodeName(child), "item") != 0) continue;
                     const char *arritemvalue = Xml::GetAttr(child,"value");
                     if (arritemvalue)
                        arr.emplace_back(ResolveEnv(arritemvalue));
                  }

                  if (arr.size() > 0) {
                     field->SetStrVect(arr);
                     return true;
                  }
               }
            } else
            if (fieldsmap && itemnode) {
               isany = true;

               for (XMLAttrPointer_t attr = Xml::GetFirstAttr(itemnode); attr != nullptr; attr = Xml::GetNextAttr(attr)) {
                  const char *attrname = Xml::GetAttrName(attr);
                  const char *attrvalue = Xml::GetAttrValue(attr);
                  if (!attrname || !attrvalue) continue;

                  if (!fieldsmap->HasField(attrname))
                     fieldsmap->Field(attrname).SetStr(ResolveEnv(attrvalue));
               }

               for (XMLNodePointer_t child = Xml::GetChild(itemnode); child != nullptr; child = Xml::GetNext(child)) {

                  if (strcmp(Xml::GetNodeName(child), "dabc:field") != 0) continue;

                  const char *attrname = Xml::GetAttr(child,"name");
                  const char *attrvalue = Xml::GetAttr(child,"value");
                  if (!attrname || !attrvalue) continue;

                  if (!fieldsmap->HasField(attrname))
                     fieldsmap->Field(attrname).SetStr(ResolveEnv(attrvalue));
               }
            }

            fCurrChld = nullptr;
         } else
         if ((curr != fCurrItem) || fCurrChld) {
            EOUT("FIXME: should not happen");
            EOUT("FIXME: problem in hierarchy search for %s lvl %d prnt %s", obj->ItemName().c_str(), level, prnt->GetName());
            EOUT("fCurrChld %p   curr %p  fCurrItem %p", fCurrChld, curr, fCurrItem);
            break;
         } else {
            // FIX from 13.05.2015.
            // Here was code to skip application node, now it is gone
            // break;
         }

         level++;

         if (level > max_depth) break;

         fCurrChld = fCurrItem;
         fCurrItem = Xml::GetParent(fCurrItem);

         if (!fCurrItem) {
            EOUT("FIXME: Wrong hierarchy search level = %d maxlevel = %d chld %p item %p dcnt = %d", level, maxlevel, fCurrChld, fCurrItem, dcnt);
            break;
         }

      } // while (level >= 0) {

//      DOUT3("End of level loop");
   }

   return isany;
}

