\page ltsm_plugin ltsm plugin for DABC (libDabcLtsm.so)

\subpage ltsm_plugin_doc

\ingroup dabc_plugins


\page ltsm_plugin_doc Short description of LTSM plugin

## Introduction
Plugin provides implementation of dabc::FileInterface class for
ltsm - remote file I/O using light tivoli storage manger by T.Stibor.
(https://github.com/tstibor/ltsm)

## Compilation
The ltsm library must be installed first on the system.
Please see https://github.com/tstibor/ltsm to get the sources and how to install

To use any other ltsm installation, the
environment LTMSAPI should contain necessary includes and `libtsmapi.a`:

    make LTSMAPI=/home/adamczew/git/ltsm/src/lib

To disable plugin compilation, noltsm variable should be specified:

    make noltsm=1
