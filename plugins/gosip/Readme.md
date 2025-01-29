\page gosip_plugin GOSIP plugin for DABC (libDabcGosip.so)

\subpage gosip_plugin_doc

\ingroup dabc_plugins


\page gosip_plugin_doc Short description of GOSIP plugin

This plugin consists of two independent functional parts
1. A webserver application (class *gosip::Player*) that may execute gosipcmd as a shell commands on the server
when called from a http request. This is e.g. used in the MBS web gui. Additionally, a custom web gui example is provided in htm subfolder
with an (very early) version of the POLAND hardware gui.

2. A command server (class *gosip::TerminalModule*) that can be started on a node with kinpex/pexor hardware and mbspex driver. This command
server can receive gosip commands via dabc socket from a remote command application *rgoc*. Syntax and usage of *rgoc* is identical with the
established local *gosipcommand* (alias *goc*) cli interface, except for the remote nodename of the commandserver required as first argument.

## Starting the commmand server
The command server can be started on a node t
hat hosts the kinpex optical receiver board which connects to the front-end slaves.
An example configuraton file is given in `plugins/gosip/app/commandserver.xml`:

~~~~~~~~~~~~~
adamczew@X86L-59:~/workspace/DABC.git/plugins/gosip/app$ dabc_exe commandserver.xml
GosipCommandServer   0.127366 Library loaded libDabcGosip.so
GosipCommandServer   0.135939 Start DABC server on localhost:12345
GosipCommandServer   0.138081 Application mainloop is now running
GosipCommandServer   0.138141        Press Ctrl-C for stop
GosipCommandServer   0.139443 Starting GOSIP command server module
~~~~~~~~~~~~~



## Usage of rgoc


```
***************************************************************************
 rgoc (remote gosipcmd) for dabc and mbspex library
 v0.3 07-Dec-2022 by JAM (j.adamczewski@gsi.de)
***************************************************************************

  usage: rgoc [-h|-z] [[-i|-r|-w|-s|-u] [-b] | [-c|-v FILE] [-n DEVICE |-d|-x] node[:port] sfp slave [address [value [words]|[words]]]]

         Options:
                 -h        : display this help
                 -z        : reset (zero) pexor/kinpex board
                 -i        : initialize sfp chain
                 -r        : read from register
                 -w        : write to  register
                 -s        : set bits of given mask in  register
                 -u        : unset bits of given mask in  register
                 -b        : broadcast io operations to all slaves in range (0-sfp)(0-slave)
                 -c FILE   : configure registers with values from FILE.gos
                 -v FILE   : verify register contents (compare with FILE.gos)
                 -n DEVICE : specify device number N (/dev/pexorN, default:0)
                 -d        : debug mode (verbose output)
                 -x        : numbers in hex format (defaults: decimal, or defined by prefix 0x)
         Arguments:
                 node:port - nodename of remote gosip command server (default port 12345)
                 sfp       - sfp chain- -1 to broadcast all registered chains
                 slave     - slave id at chain, or total number of slaves. -1 for internal broadcast
                 address   - register on slave
                 value     - value to write on slave
                 words     - number of words to read/write/set incrementally
         Examples:
          rgoc -z -n 1 x86l-59                   : master gosip reset of board /dev/pexor1 at node x86l-59
          rgoc -i x86l-59 0 24                   : initialize chain at sfp 0 with 24 slave devices
          rgoc -r -x x86l-59 1 0 0x1000          : read from sfp 1, slave 0, address 0x1000 and printout value
          rgoc -r -x x86l-59 0 3 0x1000 5        : read from sfp 0, slave 3, address 0x1000 next 5 words
          rgoc -r -b  x86l-113 1 3 0x1000 10      : broadcast read from sfp (0..1), slave (0..3), address 0x1000 next 10 words
          rgoc -r --  x86l-42 -1 -1 0x1000 10     : broadcast read from address 0x1000, next 10 words from all registered slaves
          rgoc -w -x  x86l-113 0 3 0x1000 0x2A     : write value 0x2A to sfp 0, slave 3, address 0x1000
          rgoc -w -x  x86l-113 1 0 20000 AB FF     : write value 0xAB to sfp 1, slave 0, to addresses 0x20000-0x200FF
          rgoc -w -b  localhost 1  3 0x20004c 1    : broadcast write value 1 to address 0x20004c on sfp (0..1) slaves (0..3)
          rgoc -w --  x86l-113 -1 -1 0x20004c 1    : write value 1 to address 0x20004c on all registered slaves (internal driver broadcast)
          rgoc -s     x86l-113 0 0 0x200000 0x4      : set bit 100 on sfp0, slave 0, address 0x200000
          rgoc -u     x86l-113 0 0 0x200000 0x4 0xFF : unset bit 100 on sfp0, slave 0, address 0x200000-0x2000FF
          rgoc -x -c  x86l-113  run42.gos           : write configuration values from file run42.gos to slaves
*****************************************************************************
```

## Compilation
To enable compilation of gosip plugin, select the flag in cmake configuration like:

   cmake -Dgosip=on  <path_to_source_directory>

The TerminalModule requires to link against
`libmbspex.so`. The rgoc command line tool works independently from any hardware driver library.
Because of this, rgoc is build on any host when gosip=on is set, whereas libDabcGosip.so is build only
if the includes and library of mbspex driver software is found on the system.

**Please note: the predefined search path in cmake build system is tailored for GSI MBS Linux standard installations
of libmbspex.so and includes**
