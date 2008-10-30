/*******************************************************************
 * Change History:
 * 
 * $Log: UserMemory.cpp,v $
 * Revision 1.5  2008/04/07 16:09:06  adamczew
 * *** empty log message ***
 *
 * Revision 1.4  2007-10-12 15:01:15  marcus
 * Corrected border condition with the SG list.
 *
 * Revision 1.3  2007-02-09 17:02:38  marcus
 * Modified Exception handling, made simpler and more standard.
 *
 * Revision 1.2  2006/11/17 18:57:27  marcus
 * Added SGlist type support at runtime.
 *
 * Revision 1.1  2006/10/13 17:18:33  marcus
 * Implemented and tested most of C++ interface.
 *
 *******************************************************************/

#include "UserMemory.h"
#include "Exception.h"
#include "driver/pciDriver.h"

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

using namespace pciDriver;

UserMemory::UserMemory(PciDevice& dev, void *mem, unsigned int size, bool merged )
{
	int ret,i;
	umem_handle_t uh;
	umem_sglist_t sgl;
	int dev_handle;
	
	dev_handle = dev.getHandle();
	
	this->device = &dev;
	this->vma = reinterpret_cast<unsigned long>(mem);
	this->size = size;
	
	uh.vma = reinterpret_cast<unsigned long>(mem);
	uh.size = size;
	
	/* Lock and Map the memory to their pages */
	ret = ioctl( dev_handle, PCIDRIVER_IOC_UMEM_SGMAP, &uh );
	if (ret != 0) 
		throw Exception( Exception::SGMAP_FAILED );

	this->handle_id = uh.handle_id;

	/* Obtain the scatter/gather list for this memory */
	sgl.handle_id = uh.handle_id;
	sgl.type = ((merged) ? PCIDRIVER_SG_MERGED : PCIDRIVER_SG_NONMERGED);
	sgl.nents = (size / getpagesize()) + 2;
	sgl.sg = new umem_sgentry_t[ sgl.nents ];

	ret = ioctl( dev_handle, PCIDRIVER_IOC_UMEM_SGGET, &sgl );
	if (ret != 0) {
		ioctl( dev_handle, PCIDRIVER_IOC_UMEM_SGUNMAP, &uh );
		throw Exception( Exception::SGMAP_FAILED );
	}

	/* Copy the Scatter / Gather list to our structures */
	this->nents = sgl.nents;
	this->sg = new struct sg_entry[ sgl.nents ];
	for(i=0; i< sgl.nents; i++) {
		this->sg[i].addr = sgl.sg[i].addr;
		this->sg[i].size = sgl.sg[i].size;		
	}
	
	/* Temp SG list for the driver is no longer needed */
	delete sgl.sg;
}

UserMemory::~UserMemory()
{
	int ret;
	umem_handle_t uh;

	delete this->sg;

	uh.handle_id = handle_id;
	uh.vma = vma;
	uh.size = size;

	ret = ioctl( device->getHandle(), PCIDRIVER_IOC_UMEM_SGUNMAP, &uh );
	if (ret != 0)
		throw Exception( Exception::INTERNAL_ERROR );
	
}

void UserMemory::sync(sync_dir dir)
{
	int ret;
	umem_handle_t uh;

	/* We assume (C++ API) dir === (Driver API) dir */	
	uh.handle_id = handle_id;
	uh.vma = vma;
	uh.size = size;
	uh.dir = dir;
	
	ret = ioctl( device->getHandle(), PCIDRIVER_IOC_UMEM_SYNC, &uh );
	if (ret != 0)
		throw Exception( Exception::INTERNAL_ERROR );		
}
