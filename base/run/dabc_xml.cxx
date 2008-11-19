#include "dabc/logging.h"
#include "dabc/ConfigBase.h"

#include <iostream>

int main(int numc, char* args[])
{
   dabc::SetDebugLevel(1);

   const char* configuration = "SetupRoc.xml";
   const char* workdir = 0;
   unsigned configid = 0;
   const char* connstr = 0;

   if(numc > 1) configuration = args[1];

   dabc::ConfigBase cfg(configuration);
   if (!cfg.IsOk()) return 7;

   int cnt = 2;
   while (cnt<numc) {

      const char* arg = args[cnt++];

      if (strcmp(arg,"-id")==0) {
         if (cnt < numc)
            configid = (unsigned) atoi(args[cnt++]);
      } else
      if (strcmp(arg,"-workdir")==0) {
         if (cnt < numc)
            workdir = args[cnt++];
      } else
      if (strcmp(arg,"-conn")==0) {
         if (cnt < numc)
            connstr = args[cnt++];
      } else
      if (strcmp(arg,"-number")==0) {
         unsigned res = cfg.NumNodes();
         if (res==0) return 5;
         std::cout << res << std::endl;
         std::cout.flush();
      } else
//      if (strstr(arg,"-gen")==arg) {
//         dabc::Configuration::ProduceClusterFile(configuration, numnodes);
//         DOUT1(("Produce cluster file %s", configuration));
//      } else
      if (strcmp(arg,"-ssh") == 0) {
         const char* kind = (cnt < numc) ? args[cnt++] : "start";
         std::string res = cfg.SshArgs(configid, kind, configuration, workdir, connstr);
         if (res.length()==0) return 7;
         std::cout << res << std::endl;
         std::cout.flush();
      } else
      if (strstr(arg,"-nodename")==arg) {
         if (configid<0) return 5;
         std::string name = cfg.NodeName(configid);
         if (name.length() == 0) return 5;
         std::cout << name << std::endl;
         std::cout.flush();
      }
   }

   return 0;
}
