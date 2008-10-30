/*******************************************************************
 * Change History:
 * 
 * $Log: PciDevice.cpp,v $
 * Revision 1.6  2008/04/07 16:09:06  adamczew
 * *** empty log message ***
 *
 * Revision 1.5  2008-01-24 14:21:35  marcus
 * Added a CLEAR_INTERRUPT_QUEUE ioctl.
 * Added a sysfs attribute to show the outstanding IRQ queues.
 *
 * Revision 1.4  2008-01-11 10:15:59  marcus
 * Added intSource to the Wait for Interrupt call.
 *
 * Revision 1.3  2007/02/09 17:02:39  marcus
 * Modified Exception handling, made simpler and more standard.
 *
 * Revision 1.2  2006/11/17 18:59:57  marcus
 * Fixed offset when mmaping a BAR with a not page-aligned address.
 * Added support for SGlist types at runtime.
 *
 * Revision 1.1  2006/10/13 17:18:31  marcus
 * Implemented and tested most of C++ interface.
 *
 *******************************************************************/


#include "driver/pciDriver.h"
#include "PciDevice.h"
#include "Exception.h"
#include "KernelMemory.h"
#include "UserMemory.h"

#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>

using namespace pciDriver;

PciDevice::PciDevice(int number)
{
	struct stat tmp_stat;
	int ret;
	
	unsigned int temp;
	
	device = number;
	sprintf( name, "/dev/fpga%d", number );

	ret = stat( name, &tmp_stat );
	if (ret < 0)
		throw Exception( Exception::DEVICE_NOT_FOUND );

	pthread_mutex_init( &mmap_mutex, NULL );

	handle = -1;		

	pagesize = getpagesize();

	// set pagemask and pageshift
	for( pagemask=0, pageshift=0, temp = pagesize; temp != 1; pageshift++ ) {
		temp = (temp >> 1);
		pagemask = (pagemask << 1)+1;
	}

}

PciDevice::~PciDevice()
{
	// Close device if open
	if (handle > 0)
		this->close();
	
	pthread_mutex_destroy( &mmap_mutex );
}

int PciDevice::getHandle()
{
	if (handle == -1)
		throw Exception( Exception::NOT_OPEN );
		
	return handle;
}

void PciDevice::open()
{
	int ret;

	// device already open
	if (handle != -1)
		return;
	
	ret = ::open( name, O_RDWR );
	if (ret < 0)
		throw Exception( Exception::OPEN_FAILED );
		
	handle = ret;
}

void PciDevice::close()
{
	// do nothing, pass silently if closing a non-opened device.
	if (handle != -1)	
		::close( handle );
	
	handle = -1;
}

KernelMemory& PciDevice::allocKernelMemory( unsigned int size )
{
	KernelMemory *km = new KernelMemory( *this, size );
	
	return *km;
}

UserMemory& PciDevice::mapUserMemory( void *mem, unsigned int size, bool merged )
{
	UserMemory *um = new UserMemory( *this, mem, size, merged );
	
	return *um;
}

	
void PciDevice::waitForInterrupt(unsigned int int_id)
{
	int ret;

	if (handle == -1)
		throw Exception( Exception::NOT_OPEN );
	
	ret = ioctl( handle, PCIDRIVER_IOC_WAITI, int_id );
	if (ret != 0)
		throw Exception( Exception::INTERRUPT_FAILED );
		
	return;
	
}

void PciDevice::clearInterruptQueue(unsigned int int_id)
{
	int ret;

	if (handle == -1)
		throw Exception( Exception::NOT_OPEN );
	
	ret = ioctl( handle, PCIDRIVER_IOC_CLEAR_IOQ, int_id );
	if (ret != 0)
		throw Exception( Exception::INTERNAL_ERROR );
		
	return;
	
}

unsigned int PciDevice::getBARsize(unsigned int bar)
{
	int ret;
	pci_board_info info;
	unsigned int id;

	if (handle == -1)
		throw Exception( Exception::NOT_OPEN );
		
	if (bar > 5)
		throw Exception( Exception::INVALID_BAR );
		
	ret = ioctl( handle, PCIDRIVER_IOC_PCI_INFO, &info );
	if (ret != 0)
		throw Exception( Exception::INTERNAL_ERROR );
		
	return info.bar_length[ bar ];	
}

unsigned short PciDevice::getBus() 
{
	int ret;
	pci_board_info info;

	if (handle == -1)
		throw Exception( Exception::NOT_OPEN );

	ret = ioctl( handle, PCIDRIVER_IOC_PCI_INFO, &info );
	if (ret != 0)
		throw Exception( Exception::INTERNAL_ERROR );

	return info.bus;
}

unsigned short PciDevice::getSlot()
{
	int ret;
	pci_board_info info;

	if (handle == -1)
		throw Exception( Exception::NOT_OPEN );

	ret = ioctl( handle, PCIDRIVER_IOC_PCI_INFO, &info );
	if (ret != 0)
		throw Exception( Exception::INTERNAL_ERROR );

	return info.slot;
}

