#ifndef USERMEMORY_H_
#define USERMEMORY_H_

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
 * $Log: UserMemory.h,v $
 * Revision 1.5  2008/04/07 16:09:05  adamczew
 * *** empty log message ***
 *
 * Revision 1.2  2006/11/17 18:50:17  marcus
 * Modified constructor, add 'merged' select switch.
 *
 * Revision 1.1  2006/10/13 17:18:34  marcus
 * Implemented and tested most of C++ interface.
 *
 *******************************************************************/

#include "PciDevice.h"

namespace pciDriver {

class UserMemory {
	friend class PciDevice;
private:
	struct sg_entry {
		unsigned long addr;
		unsigned long size;
	};
	
protected:
	unsigned long vma;
	unsigned long size;
	int handle_id;
	PciDevice *device;
	int nents;
	struct sg_entry *sg;

	UserMemory(PciDevice& device, void *mem, unsigned int size, bool merged );
public:
	~UserMemory();
	
	enum sync_dir {
		BIDIRECTIONAL,
		FROM_DEVICE,
		TO_DEVICE
	};
	
	void sync(sync_dir dir);

	inline unsigned int getSGcount() { return nents; }	
	inline unsigned long getSGentryAddress(unsigned int entry ) { return sg[entry].addr; }
	inline unsigned long getSGentrySize(unsigned int entry ) { return sg[entry].size; }
	
};

}

#endif /*USERMEMORY_H_*/
