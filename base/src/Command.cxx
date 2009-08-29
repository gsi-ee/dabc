/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009-
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/
#include "dabc/Command.h"

#include <map>

#include "dabc/CommandClient.h"
#include "dabc/logging.h"
#include "dabc/threads.h"

class dabc::Command::CommandParametersList : public std::map<std::string, std::string> {};


dabc::Command::Command(const char* name) :
   Basic(0, name),
   fParams(0),
   fClient(0),
   fClientMutex(0),
   fKeepAlive(false),
   fCanceled(false)
{
   DOUT5(("New command %p name %s", this, GetName()));
}

dabc::Command::~Command()
{
   // this need to be sure, that command is not attached to the client
   CleanClient();

   delete fParams;
   fParams = 0;

   DOUT5(("Delete command %p name %s", this, GetName()));
}

void dabc::Command::_CleanClient()
{
   if (fClient) fClient->_Forget(this);
   fClient = 0;
   fClientMutex = 0;
}


void dabc::Command::CleanClient()
{
   LockGuard lock(fClientMutex);
   _CleanClient();
}

bool dabc::Command::IsClient()
{
   LockGuard lock(fClientMutex);
   return fClient!=0;
}


void dabc::Command::SetPar(const char* name, const char* value)
{
   if (value==0)
      RemovePar(name);
   else {
      if (fParams==0) fParams = new CommandParametersList;
      (*fParams)[name] = value;
   }
}

const char* dabc::Command::GetPar(const char* name) const
{
   if (fParams==0) return 0;
   CommandParametersList::const_iterator iter = fParams->find(name);
   if (iter==fParams->end()) return 0;
   return iter->second.c_str();
}

bool dabc::Command::HasPar(const char* name) const
{
   if (fParams==0) return false;

   return fParams->find(name) != fParams->end();

}

void dabc::Command::RemovePar(const char* name)
{
   if (fParams==0) return;
   CommandParametersList::iterator iter = fParams->find(name);
   if (iter!=fParams->end())
     fParams->erase(iter);
}

void dabc::Command::SetStr(const char* name, const char* value)
{
   SetPar(name, value);
}

const char* dabc::Command::GetStr(const char* name, const char* deflt) const
{
   const char* val = GetPar(name);
   return val ? val : deflt;
}

void dabc::Command::SetBool(const char* name, bool v)
{
   SetPar(name, v ? xmlTrueValue : xmlFalseValue);
}

bool dabc::Command::GetBool(const char* name, bool deflt) const
{
   const char* val = GetPar(name);
   if (val==0) return deflt;

   if (strcmp(val, xmlTrueValue)==0) return true; else
   if (strcmp(val, xmlFalseValue)==0) return false;

   return deflt;
}

void dabc::Command::SetInt(const char* name, int v)
{
   char buf[100];
   sprintf(buf,"%d",v);
   SetPar(name, buf);
}

int dabc::Command::GetInt(const char* name, int deflt) const
{
   const char* val = GetPar(name);
   if (val==0) return deflt;
   int res = 0;
   if (sscanf(val, "%d", &res) != 1) return deflt;
   return res;
}

void dabc::Command::SetDouble(const char* name, double v)
{
   char buf[100];
   sprintf(buf,"%lf",v);
   SetPar(name, buf);
}

double dabc::Command::GetDouble(const char* name, double deflt) const
{
   const char* val = GetPar(name);
   if (val==0) return deflt;
   double res = 0.;
   if (sscanf(val, "%lf", &res) != 1) return deflt;
   return res;
}

void dabc::Command::SetUInt(const char* name, unsigned v)
{
   char buf[100];
   sprintf(buf, "%u", v);
   SetPar(name, buf);
}

unsigned dabc::Command::GetUInt(const char* name, unsigned deflt) const
{
   const char* val = GetPar(name);
   if (val==0) return deflt;

   unsigned res = 0;
   if (sscanf(val, "%u", &res) != 1) return deflt;
   return res;
}

void dabc::Command::SetPtr(const char* name, void* p)
{
   char buf[100];
   sprintf(buf,"%p",p);
   SetPar(name, buf);
}

void* dabc::Command::GetPtr(const char* name, void* deflt) const
{
   const char* val = GetPar(name);
   if (val==0) return deflt;

   void* p = 0;
   int res = sscanf(val,"%p", &p);
   return res>0 ? p : deflt;
}

void dabc::Command::AddValuesFrom(const dabc::Command* cmd, bool canoverwrite)
{
   if ((cmd==0) || (cmd->fParams==0)) return;

   CommandParametersList::const_iterator iter = cmd->fParams->begin();

   while (iter!=cmd->fParams->end()) {
      const char* parname = iter->first.c_str();

      if (canoverwrite || !HasPar(parname))
         SetPar(parname, iter->second.c_str());
      iter++;
   }
}

