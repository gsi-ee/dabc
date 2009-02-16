/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009- 
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH 
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/
#ifndef DABC_Exception
#define DABC_Exception

#include <exception>
#include <string>

namespace dabc {
  
  class Exception : public std::exception {
     protected:
        std::string fWhat;
     
     public:
        Exception() throw();
        Exception(const char* info) throw();
        virtual ~Exception() throw();
      
        virtual const char* what() const throw() { return fWhat.c_str(); }
  };
  
  class StopException : public Exception {
      public: 
         StopException() throw () : Exception("Stop exception") {}
   };

   class TimeoutException : public Exception {
      public: 
         TimeoutException() throw () : Exception("Timeout exception") {}
   };
  
};

#endif
