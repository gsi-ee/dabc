/*******************************************************************
 * Change History:
 * 
 * $Log: Exception.cpp,v $
 * Revision 1.5  2008/04/07 16:09:06  adamczew
 * *** empty log message ***
 *
 * Revision 1.2  2007-02-09 17:02:39  marcus
 * Modified Exception handling, made simpler and more standard.
 *
 * Revision 1.1  2006/10/13 17:18:32  marcus
 * Implemented and tested most of C++ interface.
 *
 *******************************************************************/

#include "Exception.h"

using namespace pciDriver;

const char* Exception::descriptions[] = {
	"Unknown Exception",
	"Device Not Found",
	"Invalid BAR",
	"Internal Error",
	"Open failed",
	"Not Open",
	"Mmap failed",
	"Alloc failed",
	"SGmap failed",
	"Interrupt failed"
};


