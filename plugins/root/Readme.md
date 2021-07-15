\page root_plugin
## ROOT plugin for DABC (libDabcRoot.so)

\ingroup dabc_plugins

\subpage root_readme

[JavaScript ROOT](https://github.com/root-project/jsroot/blob/master/docs/JSROOT.md)

[THttpServer](https://github.com/root-project/jsroot/blob/master/docs/HttpServer.md)


\page root_readme
# Use of ROOT plugin {#root_readme}

## Introduction

Original motivation for writing of libDabcRoot plugin was
providing web interface to the arbitrary ROOT-based analysis.
Current THttpServer class in ROOT fully solving such tasks.

Now idea of this plugin - access to ROOT application with DABC methods.
Any ROOT application which runs THttpServer can create TDabcEngine and
be attached to master DABC application. It could be many of such ROOT process and
all of them could be access seamless via central http server, implemented in DABC.

Plugin also provides root::TreeStore class, which let use TTree storage in DABC
without direct linking of ROOT functionality to DABC plugins.

There is also dabc_root utility, which let convert DABC internal histograms
into ROOT representation.

Also this plugin was a place where JavaScript ROOT was developed for a while.
Now it is just copy of central JSROOT repository from https://github.com/linev/jsroot/


## DABC compilation

First of all, ROOT itself should be compiled and configured.
DABC uses ROOTSYS shell variable, therefore it should be defined before DABC compilation starts.
One could use ". your_root_path/bin/thisroot.sh" script.

Normal [DABC compilation procedure](\ref dabc_getting_started) should be performed.
At the end of the DABC compilation libDabcRoot.so library will be created in $DABCSYS/lib subdirectory.


## Running of web server

After DABC compiled and ". dabclogin" initialization script is called,
one just starts root session and creates [THttpServer]( https://root.cern/doc/master/classTHttpServer.html) instance at
any moment of macro execution. For instance:

    [shell] root -b -l

    root [0] new THttpServer("http:8095");
    root [1] .x $ROOTSYS/tutorials/hsimple.C
    hsimple   : Real Time =   0.14 seconds Cpu Time =   0.15 seconds
    (class TFile*)0x2b74510
    root [2]


After http server is started, one should be able to access it
with the address `http://your_host_name:8095/`

One could continue root usage, creating or deleting histograms or any other objects -
it will not disturb running server.

More information about ROOT http server one could find in [ROOT documentation](https://github.com/root-project/jsroot/blob/master/docs/HttpServer.md)


## Running of DABC server

In addition to ROOT-based http and fastcgi server, one could use following servers:
* DABC-based http server
* DABC-based fastcgi server
* DABC master socket
* DABC slave socket

### DABC-based http server

    root [0] new THttpServer("dabc:http:8095");

### DABC-based fastcgi server

    root [0] new THttpServer("dabc:fastcgi:9000");


### DABC master socket

    root [0] new THttpServer("dabc:1237");

This is DABC command socket. Go4 can connect with this socket and request
all objects in binary form.


### DABC slave socket

    root [0] new THttpServer("dabc:master_node:1237");

This is possibility to attach slave to some running master and export all
objects via master. This gives possibility to merge many slave jobs together.


For any comments and wishes contact \n
Sergey Linev   S.Linev(at)gsi.de
