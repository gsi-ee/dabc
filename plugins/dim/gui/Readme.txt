How to use the GUI

Complete GUI is stored in JAR file dabc/gui/java/packages/xgui.jar.
This file may be copied anywhere including Windows machines.

There are some config xml files in dabc/gui/java/generic/config:
DabcLaunch.xml : field values for DABC launcher
MbsLaunch.xml  : field values for MBS launcher
Layout.xml     : Window geometry

These three files should be copied to the directory where the GUI
will be started from. It then reads theese files and configures
the windows from the files. After adjusting the windows positions
and sizes, and the appearance, one can save this setup by the
Button next to the exit button into Layout.xml.

Requirements:
DIM installation.
DIM name server running
Java 1.5 or higher

Linux:
DIM_DNS_NODE to node of dim name server
DIMDIR to top directory of DIM installation
DABCGUI to jarfile
CLASSPATH to $DIMDIR/jdim/classes:$DABCGUI
LD_LIBRARY_PATH append $DIMDIR/linux

Windows:
DIM_DNS_NODE to node of dim name server
DIMDIR to top directory of DIM installation
DABCGUI to jarfile
CLASSPATH append ;%DIMDIR%\jdim\classes;%DABCGUI%
PATH append ;%DIMDIR%\bin
USER to user for login on remote nodes
HOST name of GUI node

Start GUI on both:
java xgui.xGui


