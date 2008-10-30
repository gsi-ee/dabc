#include "dabc/Exception.h"

#include "dabc/logging.h"

dabc::Exception::Exception() throw() : 
   std::exception(),
   fWhat("exception") 
{ 
   DOUT2(( "Generic exception")); 
}

dabc::Exception::Exception(const char* info) throw() : 
   std::exception(),
   fWhat(info) 
{ 
   DOUT2(( "Exception %s", info)); 
}

dabc::Exception::~Exception() throw() 
{
}