void dabc::Command::SaveToString(std::string& v)
{
   v.clear();

   v = "Command";
   v+=": ";
   v+=GetName();
   v+=";";

   if (fParams==0) return;

   CommandParametersList::iterator iter = fParams->begin();

   for (;iter!=fParams->end();iter++) {

      // exclude streaming of parameters with symbol # in the name
      if (iter->first.find_first_of("#")!=std::string::npos) continue;

      v+=" ";
      v+=iter->first;
      v+=": ";
      if (iter->second.find_first_of(";:#$")==std::string::npos)
         v+=iter->second;
      else {
         v+="'";
         v+=iter->second;
         v+="'";
//         DOUT1(("!!!!!!!!!!!!!!!!!!! SPECIAL SYNTAX par = %s val = %s", iter->first.c_str(), iter->second.c_str()));
      }

      v+=";";
   }
}

bool dabc::Command::ReadFromString(const char* s, bool onlyparameters)
{
   // onlyparameters = true indicates, that only parameters values must be present
   // in the string, command name should not be in the string at all

   delete fParams; fParams = 0;

   if ((s==0) || (*s==0)) {
      EOUT(("Empty command"));
      return false;
   }

   const char myquote = '\'';

   const char* curr = s;

   int cnt = onlyparameters ? 1 : 0;

   while (*curr != 0) {
      const char* separ = strchr(curr, ':');

      if (separ==0) {
         EOUT(("Command format error 1: %s",s));
         return false;
      }

      std::string name(curr, separ-curr);

      if (cnt==0)
        if (name.compare("Command")!=0) {
          EOUT(("Wrong syntax, starts with: %s, expects: Command", name.c_str()));
          return false;
        }

      curr = separ+1;
      while (*curr == ' ') curr++;
      if (curr==0) {
         EOUT(("Command format error 4: %s",s));
         return false;
      }

      std::string val;

      if (*curr==myquote) {
         curr++;
         separ = strchr(curr, myquote);
         if ((separ==0) || (*(separ+1) != ';')) {
            EOUT(("Command format error 2: %s",s));
            return false;
         }

         val.assign(curr, separ-curr);
         separ++;

      } else {
         separ = strchr(curr, ';');
         if (separ==0) {
            EOUT(("Command format error 3: %s",s));
            return false;
         }
         val.assign(curr, separ-curr);
      }

      if (cnt==0)
         SetName(val.c_str());
      else
         SetPar(name.c_str(), val.c_str());

      cnt++;

      curr = separ+1;
      while (*curr==' ') curr++;
   }

   return true;
}

bool dabc::Command::ReadParsFromDimString(const char* pars)
{
   if ((pars==0) || (*pars==0)) return true;

   const char* curr = pars;

   while (*curr != 0) {
      while (*curr==' ') curr++;
      if (*curr == 0) return true;

      const char* separ = strstr(curr, "=\"");
      if (separ==0) {
         EOUT(("Didnot found =\" sequence"));
         return false;
      }

      const char* separ2 = strchr(separ+2, '\"');
      if (separ2==0) {
         EOUT(("Didnot found closing \" symbol"));
         return false;
      }

      std::string name(curr, separ-curr);
      std::string value(separ + 2, separ2 - separ - 2);

      DOUT1(("Set dim argument %s = %s", name.c_str(), value.c_str()));

      SetPar(name.c_str(), value.c_str());
      curr = separ2 + 1;
   }

   return true;
}


void dabc::Command::Reply(Command* cmd, bool res)
{
   if (cmd==0) return;
   cmd->SetResult(res);

   bool proseccreply = false;

   {
      DOUT5(("Cmd %p Client mutex locked %s", cmd, DBOOL((cmd->fClientMutex ? cmd->fClientMutex->IsLocked() : false))));

      LockGuard guard(cmd->fClientMutex);

      CommandClientBase* client = cmd->fClient;

      DOUT5(("Reply command %p %s Client %p", cmd, cmd->GetName(), client));

      if (client) {
         client->_Forget(cmd);
         proseccreply = client->_ProcessReply(cmd);
      }

      DOUT5(("Reply command %p done process %s", cmd, DBOOL(proseccreply)));
   }

   if (!proseccreply) Finalise(cmd);
}

void dabc::Command::Finalise(Command* cmd)
{
   if (cmd==0) return;

   // this is a way to keep object "alive" - means not destroy it
   // second call, which should be done from the user, must finally delete it

   if (cmd->IsKeepAlive()) {
      cmd->CleanClient();
      cmd->SetKeepAlive(false);
   }
      else delete cmd;
}
