--------------------------------------------------------------
          The Data Acquisition Backbone Core
         DABC Release v2.11.0 (22-Oct-2021)
-------------------------------------------------------------
Copyright (C) 2009 -
GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
Planckstr. 1, 64291 Darmstadt, Germany
Contact:  http://dabc.gsi.de
------------------------------------------------------------
Authors: Joern Adamczewski-Musch
         Hans Georg Essel
         Sergey Linev

(Data Processing group at GSI Experiment Electronics department)
---------------------------------------------------------------
DISCLAIMER:
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details (http://www.gnu.org).
---------------------------------------------------------------

This package was tested on:
    Debian 9.0 (64 bit)
    OpenSuSe Tumblweed 10.2021
    MacOS 10.14.6
    compilers: gcc 11.1, clang 13.0


INSTALLATION:

1. Checkout from repository:

      [shell] cd /home/user/git
      [shell] git clone https://github.com/gsi-ee/dabc.git

2. Compile with cmake

     [shell] mkdir build
     [shell] cd build
     [shell] cmake /home/user/git/dabc
     [shell] make -j

   One could switch on/off different plugins calling `cmake -Dverbs=OFF`

3. Install with cmake

      [shell] mkdir -p /home/user/soft/dabc
      [shell] cmake /home/user/soft/dabc -DCMAKE_INSTALL_PREFIX=/home/user/soft/dabc
      [shell] make -j
      [shell] cmake --install .

4. After successful compilation "dabclogin" script will be generated,
   which should be called like ". dabclogin" before dabc can be used

5. Use executable "dabc_exe you_file.xml" for single node applications,
   "dabc_run you_file.xml" for multi-node applications

6. Read the DABC online documentation (on https://dabc.gsi.de)
   for further information how to configure the DABC and how to use the GUI!

Please contact the DABC developer team for any bug reports and wishes!
