#ifndef KERNELMEMORY_H_
#define KERNELMEMORY_H_

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
 * $Log: KernelMemory.h,v $
 * Revision 1.5  2008/04/07 16:09:05  adamczew
 * *** empty log message ***
 *
 * Revision 1.1  2006/10/13 17:18:34  marcus
 * Implemented and tested most of C++ interface.
 *
 *******************************************************************/

#include "PciDevice.h"

namespace pciDriver {

class KernelMemory {
	friend class PciDevice;
	
protected:
	unsigned long pa;
	unsigned long size;
	int handle_id;
	void *mem;
	PciDevice *device;

	KernelMemory(PciDevice& device, unsigned int size);
public:
	~KernelMemory();
	
	inline unsigned long getPhysicalAddress() { return pa; }
	inline unsigned long getSize() { return size; }
	inline void *getBuffer() { return mem; }
	
	enum sync_dir {
		BIDIRECTIONAL,
		FROM_DEVICE,
		TO_DEVICE
	};
	
	void sync(sync_dir dir);
};
	
}

#endif /*KERNELMEMORY_H_*/
