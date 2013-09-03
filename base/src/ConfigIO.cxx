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
#include "dabc/Object.h"
#include "dabc/Manager.h"
#include "dabc/Record.h"

#include "dabc/string.h"
#include "dabc/logging.h"

#include <fnmatch.h>
#include <string.h>

dabc::ConfigIO::ConfigIO(Configuration* cfg) :
   fCfg(cfg),
   fCurrItem(0),
   fCurrChld(0),
   fCurrStrict(true)
{
}

dabc::ConfigIO::ConfigIO(const ConfigIO& src) :
   fCfg(src.fCfg),
   fCurrItem(src.fCurrItem),
   fCurrChld(src.fCurrChld),
   fCurrStrict(src.fCurrStrict)
{
}

bool dabc::ConfigIO::FindItem(const char* name)
{
   if (fCurrItem==0) return false;

   if (fCurrChld==0)
      fCurrChld = Xml::GetChild(fCurrItem);
   else
      fCurrChld = Xml::GetNext(fCurrChld);

   while (fCurrChld!=0) {
      if (fCfg->IsNodeName(fCurrChld, name)) {
         fCurrItem = fCurrChld;
         fCurrChld = 0;
         return true;
      }
      fCurrChld = Xml::GetNext(fCurrChld);
   }

   return false;
}

