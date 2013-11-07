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

#include "mbs/Factory.h"

#include "dabc/string.h"
#include "dabc/logging.h"
#include "dabc/Command.h"
#include "dabc/Manager.h"
#include "dabc/SocketThread.h"
#include "dabc/MemoryPool.h"
#include "dabc/Url.h"
#include "dabc/DataTransport.h"

#include "mbs/LmdInput.h"
#include "mbs/LmdOutput.h"
#include "mbs/TextInput.h"
#include "mbs/GeneratorInput.h"
#include "mbs/ServerTransport.h"
#include "mbs/ClientTransport.h"
#include "mbs/CombinerModule.h"
#include "mbs/Player.h"

dabc::FactoryPlugin mbsfactory(new mbs::Factory("mbs"));


dabc::Transport* mbs::Factory::CreateTransport(const dabc::Reference& port, const std::string& typ, dabc::Command cmd)
{
   dabc::Url url(typ);

   dabc::PortRef portref = port;

   if (portref.IsInput() && (url.GetProtocol()==mbs::protocolMbs) && !url.GetHostName().empty()) {

      DOUT0("Try to create new MBS input transport typ %s file %s", typ.c_str(), url.GetFileName().c_str());

      int kind = mbs::NoServer;
      int portnum = 0;

      if (!url.GetFileName().empty())
         kind = StrToServerKind(url.GetFileName().c_str());

      if (url.GetPort()>0)
         portnum = url.GetPort();

      if (kind == mbs::NoServer) {
         if (portnum==DefualtServerPort(mbs::TransportServer)) kind = mbs::TransportServer; else
         if (portnum==DefualtServerPort(mbs::StreamServer)) kind = mbs::StreamServer;
      }

      if (portnum==0) portnum = DefualtServerPort(kind);

      if ((kind == mbs::NoServer) || (portnum==0)) {
         EOUT("MBS server in url %s not specified correctly", typ.c_str());
         return 0;
      }

      dabc::TimeStamp tm(dabc::Now());
      bool firsttime = true;

      // TODO: configure timeout via url parameter
      // TODO: make connection via special addon (not blocking thread here)
      double tmout(3.);

      int fd(-1);

      do {

         if (firsttime) firsttime = false;
                   else dabc::mgr.Sleep(0.01);

         fd = dabc::SocketThread::StartClient(url.GetHostName().c_str(), portnum);
         if (fd>0) break;

      } while (!tm.Expired(tmout));

      if (fd<=0) {
         DOUT0("Fail connecting to host:%s port:%d", url.GetHostName().c_str(), portnum);
         return 0;
      }

      DOUT0("Try to establish connection with host:%s kind:%s port:%d", url.GetHostName().c_str(), mbs::ServerKindToStr(kind), portnum);

      ClientTransport* tr = new ClientTransport(fd, kind);

      return new dabc::InputTransport(cmd, portref, tr, false, tr);
   }


   if (portref.IsOutput() && (url.GetProtocol()==mbs::protocolMbs) && !url.GetHostName().empty()) {

      DOUT0("Try to create new MBS output transport type %s", typ.c_str());

      int kind = mbs::StreamServer;
      int portnum = 0;
      int connlimit = 0;

      if (url.GetPort()>0) {
         portnum = url.GetPort();
         if (portnum==DefualtServerPort(mbs::TransportServer))
            kind = mbs::TransportServer;
      }

      if (!url.GetHostName().empty())
         kind = StrToServerKind(url.GetHostName().c_str());

      if (url.HasOption("limit"))
         connlimit = url.GetOptionInt("limit", connlimit);

      bool blocking = (kind == mbs::TransportServer);

      if (url.HasOption("nonblock")) blocking = false;

      if (portnum==0) portnum = DefualtServerPort(kind);

      int fd = dabc::SocketThread::StartServer(portnum);

      if (fd<=0) {
         DOUT0("Fail assign MBS server to port:%d", portnum);
         return 0;
      }

      DOUT0("Create MBS server fd:%d kind:%s port:%d limit:%d blocking:%s", fd, mbs::ServerKindToStr(kind), portnum, connlimit, DBOOL(blocking));

      dabc::SocketServerAddon* addon = new dabc::SocketServerAddon(fd, portnum);

      return new mbs::ServerTransport(cmd, portref, kind, addon, connlimit, blocking);
   }


   return dabc::Factory::CreateTransport(port, typ, cmd);
}


