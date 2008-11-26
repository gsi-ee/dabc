This is standalone API to read/write lmd files.

It includes C-based code, LmdFile class (C++ wrapper for C code) and 
several structures definitions in LmdTypeDefs.h to work with 
mbs event structures.

Install:
   1. Unpack in any empty directory:
         [shell] tar xzf lmdfile.tar.gz
   2. If one would like to use lmd files in ROOT scripts,
      install ROOT (see site http://root.cern.ch) and configure correctly
      ROOTSYS and LD_LIBRARY_PATH environment variable.
   3. Compile code by invoking:
         [shell] make
      It will create library "libLmdFile.so" and executable "test".
      
Examples:
   1. Standalone program in "test.cxx". It is compiled together with library
      and shows how one can iterate over all events in lmd file.
   2. ROOT script in "test.C". It shows possibility to read lmd files in ROOT
      script. To run it, just call from shell:
         [shell] root -b test.C -q
         