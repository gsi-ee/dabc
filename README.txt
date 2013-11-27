--------------------------------------------------------------
          The Data Acquisition Backbone Core 
         DABC Release v2.6.0 (27-November-2013)
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
    compilers: gcc 4.3.x, gcc 4.4.x, 4.5.x, 4.7.x 

  
INSTALLATION:

1. Unpack this DABC distribution in the future DABC system directory.
   
      [shell] cd /usr/local/pub/dabc 
      [shell] tar xzvf /path/to/the/file/dabc.tar.gz 
  
   or checkout DABC from repository:
   
      [shell] cd /usr/local/pub 
      [shell] svn co https://subversion.gsi.de/dabc/trunk dabc
   
3. To compile dabc, just call make:
   
       [shell] make
       
    One could specify following options:
    
       noverbs=1 - disable compilation of IB VERBS plugin. In some situation
                   plugin can have problem during compilation and can be disabled
                   with such option
       
4. After successful compilation "dabclogin" script will be generated,
   which should be called like ". dabclogin" before dabc can be used 

5. Use executable "dabc_exe you_file.xml" for single node applications, 
   "dabc_run you_file.xml" for multi-node applications

6. Read the DABC online documentation (on dabc.gsi.de) 
   for further information how to configure the DABC and how to use the GUI!

Please contact the DABC developer team for any bug reports and wishes!