bool dabc::ConfigIO::CheckAttr(const char* name, const char* value)
{
   // make extra check - if fCurrChld!=0 something was wrong already
   if ((fCurrChld!=0) || (fCurrItem==0)) return false;

   bool res = true;

   if (value==0) {
      res = Xml::HasAttr(fCurrItem, name);
   } else {
      const char* attr = Xml::GetAttr(fCurrItem, name);

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

   if ((obj!=0) && (obj->GetParent()==0) && (lvl==1) && (obj!=dabc::mgr())) return dabc::mgr();

   while ((obj!=0) && (lvl-->0)) obj = obj->GetParent();
   return obj;
}

dabc::XMLNodePointer_t dabc::ConfigIO::FindSubItem(XMLNodePointer_t node, const char* name)
{
   if ((node==0) || (name==0) || (strlen(name)==0)) return node;

   const char* pos = strchr(name, '/');
   if (pos==0) return fCfg->FindChild(node, name);

   std::string subname(name, pos-name);
   return FindSubItem(fCfg->FindChild(node, subname.c_str()), pos+1);
}

/** \brief Special class to resolve configuration variables at the time when field value set */

namespace dabc {

   class ConfigIOResolveNew : public RecordFieldsMap::ResolveFunc {
      protected:
         Configuration* fCfg;
         std::string fBuf;

      public:
         ConfigIOResolveNew(Configuration* cfg) : RecordFieldsMap::ResolveFunc(), fCfg(cfg) {}

         virtual ~ConfigIOResolveNew() {}

         virtual const char* Resolve(const char* arg) const
         {
            if ((fCfg==0) || (arg==0) || (strstr(arg,"${")==0)) return arg;
            ConfigIOResolveNew* me = const_cast<ConfigIOResolveNew*> (this);
            me->fBuf = me->fCfg->ResolveEnv(arg);
            return fBuf.c_str();
         }
   };
}

bool dabc::ConfigIO::ReadRecordField(Object* obj, const std::string& itemname, RecordField* field, RecordFieldsMap* fieldsmap)
{
   // here we search all nodes in config file which are compatible with for specified object
   // and config name. From all places we reconstruct all fields and attributes which can belong
   // to the record

   if ((fCfg==0) || (obj==0)) return false;

   int maxlevel = 0;
   Object* prnt = 0;
   while ((prnt = GetObjParent(obj, maxlevel)) != 0) {
      if (prnt==dabc::mgr()) break;
      maxlevel++;
   }
   // TODO: could we read objects which are not in manager?
   if (prnt==0) return false;

   DOUT2("Start reading of obj %s item %s maxlevel %d", obj->ItemName().c_str(),  itemname.c_str(), maxlevel);

   bool isany = false;

   // DOUT0("Start read of record maxlvl %d", maxlevel);

   Reference app = dabc::mgr.GetAppFolder();

   // first loop - strict syntax in the selected context
   // second loop - wildcard syntax in selected context

   for (int dcnt=0;dcnt<2;dcnt++) {

      DOUT2("Switch to search mode %d", dcnt);

      fCurrStrict = dcnt==0;

      // only in first case we skipping context node
      int max_depth = (dcnt==0) ? maxlevel - 1 : maxlevel;

      switch (dcnt) {
         case 0: fCurrItem = fCfg->fSelected; break;
         case 1: fCurrItem = fCfg->RootNode(); break;
         default: EOUT("INTERNAL ERROR"); fCurrItem = 0; break;
      }

      fCurrChld = 0;
      int level = max_depth;
      int appskiplevel(-1);

      while (level >= 0) {
         prnt = GetObjParent(obj, level);

         DOUT2("Search with loop %d path level = %d obj = %s class %s", dcnt,  level, DNAME(prnt), (prnt ? prnt->ClassName() : "---"));

//         DOUT3("Search with level = %d prnt = %p %s", level, prnt, DNAME(prnt));

         if (prnt == 0) return false;

         // DOUT0("+++ Search parent %s class %s level %d", prnt->GetName(), prnt->ClassName(), level);

         XMLNodePointer_t curr = fCurrItem;
         //         DOUT0("Search for parent %s level %d loop = %d", prnt->GetName(), level, dcnt);

         if (prnt->Find(*this)) {
            DOUT2("Find parent of level:%d", level);
            if (level-->0) continue;

            // in case of single field (configuration) we either check attribute in node itself
            // or "value" attribute in child node in provided name

            XMLNodePointer_t itemnode = FindSubItem(fCurrItem, itemname.c_str());

            if (field!=0) {
               const char* attrvalue = 0;
               if (itemnode!=0) attrvalue = Xml::GetAttr(itemnode, "value");
               if (attrvalue==0) attrvalue = Xml::GetAttr(fCurrItem, itemname.c_str());

               if (attrvalue!=0) {
                  if ((strstr(attrvalue,"${")!=0) && fCfg)
                     field->SetStr(fCfg->ResolveEnv(attrvalue).c_str());
                  else
                     field->SetStr(attrvalue);
                  DOUT2("For item %s find value %s", itemname.c_str(), attrvalue);
                  return true;
               }
            } else
            if ((fieldsmap!=0) && (itemnode!=0)) {
               ConfigIOResolveNew resolve(fCfg);

               fieldsmap->ReadFromXml(itemnode, false, resolve);
               isany = true;
            }

            fCurrChld = 0;
         } else
         if ((curr != fCurrItem) || (fCurrChld != 0)) {
            EOUT("FIXME: should not happen");
            EOUT("FIXME: problem in hierarchy search for %s lvl %d prnt %s", obj->ItemName().c_str(), level, prnt->GetName());
            EOUT("fCurrChld %p   curr %p  fCurrItem %p", fCurrChld, curr, fCurrItem);
            break;
         } else
         if ((level>0) && (app == prnt))  {
            // if we were trying to find application and didnot find it - no problem
            // probably modules/pools/device configuration specified without application - lets try to find it

            DOUT2("Skip missing application at level %d", level);

            // remember on which level we skip application, do not try to rall back
            appskiplevel = level--;

            continue;
         }

         level++;
         if (level==appskiplevel) level++;

         if (level > max_depth) break;

         // if we skip application node, we should not try to traverse it back
         fCurrChld = fCurrItem;
         fCurrItem = Xml::GetParent(fCurrItem);

         if (fCurrItem==0) {
            EOUT("FIXME: Wrong hierarchy search level = %d maxlevel = %d chld %p item %p dcnt = %d", level, maxlevel, fCurrChld, fCurrItem, dcnt);
            break;
         }

      } // while (level >= 0) {

//      DOUT3("End of level loop");
   }

   return isany;
}