void *PciDevice::mapBAR(unsigned int bar)
{
	int ret;
	void *mem;
	pci_board_info info;

	if (handle == -1)
		throw Exception( Exception::NOT_OPEN );
		
	if (bar > 5)
		throw Exception( Exception::INVALID_BAR );

	ret = ioctl( handle, PCIDRIVER_IOC_PCI_INFO, &info );
	if (ret != 0)
		return NULL;

	/* Mmap */
	/* This is not fully safe, as a separate process can still open the device independently.
	 * That will use a separate mutex and the race condition can arise.
	 * Posible fix: Do not allow the driver for mutliple openings of a device */
	mmap_lock();

	ret = ioctl( handle, PCIDRIVER_IOC_MMAP_MODE, PCIDRIVER_MMAP_PCI );
	if (ret != 0)
		throw Exception( Exception::INTERNAL_ERROR );

	ret = ioctl( handle, PCIDRIVER_IOC_MMAP_AREA, PCIDRIVER_BAR0+bar );
	if (ret != 0)
		throw Exception( Exception::INTERNAL_ERROR );

	mem = mmap( 0, info.bar_length[bar], PROT_WRITE | PROT_READ, MAP_SHARED, handle, 0 );
	
	mmap_unlock();

	if ((mem == MAP_FAILED) || (mem == NULL))
		throw Exception( Exception::MMAP_FAILED );

	// check if the BAR is not page aligned. If it is not, adjust the pointer
	unsigned int offset = info.bar_start[bar] & pagemask;
	
	// adjust pointer
	if (offset != 0) {
		unsigned char* ptr = static_cast<unsigned char *>(mem);
		ptr += offset;
		mem = static_cast<void *>(ptr);
	}
		
	return mem;
}

void PciDevice::unmapBAR(unsigned int bar, void *ptr)
{
	int ret;
	pci_board_info info;

	if (handle == -1)
		throw Exception( Exception::NOT_OPEN );

	if (bar > 5)
		throw Exception( Exception::INVALID_BAR );

	ret = ioctl( handle, PCIDRIVER_IOC_PCI_INFO, &info );
	if (ret != 0)
		throw Exception( Exception::INVALID_BAR );


	unsigned int offset = info.bar_start[bar] & pagemask;
	
	// adjust pointer
	if (offset != 0) {
		unsigned long tmp = reinterpret_cast<unsigned long>(ptr);
		tmp -= offset;
		ptr = reinterpret_cast<void *>(tmp);
	}

	munmap( ptr, info.bar_length[bar] );
}
	
unsigned char PciDevice::readConfigByte(unsigned int addr)
{
	pci_cfg_cmd cmd;

	if (handle == -1)
		throw new Exception( Exception::NOT_OPEN );

	cmd.addr = addr;	
	cmd.size = PCIDRIVER_PCI_CFG_SZ_BYTE;
	ioctl( handle, PCIDRIVER_IOC_PCI_CFG_RD, &cmd );
	
	return cmd.val.byte;
}

unsigned short PciDevice::readConfigWord(unsigned int addr)
{
	pci_cfg_cmd cmd;

	if (handle == -1)
		throw Exception( Exception::NOT_OPEN );

	cmd.addr = addr;	
	cmd.size = PCIDRIVER_PCI_CFG_SZ_WORD;
	ioctl( handle, PCIDRIVER_IOC_PCI_CFG_RD, &cmd );
	
	return cmd.val.word;
}

unsigned int PciDevice::readConfigDWord(unsigned int addr)
{
	pci_cfg_cmd cmd;

	if (handle == -1)
		throw Exception( Exception::NOT_OPEN );

	cmd.addr = addr;	
	cmd.size = PCIDRIVER_PCI_CFG_SZ_DWORD;
	ioctl( handle, PCIDRIVER_IOC_PCI_CFG_RD, &cmd );
	
	return cmd.val.dword;
}
	
void PciDevice::writeConfigByte(unsigned int addr, unsigned char val)
{
	pci_cfg_cmd cmd;

	if (handle == -1)
		throw Exception( Exception::NOT_OPEN );

	cmd.addr = addr;	
	cmd.size = PCIDRIVER_PCI_CFG_SZ_BYTE;
	cmd.val.byte = val;
	ioctl( handle, PCIDRIVER_IOC_PCI_CFG_WR, &cmd );
	
	return;
}

void PciDevice::writeConfigWord(unsigned int addr, unsigned short val)
{
	pci_cfg_cmd cmd;

	if (handle == -1)
		throw Exception( Exception::NOT_OPEN );

	cmd.addr = addr;	
	cmd.size = PCIDRIVER_PCI_CFG_SZ_WORD;
	cmd.val.word = val;
	ioctl( handle, PCIDRIVER_IOC_PCI_CFG_WR, &cmd );
	
	return;
}

void PciDevice::writeConfigDWord(unsigned int addr, unsigned int val)
{
	pci_cfg_cmd cmd;

	if (handle == -1)
		throw Exception( Exception::NOT_OPEN );

	cmd.addr = addr;	
	cmd.size = PCIDRIVER_PCI_CFG_SZ_DWORD;
	cmd.val.dword = val;
	ioctl( handle, PCIDRIVER_IOC_PCI_CFG_WR, &cmd );
	
	return;
}
