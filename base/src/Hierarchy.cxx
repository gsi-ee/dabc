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
   dabc::RecordContainer(name, flNoMutex), // disable default ownership and no mutex is required
   fNodeVersion(0),
   fHierarchyVersion(0),
   fNodeChanged(false),
   fHierarchyChanged(false)
{
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

bool dabc::HierarchyContainer::UpdateHierarchyFrom(HierarchyContainer* cont)
{
   fNodeChanged = false;
   fHierarchyChanged = false;

   if (cont==0) throw dabc::Exception(ex_Hierarchy, "empty container", ItemName());

   // we do not check names here - top object name can be different
   // if (!IsName(obj->GetName())) throw dabc::Exception(ex_Hierarchy, "mismatch between object and hierarchy itme", ItemName());

   // we need to recognize if any attribute disappear or changed

   if (!CompareFields(cont->GetFieldsMap())) {
      fNodeChanged = true;
      SetFieldsMap(cont->TakeFieldsMap());
   }

   // now we should check if any childs was changed

   unsigned cnt1(0); // counter over source container
   unsigned cnt2(0); // counter over existing childs

   while ((cnt1 < cont->NumChilds()) || (cnt2 < NumChilds())) {
      if (cnt1 >= cont->NumChilds()) {
         DeleteChild(cnt2);
         fHierarchyChanged = true;
         continue;
      }

      dabc::Hierarchy cont_child(cont->GetChild(cnt1));
      if (cont_child.null()) {
         EOUT("FAILURE");
         return false;
      }

      bool findchild(false);
      unsigned findindx(0);

      for (unsigned n=cnt2;n<NumChilds();n++)
         if (GetChild(n)->IsName(cont_child.GetName())) {
            findchild = true;
            findindx = n;
            break;
         }

      if (!findchild) {
         // if child did not found, just take it out form source container and place at proper position

         cont->RemoveChild(cont_child());

         AddChildAt(cont_child(), cnt2);

         fHierarchyChanged = true;
         cnt2++;

         continue;
      }

      while (findindx > cnt2) { DeleteChild(cnt2); findindx--; fHierarchyChanged = true; }

      dabc::HierarchyContainer* child = (dabc::HierarchyContainer*) (GetChild(cnt2));

      if ((child==0) || !child->IsName(cont_child.GetName())) {
         throw dabc::Exception(ex_Hierarchy, "mismatch between object and hierarchy item", ItemName());
         return false;
      }

      if (child->UpdateHierarchyFrom(cont_child())) fHierarchyChanged = true;

      cnt1++;
      cnt2++;
   }

   return fHierarchyChanged || fNodeChanged;
}

dabc::HierarchyContainer* dabc::HierarchyContainer::CreateChildAt(const std::string& name, int indx)
{
   while ((indx>=0) && (indx<(int) NumChilds())) {
      dabc::HierarchyContainer* child = (dabc::HierarchyContainer*) GetChild(indx);
      if (child->IsName(name.c_str())) return child;
      DeleteChild(indx);
   }

   dabc::HierarchyContainer* res = new dabc::HierarchyContainer(name);
   AddChild(res);

   return res;
}



std::string dabc::HierarchyContainer::ItemName()
{
   std::string res;
   FillFullName(res, 0, true);
   return res;
}

bool dabc::HierarchyContainer::UpdateHierarchyFromXmlNode(XMLNodePointer_t objnode)
{
   // we do not check node name - it is done when childs are selected
   // for top-level node name can differ

   // if (!IsName(Xml::GetNodeName(objnode))) return false;

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

std::string dabc::HierarchyContainer::HtmlBrowserText()
{
   if (NumChilds()>0) return GetName();

   if (!HasField("rate")) return GetName();

   return dabc::format("<a href='#' onClick='return displayObject(this);' fullname='%s'>%s</a>", ItemName().c_str(), GetName());
}

void dabc::HierarchyContainer::SaveToJSON(std::string& sbuf, int level)
{
   bool compact = level==0;
   const char* nl = compact ? "" : "\n";

   if (NumChilds()==0) {
      sbuf += dabc::format("%*s{\"text\":\"%s\"}", level*3, "", HtmlBrowserText().c_str());
      return;
   }

   sbuf += dabc::format("%*s{\"text\":\"%s\",%s", level*3, "",  HtmlBrowserText().c_str(), nl);
   sbuf += dabc::format("%*s\"children\":[%s", level*3, "", nl);
   if (!compact) level++;

   bool isfirst = true;
   for (unsigned n=0;n<NumChilds();n++) {
      dabc::HierarchyContainer* child = dynamic_cast<dabc::HierarchyContainer*> (GetChild(n));

      if (child==0) continue;

      if (!isfirst) sbuf += dabc::format(",%s", nl);

      child->SaveToJSON(sbuf, compact ? 0 : level+1);
      isfirst = false;
   }

   sbuf+= dabc::format("%s%*s]%s", nl, level*3, "", nl);
   if (!compact) level--;
   sbuf+= dabc::format("%*s}", level*3, "");
}

void dabc::HierarchyContainer::BuildHierarchy(HierarchyContainer* cont)
{
   cont->CopyFieldsMap(GetFieldsMap());

   dabc::RecordContainer::BuildHierarchy(cont);
}

// __________________________________________________-


void dabc::Hierarchy::Build(const std::string& topname, Reference top)
{
   Destroy();

   Create(topname);

   if (!top.null())

   top()->BuildHierarchy(GetObject());
}

bool dabc::Hierarchy::Update(dabc::Hierarchy& src)
{
   if (src.null()) {
      Destroy();
      return true;
   }

   if (null()) {
      EOUT("Hierarchy structure MUST exist at this moment");
      Create("DABC");
   }

   if (GetObject()->UpdateHierarchyFrom(src())) {

      uint64_t next_ver = GetObject()->GetVersion() + 1;

      GetObject()->SetVersion(next_ver, true, false);
   }

   return true;
}


bool dabc::Hierarchy::UpdateHierarchy(Reference top)
{
   if (null()) {
      Build(top.GetName(), top);
      return true;
   }

   if (top.null()) {
      EOUT("Objects structure must be provided");
      return false;
   }

   dabc::Hierarchy src;

   src.Build(top.GetName(), top);

   return Update(src);
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


void dabc::Hierarchy::Create(const std::string& name)
{
   Release();
   SetObject(new HierarchyContainer(name));
   SetOwner(true); // top level is owner
   SetTransient(false);
}

dabc::Hierarchy dabc::Hierarchy::CreateChild(const std::string& name, int indx)
{
   if (null() || name.empty()) return dabc::Hierarchy();

   return GetObject()->CreateChildAt(name, indx);

   return FindChild(name.c_str());
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

      if (null()) Create(Xml::GetNodeName(topnode));

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

std::string dabc::Hierarchy::SaveToJSON(bool compact, bool excludetop)
{
   if (null()) return "";

   std::string res;

   res.append(compact ? "[" : "[\n");

   if (excludetop) {
      bool isfirst = true;
      for (unsigned n=0;n<NumChilds();n++) {
         dabc::Hierarchy child = GetChild(n);

         if (child.null()) continue;

         if (!isfirst) res += (compact ? "," : ",\n");

         child()->SaveToJSON(res, compact ? 0 : 1);
         isfirst = false;
      }
   } else {
      GetObject()->SaveToJSON(res, compact ? 0 : 1);
   }

   res.append(compact ? "]" : "\n]\n");

   return res;
}
