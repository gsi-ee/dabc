--------------------------------------------------------------
         The Data Acquisition Backbone Core 
         DABC Release v1.1.0  (25-January-2011)
-------------------------------------------------------------
Copyright (C) 2009- 
GSI Helmholtzzentrum fuer Schwerionenforschung GmbH 
Planckstr. 1
64291 Darmstadt
Germany
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

These packages are tested on
    Debian 3.1 woody (32 bit and AMD 64bit)
    Debian 4.0 Etch (32 bit)
    SuSe 11.1 (64bit)
    compilers: gcc 3.3.5, gcc 4.3.x

  
INSTALLATION:

1. Unpack this DABC distribution in the future DABC System directory,
  e.g. /usr/local/pub/dabc 
  ("cd /usr/local/pub/dabc; tar zxvf dabc.tar.gz")

2. Edit your dabclogin.sh script:
  A template for this script is scripts/dabclogin.sh
  Edit the DABCSYS environment according to your local installation directory
  Edit the DIM_DNS_NODE environment according to the machine where 
  the DIM name server will run
  Put the script at a location in your global $PATH for later login.

3. To compile DIM with Java extensions, JDK_INCLUDE variable should be set.
   By default, $JAVA_HOME/include path is used by DABC. This means, that
   JAVA_HOME should point on JDK installation, JVM is not sufficient.
   One can set JDK_INCLUDE explicitly in dabclogin.sh script.  
   
4. Execute your ". dabclogin.sh" in your shell to set the environment
  for compilation.

5. Change to the dabc installation directory and start the build:
  "cd $DABCSYS; make"

6. after successful compilation, you may use DABC from any shell after
execution of ". dabclogin.sh"

7.use executable "dabc_run" for single node batch mode.

8. For GUI controls: 
    * Start the DIM name server _once_ on the machine you specified in 
      the dabclogin.sh by calling "dimDns".
    * start the dabc GUI in another shell by typing "dabc" 
      (do not forget to call ". dabclogin.sh" first!)

9. Read the DABC User Manual  for further information how to configure
  the DABC and how to use the GUI!

Please contact the DABC developer team for any bug reports and wishes!

