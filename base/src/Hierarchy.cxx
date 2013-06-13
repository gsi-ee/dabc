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

#include "dabc/Hierarchy.h"

#include <string.h>

#include "dabc/Iterator.h"
#include "dabc/logging.h"
#include "dabc/Exception.h"

dabc::HierarchyContainer::HierarchyContainer(const std::string& name) :
   dabc::RecordContainer(name),
   fNodeVersion(0),
   fHierarchyVersion(0),
   fNodeChanged(false),
   fHierarchyChanged(false)
{
   SetOwner(true);
}

dabc::HierarchyContainer* dabc::HierarchyContainer::TopParent()
{
   dabc::HierarchyContainer* parent = this;

   while (parent!=0) {
      dabc::HierarchyContainer* top = dynamic_cast<dabc::HierarchyContainer*> (parent->GetParent());
      if (top==0) return parent;
      parent = top;
   }

   EOUT("Should never happen");
   return this;
}

bool dabc::HierarchyContainer::SetField(const std::string& name, const char* value, const char* kind)
{
   return dabc::RecordContainer::SetField(name, value, kind);
}

dabc::XMLNodePointer_t dabc::HierarchyContainer::SaveHierarchyInXmlNode(XMLNodePointer_t parentnode, uint64_t version)
{
   XMLNodePointer_t objnode = SaveInXmlNode(parentnode, WasNodeModifiedAfter(version));

   // provide information about node
   // sometime node just shows that it is there
   // provide mask attribute only when it is not 3

   unsigned mask = ModifiedMask(version);
   if (mask!=maskDefaultValue) Xml::NewAttr(objnode, 0, "dabc:mask", dabc::format("%u", mask).c_str());

   if (WasHierarchyModifiedAfter(version))
      for (unsigned n=0;n<NumChilds();n++) {
         dabc::HierarchyContainer* child = dynamic_cast<dabc::HierarchyContainer*> (GetChild(n));
         if (child) child->SaveHierarchyInXmlNode(objnode, version);
      }

   return objnode;
}

void dabc::HierarchyContainer::SetVersion(uint64_t version, bool recursive, bool force)
{
   if (force || fNodeChanged) fNodeVersion = version;
   if (force || fHierarchyChanged) fHierarchyVersion = version;
   if (recursive)
      for (unsigned n=0;n<NumChilds();n++) {
         dabc::HierarchyContainer* child = dynamic_cast<dabc::HierarchyContainer*> (GetChild(n));
         if (child) child->SetVersion(version, recursive, force);
      }
}

bool dabc::HierarchyContainer::UpdateHierarchyFromObject(Object* obj)
{
   fNodeChanged = false;
   fHierarchyChanged = false;

   if (obj==0) throw dabc::Exception(ex_Hierarchy, "empty obj", ItemName());

   if (!IsName(obj->GetName())) throw dabc::Exception(ex_Hierarchy, "mismatch between object and hierarchy itme", ItemName());

   // we need to recognize if any attribute disappear or changed

   dabc::RecordContainerMap* fields = TakeContainer();

   obj->SaveAttr(this);

   if (!CompareFields(fields)) fNodeChanged = true;

   DeleteContainer(fields);

   // now we should check if any childs was changed

   unsigned cnt1(0); // counter over object childs
   unsigned cnt2(0); // counter over existing childs

   while ((cnt1 < obj->NumChilds()) || (cnt2 < NumChilds())) {
      if (cnt1 >= obj->NumChilds()) {
         DeleteChild(cnt2);
         fHierarchyChanged = true;
         continue;
      }

      Reference obj_child = obj->GetChildRef(cnt1);

      bool findchild(false);
      unsigned findindx(0);

      for (unsigned n=cnt2;n<NumChilds();n++)
         if (GetChild(n)->IsName(obj_child.GetName())) {
            findchild = true;
            findindx = n;
            break;
         }

      dabc::HierarchyContainer* child = 0;

      if (findchild) {
         // delete all child with non-matching names
         while (findindx > cnt2) { DeleteChild(cnt2); findindx--; fHierarchyChanged = true; }
         child = dynamic_cast<dabc::HierarchyContainer*> (GetChild(cnt2));
      } else {
         child = new dabc::HierarchyContainer(obj_child.GetName());
         AddChildAt(child, cnt2);
         fHierarchyChanged = true;
      }

      if ((child==0) || !child->IsName(obj_child.GetName())) {
         throw dabc::Exception(ex_Hierarchy, "mismatch between object and hierarchy item", ItemName());
         return false;
      }

      if (child->UpdateHierarchyFromObject(obj_child())) fHierarchyChanged = true;

      cnt1++;
      cnt2++;
   }

   if (cnt1!=cnt2) {
      throw dabc::Exception(ex_Hierarchy, "internal failure - counters mismatch", ItemName());
      return false;
   }

   return fHierarchyChanged || fNodeChanged;
}



