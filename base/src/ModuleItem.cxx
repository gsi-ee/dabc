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

std::string dabc::ModuleItem::GetCfgStr(const char* name, const std::string& dfltvalue, Command* cmd)
{
   std::string v = GetModule()->GetParStr(name, dfltvalue);
   v = GetParStr(name, v);
   if (cmd) v = cmd->GetStr(name, v.c_str());
   if (FindPar(name)==0) {
      Parameter* par = CreateParStr(name, v.c_str());
      if (par) par->SetFixed(true);
   }
   return v;
}

double dabc::ModuleItem::GetCfgDouble(const char* name, double dfltvalue, Command* cmd)
{
   dfltvalue = GetModule()->GetParDouble(name, dfltvalue);
   dfltvalue = GetParDouble(name, dfltvalue);
   if (cmd) dfltvalue = cmd->GetDouble(name, dfltvalue);
   if (FindPar(name)==0) {
      Parameter* par = CreateParDouble(name, dfltvalue);
      if (par) par->SetFixed(true);
   }
   return dfltvalue;
}

int dabc::ModuleItem::GetCfgInt(const char* name, int dfltvalue, Command* cmd)
{
   dfltvalue = GetModule()->GetParInt(name, dfltvalue);
   dfltvalue = GetParInt(name, dfltvalue);
   if (cmd) dfltvalue = cmd->GetInt(name, dfltvalue);
   if (FindPar(name)==0) {
      Parameter* par = CreateParInt(name, dfltvalue);
      if (par) par->SetFixed(true);
   }
   return dfltvalue;
}

bool dabc::ModuleItem::GetCfgBool(const char* name, bool dfltvalue, Command* cmd)
{
   dfltvalue = GetModule()->GetParBool(name, dfltvalue);
   dfltvalue = GetParBool(name, dfltvalue);
   if (cmd) dfltvalue = cmd->GetBool(name, dfltvalue);
   if (FindPar(name)==0) {
      Parameter* par = CreateParBool(name, dfltvalue);
      if (par) par->SetFixed(true);
   }
   return dfltvalue;
}
