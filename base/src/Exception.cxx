// $Id$

/************************************************************
 * The Data Acquisition Backbone Core (DABC)                *
 ************************************************************
 * Copyright (C) 2009 -                                     *
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH      *
 * Planckstr. 1, 64291 Darmstadt, Germany                   *
 * Contact:  http://dabc.gsi.de                             *
 ************************************************************
 * This software can be used under the GPL license          *
 * agreements as stated in LICENSE.txt file                 *
 * which is part of the distribution.                       *
 ************************************************************/

#include "dabc/Exception.h"

#include "dabc/logging.h"

dabc::Exception::Exception() throw() : 
   std::exception(),
   fKind(ex_None),
   fWhat(),
   fItem()
{ 
   DOUT2("Generic exception");
}

dabc::Exception::Exception(const std::string &info, const std::string &item) throw() :
   std::exception(),
   fKind(ex_Generic),
   fWhat(info),
   fItem(item)
{
   DOUT2( "Exception %s from item %s", what(), item.c_str());
}


dabc::Exception::Exception(ExceptionKind _kind, const std::string &info, const std::string &item) throw() :
   std::exception(),
   fKind(_kind),
   fWhat(info),
   fItem(item)
{
   DOUT2( "Exception %s from item %s", what(), item.c_str());
}


dabc::Exception::~Exception() throw() 
{
}

const char* dabc::Exception::what() const throw()
{
   switch(kind()) {
      case ex_None:    return "undefined";
      case ex_Stop:    return "stop";
      case ex_Timeout: return "timeout";
      case ex_Input:   return "input";
      case ex_Output:  return "output";
      case ex_Connect: return "connect";
      case ex_Disconnect: return "disconnect";
      case ex_Command: return fWhat.c_str();
      case ex_Buffer:  return fWhat.c_str();
      case ex_Pool:    return fWhat.c_str();
      case ex_Object:  return fWhat.c_str();
      case ex_Hierarchy: return fWhat.c_str();
      case ex_Pointer: return fWhat.c_str();
      case ex_Generic: return fWhat.c_str();
   }

   return "uncknown";
}
