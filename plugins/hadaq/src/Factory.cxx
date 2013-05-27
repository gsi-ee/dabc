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

#include "hadaq/Factory.h"

#include "dabc/string.h"
#include "dabc/logging.h"
#include "dabc/Command.h"
#include "dabc/Manager.h"
#include "dabc/SocketThread.h"
#include "dabc/MemoryPool.h"
#include "dabc/Port.h"
#include "dabc/Url.h"

#include "hadaq/HadaqTypeDefs.h"
#include "hadaq/HldInput.h"
#include "hadaq/HldOutput.h"
#include "hadaq/UdpTransport.h"
#include "hadaq/CombinerModule.h"
#include "hadaq/MbsTransmitterModule.h"
#include "hadaq/Observer.h"

dabc::FactoryPlugin hadaqfactory(new hadaq::Factory("hadaq"));


dabc::DataInput* hadaq::Factory::CreateDataInput(const std::string& typ)
{
   dabc::Url url(typ);
   if (url.GetProtocol()=="hld") {
      DOUT0("HLD input file name %s", url.GetFullName().c_str());

      return new hadaq::HldInput(url);
   }

   return 0;
}

dabc::DataOutput* hadaq::Factory::CreateDataOutput(const std::string& typ)
{
   dabc::Url url(typ);
   if (url.GetProtocol()=="hld") {
      DOUT0("HLD output file name %s", url.GetFullName().c_str());
      return new hadaq::HldOutput(url);
   }

   return 0;
}

dabc::Transport* hadaq::Factory::CreateTransport(const dabc::Reference& port, const std::string& typ, dabc::Command cmd)
{
   dabc::Url url(typ);

   dabc::PortRef portref = port;

   if (portref.IsInput() && (url.GetProtocol()=="hadaq") && !url.GetHostName().empty()) {

      int nport = url.GetPort();
      int rcvbuflen = url.GetOptionInt("buf", 1 << 20);

      nport = portref.Cfg(hadaq::xmlUdpPort, cmd).AsInt(nport);
      rcvbuflen = portref.Cfg(hadaq::xmlUdpBuffer, cmd).AsInt(rcvbuflen);
      int mtu = portref.Cfg(hadaq::xmlMTUsize, cmd).AsInt(63*1024);

      if (nport>0) {

         int fd = DataSocketAddon::OpenUdp(nport, rcvbuflen);

         if (fd>0) {
            DataSocketAddon* addon = new DataSocketAddon(fd, nport, mtu);

            return new hadaq::DataTransport(cmd, portref, addon);
         }
      }
   }


   return dabc::Factory::CreateTransport(port, typ, cmd);
}



dabc::Module* hadaq::Factory::CreateModule(const std::string& classname, const std::string& modulename, dabc::Command cmd)
{
   if (classname == "hadaq::CombinerModule")
      return new hadaq::CombinerModule(modulename, cmd);

   if (classname == "hadaq::MbsTransmitterModule")
      return new hadaq::MbsTransmitterModule(modulename, cmd);

   return dabc::Factory::CreateModule(classname, modulename, cmd);
}


void hadaq::Factory::Initialize()
{
   hadaq::Observer* observ = new hadaq::Observer("/shm");
   dabc::WorkerRef w = observ;
   if (!observ->IsEnabled()) {
      w.Destroy();
   } else {
      DOUT0("Initialize SHM connected control");
      w.MakeThreadForWorker("ShmThread");
   }
}


/** \page hadaq_plugin HADAQ plugin for DABC (libDabcHadaq.so)
 *
 *  \subpage hadaq_plugin_doc
 *
 *  \n
 *
 *  \subpage hadaq_trb3_package
 *
 *
 *  \ingroup dabc_plugins
 *
 */


/** \page hadaq_plugin_doc Short description of HADAQ plugin
 *
 * This should be description of HADAQ plugin for DABC.
 *
 */


