# MBS API documentation {#mbs_api}

[MBS plugin in DABC](@ref mbs_plugin) provides C++ based API to access MBS
from user code. Two major classes are provided:

+ @ref mbs::MonitorHandle - access logging, statistic and command channels of remote MBS nodes
+ @ref mbs::ReadoutHandle - readout data from MBS servers and LMD files

Complete API available via ["mbs/api.h"](\ref mbs/api.h) include file.

## Simple MBS readout

~~~~~~~~~~~~~
#include "mbs/api.h"

int main()
{
   mbs::ReadoutHandle ref = mbs::ReadoutHandle::Connect("mbs://r4-5/Stream");
   if (ref.null()) return 1;

   mbs::EventHeader* evnt = ref.NextEvent(1.);

   if (evnt) evnt->PrintHeader();

   ref.Disconnect();
   return 0;
}
~~~~~~~~~~~~~

Example of usage such interface can be found in [mbsprint.cxx](\ref plugins/mbs/utils/mbsprint.cxx)


## Command sending to MBS

~~~~~~~~~~~~~
#include "mbs/api.h"

int main()
{
   mbs::MonitorHandle ref = mbs::MonitorHandle::Connect("r4-5");
   if (ref.null()) return 1;

   ref.MbsCmd("type event");

   ref.Disconnect();
   return 0;
}
~~~~~~~~~~~~~

Example of usage such interface can be found in [mbscmd.cxx](@ref plugins/mbs/utils/mbscmd.cxx)
