\page rfio_plugin RFIO plugin for DABC (libDabcRfio.so)

\subpage rfio_plugin_doc

\ingroup dabc_plugins


\page rfio_plugin_doc  Short description of RFIO plugin

## Introduction
Plugin provides implementation of dabc::FileInterface class for
RFIO - remote file I/O.

## Compilation

Together with DABC actual version of ADSM library is distributed.
It is automatically compiled with all other plugins. One could use
other version, setting ADSMDIR variable during DABC compilation.
ADSMDIR should contain necessary includes and librawapiclin64.a.

    make ADSMDIR=/home/user/adsmv80

To disable plugin compilation, norfio variable should be specified:

    cmake .. -Drfio=OFF