/** \page hadaq_trb3_package Software collection for TRB3

Introduction
============

This is short instruction how dabc/go4/stream frameworks can be used for
data taking and online/offline analysis of TRB3 data.

There are two parts of software:
  1. DABC which is required to take data from TRB3 and store to hld or lmd files
  2. ROOT/Go4/stream frameworks to analyse online/offline TRB3/TDC data

The easiest way to install necessary software is to use repository
   https://subversion.gsi.de/dabc/trb3



Installation of all components in once
======================================

This method describes how DABC, ROOT, Go4 and stream analysis can be installed
with minimal efforts.

Requirements
------------

Following packages should be installed:
   * libqt4-devel
   * xorg-devel
   * g++

It is recommended to use bash (at least, during compilation)


Reuse existing ROOT installation
--------------------------------

Most of the time is consumed by ROOT compilation, therefore if ROOT
already installed on your machine, it can be reused. Just configure
ROOTSYS, PATH and LD_LIBRARY_PATH variables before starting.
For instance, call thisroot.sh script:

    [shell] . your_root_path/bin/thisroot.sh


Compilation
-----------

To get and compile all components, just do:

    [shell] svn co https://subversion.gsi.de/dabc/trb3 trb3
    [shell] cd trb3
    [shell] make -j4

During compilation makelog.txt file will be created in each sub-directory.
In case of any compilation problem please send me (S.Linev@gsi.de)
error message from that file.


Before using
------------

There is login script 'trb3login', which must be called before software can be used

    [shell] . your_trb3_path/trb3login

It set all shell variables, which are required for DAQ and analysis


Update from repository
----------------------

To obtain newest version from repository do:

    [shell] cd your_trb3_path
    [shell] svn up
    [shell] make -j4

Please perform these commands from the shell, where trb3login was not called.

If for that ever reasons compilation did not work after update, try recompile completely:

    [shell] make clean
    [shell] make -j4



Installation of packages separately
===================================

Use this methods only if installation of all packages at once did not work or
one need only some specific package to install.

Installation of DABC
--------------------

If only DAQ functionality is required, than one need to install DABC only.
It is much faster and easier. Just do:

    [shell] svn co https://subversion.gsi.de/dabc/trunk dabc
    [shell] cd dabc; make nodim=1 -j4
    [shell] . ./dabclogin

Installation of ROOT
--------------------

See http://root.cern.ch for information

Installation of Go4
-------------------

Main information in http://go4.gsi.de.
To install from repository first initialize ROOT variables, than do:

    [shell] svn co https://subversion.gsi.de/go4/trunk go4
    [shell] cd go4; make -j4
    [shell] . ./go4login

Installation of stream
----------------------

First compile and configure go4. Than:

    [shell] svn co https://subversion.gsi.de/go4/app/stream stream
    [shell] cd stream; make -j4
    [shell] . ./streamlogin



Running of DAQ
==============

To run DAQ, only DABC installation is required.

Example configuration file is in https://subversion.gsi.de/dabc/trunk/plugins/hadaq/app/EventBuilder.xml
Copy it in any suitable place and modify it for your needs.

Main configuration parameters:

Memory pool
-----------

Defines number and size of buffers, used in application. Normally must remain as is.
Should be increased if queue sizes of input/output ports are increased

Combiner module
---------------

It is central functional module of DAQ application.
It could have arbitrary number of inputs, defined by NumInputs parameter.
Each input corresponds to separate TRB3 board which should be readout.
For each input only correct port number should be assigned in line like:

     <InputPort name="Input0" url="hadaq://host:10101"/>

Here only port number is relevant, all other parameters must remain as is.

Events, produced by combiner module, can be stored in hld file or (and) delivered
via online server to online analysis.


Write HLD files
---------------

To write HLD files, one should specify following parameters in combiner module:

     <NumOutputs value="2"/>

     <OutputPort name="Output1" url="hld://dabc.hld?maxsize=30"/>

 Only second output port (name Output1) can be use for HLD file storage.
 maxsize defines maximum size (in MB) of file, which than will be closed and new
 file will be started

 ## Configure online server

 Combiner module should have at least one output and
 auto="true" should be specified for "OnlineServer" module.
 hadaq events will be delivered to "OnlineServer" module, where
 conversion into lmd format will be done(just adding lmd headers).

 Produced data can be provided to online server - configured for Output0:

        <OutputPort name="Output0" url="mbs://Stream"/>

 At the same time, data can be stored to lmd file. One should set two output ports
 and name of output file provide in configuration of second output port:

        <OutputPort name="Output1" url="lmd://test.lmd?maxsize=30&log=2"/>

 Here maxsize also defines maximum size of output file.


Running DABC
------------

Once configuration file is adjusted, one should call:

     [shell] dabc_exe EventBuilder.xml

Execution can always be regularly stopped by Ctrl-C.
All opened files will be closed normally.



Running analysis
================

Analysis code is provided in new stream project.
It is generic analysis framework, dedicated for synchronization and
processing of different kinds of time-stamped data streams. Classes,
relevant for TRB3/FPGA-TDC processing situated in
https://subversion.gsi.de/go4/app/stream/include/hadaq/ and
https://subversion.gsi.de/go4/app/stream/framework/hadaq/

In principle, in most cases it is not required to change these classes -
all user-specific configurations provided in ROOT scripts, which can be
found in https://subversion.gsi.de/go4/app/stream/applications/trb3tdc/first.C.
It is example how to process data from single TDC, many of them can be created
if necessary. Please read comment in script itself.
One can always copy such script to any other location and modify it to specific needs.


Running in batch
----------------

To run analysis in batch (offline), start from directory where first.C script is
situated:

    [shell]  go4analysis -user file_0000.hld

or if lmd files were written:

    [shell]  go4analysis -file file_0000.lmd

Advantage of lmd file usage - wildcard expression can be specified like:

    [shell]  go4analysis -file file_000*.lmd

After analysis finished, filled histograms will be saved in Go4AutoSave.root file and
can be viewed in ROOT or in Go4 browser. Just type:

    [shell] go4 Go4AutoSave.root

There are many parameters of go4analysis executable (run go4analysis -help).
For instance, one can run only specified number of events or change output file name:

    [shell] go4analysis -user file_0000.hld -number 100000 -asf new_name.root


Running analysis online
-----------------------

First of all, online server should be configured in DABC. In any moment one
could start analysis from batch, connecting to DABC server with command:

    [shell] go4analysis -stream dabc_host_name

With Ctrl-C one can always stop execution and check histograms in auto-save file.

But more convenient way is to run analysis from the gui to be able monitor
all histogram in live mode. For that one need:
1. start go4 gui (type go4) from directory with first.C macro
2. Select "Launch analysis" menu command in go4
3. set "Dir" parameter to "." (current directory)
4. keep empty library name file of analysis code
    (library will be located automatically by go4 itself)
5. when analysis configuration window appears,
   select "MBS stream server" and host name of DABC
   (can be localhost if DABC runs on same machine)
6. press "Submit and start" button

Via analysis browser one can display and monitor any histogram.
For more details about go4 see introduction on http://go4.gsi.de.


Any comments and wishes: S.Linev@gsi.de

 */
