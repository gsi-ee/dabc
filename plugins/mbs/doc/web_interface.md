\page mbs_web_interface Web interface to %MBS

@ref mbs::Monitor module provides possibility to interact with MBS over DABC web interface


## XML file syntax

Example of configuration file is @ref plugins/mbs/app/web-mbs.xml

Following parameters could be specified for the module:

| Parameter | Description |
| --------: | :---------- |
|      node | Name of MBS node |
|    period | How often status information requested from MBS node [default 1 sec] |
|   history | Size of preserved history [default 200] |
|       cmd | port on MBS node to submit commands (typically 6019) [default - none] |
|    logger | port on MBS node to read log information (normally 6007) [default - none] |


Several MBS nodes can be readout at once:

~~~~~~~~~~~~~{.xml}
<?xml version="1.0"?>
<dabc version="2">
  <Context name="web-mbs">
    <Run>
      <lib value="libDabcMbs.so"/>
      <lib value="libDabcHttp.so"/>
    </Run>
    <Module name="mbs1" class="mbs::Monitor">
       <node value="r4-1"/>
       <log value="6007"/>
       <cmd value="6019"/>
    </Module>
    <Module name="mbs2" class="mbs::Monitor">
       <node value="r4l-1"/>
       <log value="6007"/>
    </Module>
  </Context>
</dabc>
~~~~~~~~~~~~~


## MBS status record (port 6008)

In most situations started automatically by MBS.
mbs::Monitor module will periodically request status record and
calculate several rate values - data rate, event rate. Several log variables
are created, which reproduce output of **rate** command of MBS.


## MBS remote command channel (port 6019)

To enable it, one should call 'sta cmdrem' in normal MBS processs. This functionality available
starting from MBS version 6.3. If XML file contains line `<cmd value="6019"/>`,
module will try to connect with MBS and will publish **CmdMbs** command.
Via web interface user can than submit commands like
`start acq`, `stop acq`, `@startup`, `show rate` to MBS process.


## MBS prompter (port 6006)

MBS prompter also allows to submit MBS commands remotely (but only remotely).
To start prompter on mbs, one should do:

    rio4> rpm -r client_host_name

Value of argument after -r should be host name, where DABC will run.


## MBS logger (port 6007)

When logger is enabled by MBS ('sta logrem'), one can connect to it and get log information.
At the moment logger is available in the MBS only together with the prompter.
If enabled, module will readout log information and put into log record.


## How to connect mbs dispatcher with dabc web server:
(Note: this requires MBS versions >=6.3!)

Two possibilities:

1) Start mbs dispatcher as normal ("mbs"). Invoke mbs commands "start cmdrem" (port 6019) and "start logrem" (port 6007).
Then run dabc process with web server that connects to these ports: "dabc_exe web-mbs-gui.xml"
(Please specify correct mbs node name in example xml configuration!)

2) start mbs dispatcher with option: "mbs -dabc". This will open command and log sockets at startup. Then
run  "dabc_exe web-mbs-gui.xml". The advantage of this method is that all startup commands can be send from
web interface.


## Customized web gui for kinpex/pexor node
In addition to the generic web interface, a control gui implementation is provided that provides buttons and
log panels to monitor and control a single node mbs with Linux PC and pexor/kinpex hardware.
An example configuration file of this is \ref plugins/mbs/app/web-mbs-gui.xml
To start specialized web gui, select in hierarchy treeview of the generic dabc website the
entry "MBS/<nodename>/ControlGUI". This will open control gui in separate window/tab.

### Main Sections of Control gui:
#### MBS:
- may send any mbs command to the dispatcher by command line
- shortcut buttons for `start acquisition`, `stop acquisition`, `@startup`, `@shutdown` commands
- shows output of logger, specifies history length
- shows/hides gosip panel
- background color indicates acquisition state of mbs (green: running, red: stopped)

#### GOSIP:
- may send any gosipcmd call to remote node
- shows output of gosipcmd execution, my clear output history
- starts user gui (POLAND frontend in this example) in new browser tab/window


#### Data Taking:
- may open and close lmd file on local disk or at rfio server
- can toggle automatic file creation mode
- shows output of rate/rash/rast/ratf monitors as in local mbs terminal
- may specify log output history
- background color indicates file state of mbs (green: open, red: closed)

#### Display Refresh:
- acquire update of shown status information from web server
- may turn on timer with selectable interval for frequent refresh mode
- background color indicates monitoring state of web gui (green: frequent refresh, red: manual refresh required)

#### Rates Display:
- shows ratemeters of collector data rate and event rate, and online monitoring data rate at optional stream server socket
- may switch between gauge style display, or trending graph with selectable history

