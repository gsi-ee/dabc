#include "bnet/ClusterPlugin.h"
#include "dabc/Manager.h"

extern "C" void InitUserPlugin(dabc::Manager* mgr)
{
   bnet::ClusterPlugin* plugin = new bnet::ClusterPlugin(mgr);
   
   plugin->SetPars(false, 2*1024, 128*1024, 16);
}
