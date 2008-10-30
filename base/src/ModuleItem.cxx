#include "dabc/ModuleItem.h"

dabc::ModuleItem::ModuleItem(int typ, Basic* parent, const char* name) :
   Basic(parent, name),
   WorkingProcessor(),
   fModule(0),
   fItemType(typ),
   fItemId(0)
{
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
