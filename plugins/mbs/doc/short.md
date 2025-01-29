# Short description of MBS plugin {#mbs_readme}

Plugin designed to work with GSI DAQ system [MBS](http://daq.gsi.de) and
provides following components:

   - @ref mbs::LmdFile  - class for reading and writing of lmd files
   - @ref mbs::LmdOutput - output transport to store data in lmd files (like lmd://file.lmd)
   - @ref mbs::LmdInput  - input transport for reading of lmd files
   - @ref mbs::ClientTransport - input client transport to connect with running MBS node
   - @ref mbs::ServerTransport - output server transport to provide data as MBS server dose
   - @ref mbs::CombinerModule - module to combine events from several MBS sources
   - @ref mbs::Monitor - module to interact with MBS over DABC web interface
