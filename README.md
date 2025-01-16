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
