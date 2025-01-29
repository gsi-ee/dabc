\page mbs_root_plugin ROOT-MBS plugin for DABC (libDabcMbsRoot.so)

\ingroup dabc_plugins
\subpage mbs_root_plugin_doc


\page mbs_root_plugin_doc Info about ROOT-MBS plugin

Version 0.9 September 2010 by Ahl Balitaon, Joern Adamczewski-Musch

Defines a DabcEvent class to write in a root TTree
which contains the structure of mbs event with mbs subevent payload

Any mbs formatted dabc buffer which is connected to the RootTreeOutput
will be written to a ROOT Tree in raw format (generic data container).

Example file RocReadout.xml shows how parameters for plug-in are set
which are passed via CreateTransport in the
CreateAppModules() of the dabc application:

~~~~~~~~
   <CalibrFile value="/data.local1/adamczew/RocCalib.root"/>
   <RootSplitlevel value="99"/>
   <RootTreeBufsize value="32000"/>
   <RootCompression value="5"/>
   <RootMaxFileSize value="200000000"/>
~~~~~~~~

Note that tag CalibrFile is part of the ROC example, not part of the plug-in.

TODO:
- cleanup and global definition of naming strings for tags
- lmd file converter application
- root file input
- ...


\authour Joern Adamczewski-Musch
\fate 2-Dec-2010
