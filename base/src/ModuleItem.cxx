#include "dabc/ModuleItem.h"

dabc::ModuleItem::ModuleItem(int typ, Basic* parent, const char* name) :
   Folder(parent, name),
   WorkingProcessor(),
   fModule(0),
   fItemType(typ),
   fItemId(0)
{
   SetParsHolder(this);
   SetParDflts(3);

   while ((fModule==0) && (parent!=0)) {
      fModule = dynamic_cast<Module*> (parent);
      parent = parent->GetParent();
   }

   SetProcessorPriority(1);

   if (fModule)
      fModule->ItemCreated(this);
}

dabc::ModuleItem::~ModuleItem()
{
   if (fModule)
      fModule->ItemDestroyed(this);
}

int dabc::ModuleItem::GetCfgInt(const char* name, int dfltvalue)
{
   if (FindPar(name)==0) {
      Parameter* par = CreateParInt(name, dfltvalue);
      if (par) par->SetFixed(true);
   }
   return GetParInt(name, dfltvalue);
}

bool dabc::ModuleItem::GetCfgBool(const char* name, bool dfltvalue)
{
   if (FindPar(name)==0) {
      Parameter* par = CreateParBool(name, dfltvalue);
      if (par) par->SetFixed(true);
   }
   return GetParBool(name, dfltvalue);
}