bool dabc::HierarchyContainer::UpdateHierarchyFromXmlNode(XMLNodePointer_t objnode)
{
   if (!IsName(Xml::GetNodeName(objnode))) return false;

   unsigned mask = maskDefaultValue;
   if (Xml::HasAttr(objnode,"dabc:mask")) {
      mask = (unsigned) Xml::GetIntAttr(objnode, "dabc:mask");
      Xml::FreeAttr(objnode, "dabc:mask");
   }

   fNodeChanged = (mask & maskNodeChanged) != 0;
   fHierarchyChanged = (mask & maskChildsChanged) != 0;

   if (fNodeChanged) {
      ClearFields();
      if (!ReadFieldsFromNode(objnode, true)) return false;
   }

   if (!fHierarchyChanged) return true;

   XMLNodePointer_t childnode = Xml::GetChild(objnode);
   unsigned cnt = 0;

   while ((childnode!=0) || (cnt<NumChilds())) {

      if (childnode==0) {
         // special case at the end - one should delete childs at the end
         DeleteChild(cnt);
         continue;
      }

      const char* childnodename = Xml::GetNodeName(childnode);
      if (strcmp(childnodename, "dabc:field") == 0) {
         Xml::ShiftToNext(childnode);
         continue;
      }

      bool findchild(false);
      unsigned findindx(0);

      for (unsigned n=cnt;n<NumChilds();n++)
         if (GetChild(n)->IsName(childnodename)) {
            findchild = true;
            findindx = n;
            break;
         }

      dabc::HierarchyContainer* child = 0;

      if (findchild) {
         // delete all child with non-matching names
         while (findindx > cnt) { DeleteChild(cnt); findindx--; }
         child = dynamic_cast<dabc::HierarchyContainer*> (GetChild(cnt));
      } else {
         child = new dabc::HierarchyContainer(childnodename);
         AddChildAt(child, cnt);
      }

      if ((child==0) || !child->IsName(childnodename)) {
         EOUT("Internal error - did not create or found child for node %s", childnodename);
         return false;
      }

      if (!child->UpdateHierarchyFromXmlNode(childnode)) return false;

      Xml::ShiftToNext(childnode);
      cnt++;
   }

   return true;
}