dabc::DataInput* mbs::Factory::CreateDataInput(const std::string& typ)
{
   DOUT2("Factory::CreateDataInput %s", typ.c_str());

   dabc::Url url(typ);
   if ((url.GetProtocol()==mbs::protocolLmd) && (url.GetFullName() == "Generator")) {
      DOUT0("Create LMD Generator input");
      return new mbs::GeneratorInput(url);
   } else
   if (url.GetProtocol()==mbs::protocolLmd) {
      DOUT0("LMD input file name %s", url.GetFullName().c_str());
      return new mbs::LmdInput(url);
   } else
   if (url.GetProtocol()=="lmd2") {
      DOUT0("LMD2 input file name %s", url.GetFullName().c_str());
      return new mbs::LmdInputNew(url);
   } else
   if (url.GetProtocol()=="lmdtxt") {
      DOUT0("TEXT LMD input file name %s", url.GetFullName().c_str());
      return new mbs::TextInput(url);
   }

   return 0;
}

dabc::DataOutput* mbs::Factory::CreateDataOutput(const std::string& typ)
{
   DOUT2("Factory::CreateDataOutput typ:%s", typ.c_str());

   dabc::Url url(typ);
   if (url.GetProtocol()==mbs::protocolLmd) {
      DOUT0("LMD output file name %s", url.GetFullName().c_str());
      return new mbs::LmdOutput(url);
   } else
   if (url.GetProtocol()=="lmd2") {
      DOUT0("LMD2 output file name %s", url.GetFullName().c_str());
      return new mbs::LmdOutputNew(url);
   }

   return 0;
}

dabc::Module* mbs::Factory::CreateModule(const std::string& classname, const std::string& modulename, dabc::Command cmd)
{
   if (classname == "mbs::CombinerModule")
      return new mbs::CombinerModule(modulename, cmd);

   if (classname == "mbs::Player")
      return new mbs::Player(modulename, cmd);

   return dabc::Factory::CreateModule(classname, modulename, cmd);
}

/** \page mbs_plugin MBS plugin for DABC (libDabcMbs.so)

\ingroup dabc_plugins

\subpage mbs_plugin_doc <br>

\subpage mbs_web_interface

 */


/** \page mbs_plugin_doc Short description of MBS plugin

Plugin designed to work with GSI DAQ system [MBS](http://daq.gsi.de) and
provides following components:
+ \ref mbs::LmdFile  - class for reading and writing of lmd files
+ \ref mbs::LmdOutput - output transport to store data in lmd files (like lmd://file.lmd)
+ \ref mbs::LmdInput  - input transport for reading of lmd files
+ \ref mbs::ClientTransport - input client transport to connect with running MBS node
+ \ref mbs::ServerTransport - output server transport to provide data as MBS server dose
+ \ref mbs::CombinerModule - module to combine events from several MBS sources
+ \ref mbs::Player - module to interact with MBS over DABC web interface
*/


/** \page mbs_web_interface Web interface to MBS

\ref mbs::Player module provides possibility to interact with MBS over DABC web interface


### XML file syntax

Example of configuration file is \ref plugins/mbs/app/web-mbs.xml

Following parameters could be specified for the module:

| Parameter | Description |
| --------: | :---------- |
|      node | Name of MBS node |
|    period | How often status information requested from MBS node |
|   history | Size of preserved history |
|  prompter | Argument, used for starting prompter |
|    logger | If true, logger will be readout |


Several MBS nodes can be readout at once:

~~~~~~~~~~~~~{.xml}
<?xml version="1.0"?>
<dabc version="2">
  <Context name="web-mbs">
    <Run>
      <lib value="libDabcMbs.so"/>
      <lib value="libDabcHttp.so"/>
    </Run>
    <Module name="mbs1" class="mbs::Player">
       <node value="r4-1"/>
       <period value="1"/>
       <history value="100"/>
       <prompter value=""/>
       <logger value="false"/>
    </Module>
    <Module name="mbs2" class="mbs::Player">
       <node value="r4l-1"/>
       <period value="1"/>
       <history value="100"/>
       <prompter value=""/>
       <logger value="false"/>
    </Module>
  </Context>
</dabc>
~~~~~~~~~~~~~



### MBS status record (port 6008)

In most situations started automatically by MBS.
mbs::Player module will periodically request status record and
calculate several rate values - data rate, event rate. Several log variables
are created, which reproduce output of **rate** command of MBS.


### MBS logger (port 6007)

When logger is enabled by MBS, one can connect to it and get log information.
If enabled (parameter logger is true), module will readout log information and put into
log record.


### MBS prompter (port 6006)

MBS prompter allows to submit MBS commands remotely and only remotely.
To start prompter on mbs, one should do:

~~~~~~~~~~~~~
rio4> rpm -r ARG
~~~~~~~~~~~~~

Value of argument after -r (here ARG) should be specified as value of **prompter**
configuration parameter in xml file. In that case module will publish **CmdMbs** command,
which can be used from web interface to submit commands like
"start acq", "stop acq", "@startup", "show rate" and others to MBS process.


*/

