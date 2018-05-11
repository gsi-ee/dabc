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

#ifndef DABC_Exception
#define DABC_Exception

#include <exception>

#ifndef DABC_string
#include "dabc/string.h"
#endif

namespace dabc {
  
   enum ExceptionKind {
      ex_None,        // type is undefined
      ex_Stop,        // stop exception
      ex_Timeout,     // timeout exception
      ex_Input,       // exception with input port
      ex_Output,      // exception with output port
      ex_Connect,     // connect for any port
      ex_Disconnect,  // disconnect for any port
      ex_Command,     // command error,
      ex_Buffer,      // buffer error,
      ex_Pool,        // memory pool error
      ex_Object,      // object error
      ex_Hierarchy,   // hierarchy error
      ex_Pointer,     // pointer error
      ex_Generic      // generic exception, extra info is available
   };

   /** \brief DABC exception
    *
    * \ingroup dabc_core_classes
    * \ingroup dabc_all_classes
    *
    *
    * Generic exception for all situation
    * Not trying to duplicate classes hierarchy, just put exception name and
    * source item name into central class.
    * For convenience will provide several most common names
    * like IsStop(), IsTimeout() and so on
    */

   class Exception : public std::exception {
      protected:

         ExceptionKind fKind;
         std::string fWhat;
         std::string fItem;
     
     public:
         Exception() throw();

         Exception(const std::string &info, const std::string &item = "") throw();
         Exception(ExceptionKind _kind, const std::string &info, const std::string &item) throw();
         virtual ~Exception() throw();

         virtual const char* what() const throw();

         ExceptionKind kind() const { return fKind; }
         const std::string ItemName() const throw() { return fItem; }

         bool IsStop() const { return kind() == ex_Stop; }
         bool IsTimeout() const { return kind() == ex_Timeout; }
         bool IsInput() const { return kind() == ex_Input; }
         bool IsOutput() const { return kind() == ex_Output; }
         bool IsConnect() const { return kind() == ex_Connect; }
         bool IsDisconnect() const { return kind() == ex_Disconnect; }
         bool IsCommand() const { return kind() == ex_Command; }
         bool IsBuffer() const { return kind() == ex_Buffer; }
         bool IsPool() const { return kind() == ex_Pool; }
         bool IsPointer() const { return kind() == ex_Pointer; }
         bool IsObject() const { return kind() == ex_Object; }
         bool IsHierarchy() const { return kind() == ex_Hierarchy; }
   };

};

#endif
