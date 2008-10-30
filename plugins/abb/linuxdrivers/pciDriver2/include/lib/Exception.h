#ifndef PCIDRIVER_EXCEPTION_H_
#define PCIDRIVER_EXCEPTION_H_

/********************************************************************
 * 
 * October 10th, 2006
 * Guillermo Marcus - Universitaet Mannheim
 * 
 * $Revision: 1.5 $
 * $Date: 2008/04/07 16:09:05 $
 * 
 *******************************************************************/

/*******************************************************************
 * Change History:
 * 
 * $Log: Exception.h,v $
 * Revision 1.5  2008/04/07 16:09:05  adamczew
 * *** empty log message ***
 *
 * Revision 1.3  2008-01-11 10:06:09  marcus
 * Solved ifdef name clash when importing in the mprace lib.
 *
 * Revision 1.2  2007-02-09 17:01:28  marcus
 * Added Exception as part of the standard exception hierarchy.
 * Simplified description handling, now using an array.
 *
 * Revision 1.1  2006/10/13 17:18:33  marcus
 * Implemented and tested most of C++ interface.
 *
 *******************************************************************/

#include <exception>

namespace pciDriver {
	
class Exception : public std::exception {
protected:
	int type;
public:

	enum Type {
		UNKNOWN,
		DEVICE_NOT_FOUND,
		INVALID_BAR,
		INTERNAL_ERROR,
		OPEN_FAILED,
		NOT_OPEN,
		MMAP_FAILED,
		ALLOC_FAILED,
		SGMAP_FAILED,
		INTERRUPT_FAILED
	};

	static const char* descriptions[];

	Exception(Type t) : type(t) {}
	inline int getType() { return type; }
	char const *toString() { return descriptions[type]; }
	virtual const char* what() const throw() { return descriptions[type]; }
};

}

#endif /*PCIDRIVER_EXCEPTION_H_*/
