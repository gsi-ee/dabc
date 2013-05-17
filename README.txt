--------------------------------------------------------------
         The Data Acquisition Backbone Core 
         DABC Release v2.0.3  (17-May-2013)
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
    Debian 5.0/6.0 (32 bit & 64 bit)
    SuSe 11.2, 11.4, 12.1 (64bit)
    compilers: gcc 4.3.2, gcc 4.4.x 

  
INSTALLATION:

1. Unpack this DABC distribution in the future DABC System directory,
   e.g. /usr/local/pub/dabc 
   ("cd /usr/local/pub/dabc; tar zxvf dabc.tar.gz")
   
   or checkout DABC from repository:
   
   cd /usr/local/pub; svn co https://subversion.gsi.de/dabc/trunk dabc
   

2. To compile DIM with Java extensions, JDK_INCLUDE variable should be set.
   By default, $JAVA_HOME/include path is used by DABC. This means, that
   JAVA_HOME should point on JDK installation, JVM is not sufficient.
   One can set JDK_INCLUDE explicitly before starting compilation.  
   
3. Change to the dabc installation directory and start the build:
   "cd your_dabc_path; make"

4. After successful compilation "dabclogin" script will be generated,
   which should be called like ". dabclogin" before dabc can be used 

5. Use executable "dabc_exe you_file.xml" for single node applications, 
   "dabc_run you_file.xml" for multi-node applications

6. For DIM GUI controls: 
    * Start the DIM name server _once_ (by typing dimDns)
    * Specify node where name server is running with 
      export DIM_DNS_NODE=nodename 
    * start the dabc GUI in another shell by typing "dabc"
      (do not forget to call ". dabclogin.sh" first!)

7. Read the DABC User Manual for further information how to configure
   the DABC and how to use the GUI!

Please contact the DABC developer team for any bug reports and wishes!