void dabc::HierarchyContainer::SaveToHtml(std::string& sbuf, int kind, int level)
{
   bool compact = level==0;
   const char* nl = compact ? "" : "\n";

   if (NumChilds()==0) {
      if (kind == Hierarchy::kind_Html)
         sbuf += dabc::format("%*s<li>%s</li>%s", level*3, "", GetName(), nl);
      else
         sbuf += dabc::format("%*s{\"text\": \"%s\"}", level*3, "", GetName());
      return;
   }

   if (kind == Hierarchy::kind_Html) {

      sbuf += dabc::format("%*s<li>%s", level*3, "", nl);
      if (!compact) level++;

      sbuf += dabc::format("%*s<span>%s</span>%s", level*3, "", GetName(), nl);
      sbuf += dabc::format("%*s<ul>%s", level*3, "", nl);
   } else {
      sbuf += dabc::format("%*s{\"text\": \"%s\",%s", level*3, "", GetName(), nl);
      sbuf += dabc::format("%*s \"children\": [%s", level*3, "", nl);
      if (!compact) level++;
   }

   bool isfirst = true;
   for (unsigned n=0;n<NumChilds();n++) {
      dabc::HierarchyContainer* child = dynamic_cast<dabc::HierarchyContainer*> (GetChild(n));

      if (child==0) continue;

      if ((kind == Hierarchy::kind_TxtList) && !isfirst)
         sbuf += dabc::format(",%s", nl);

      child->SaveToHtml(sbuf, kind, compact ? 0 : level+1);
      isfirst = false;
   }

   if (kind == Hierarchy::kind_Html) {
      sbuf += dabc::format("%*s</ul>%s", level*3, "", nl);
      if (!compact) level--;
      sbuf += dabc::format("%*s</li>%s", level*3, "", nl);
   } else {
      sbuf+= dabc::format("%s%*s]%s", nl, level*3, "", nl);
      if (!compact) level--;
      sbuf+= dabc::format("%*s}", level*3, "");
   }
}


// __________________________________________________-


void dabc::Hierarchy::MakeHierarchy(Reference top)
{
   Destroy();

   UpdateHierarchy(top);
}

bool dabc::Hierarchy::UpdateHierarchy(Reference top)
{
   if (top.null()) {
      Destroy();
      return true;
   }

   if (null()) {
      SetObject(new HierarchyContainer(top.GetName()));
      SetOwner(true);
      SetTransient(false);
   }
   if (GetObject()->UpdateHierarchyFromObject(top())) {

      uint64_t next_ver = GetObject()->GetVersion() + 1;

      GetObject()->SetVersion(next_ver, true, false);
   }

   return true;
}



std::string dabc::Hierarchy::SaveToXml(bool compact, uint64_t version)
{
   Iterator iter2(Ref());

   XMLNodePointer_t topnode = GetObject()->SaveHierarchyInXmlNode(0, version);

   Xml::NewAttr(topnode, 0, "dabc:version", dabc::format("%lu", (long unsigned) GetVersion()).c_str());

   std::string res;

   if (topnode) {
      Xml::SaveSingleNode(topnode, &res, compact ? 0 : 1);
      Xml::FreeNode(topnode);
   }

   return res;
}


bool dabc::Hierarchy::UpdateFromXml(const std::string& src)
{
   if (src.empty()) return false;

   XMLNodePointer_t topnode = Xml::ReadSingleNode(src.c_str());

   if (topnode==0) return false;

   bool res = true;
   long unsigned version(0);

   if (!Xml::HasAttr(topnode,"dabc:version") || !dabc::str_to_luint(Xml::GetAttr(topnode,"dabc:version"), &version)) {
      res = false;
      EOUT("Not found topnode version");
   } else {
      DOUT0("Hierarchy version is %lu", version);
      Xml::FreeAttr(topnode, "dabc:version");
   }

   if (res) {

      if (null()) {
         SetObject(new HierarchyContainer(Xml::GetNodeName(topnode)));
         SetOwner(true);
         SetTransient(false);
      }

      if (!GetObject()->UpdateHierarchyFromXmlNode(topnode)) {
         EOUT("Fail to update hierarchy from xml");
         res = false;
      } else {
         GetObject()->SetVersion(version, true, false);
      }
   }

   Xml::FreeNode(topnode);

   return res;
}

std::string dabc::Hierarchy::SaveToHtml(int kind, bool compact)
{
   if (null()) return "";

   std::string res;

   if (kind==kind_TxtList) res.append(compact ? "[" : "[\n");

   GetObject()->SaveToHtml(res, kind, compact ? 0 : 1);

   if (kind==kind_TxtList) res.append(compact ? "]" : "\n]\n");

   return res;
}
