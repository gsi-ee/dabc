/*******************************************************************
 * Change History:
 * 
 * $Log: MPRACE2.cpp,v $
 * Revision 1.4  2008/04/07 16:09:04  adamczew
 * *** empty log message ***
 *
 * Revision 1.3  2007-07-06 19:20:36  marcus
 * New Register map for the ABB.
 * User Memory Support with Scatter/Gather Lists.
 *
 * Revision 1.2  2007-03-02 14:58:23  marcus
 * DMAEngineWG basic functionality working.
 *
 * Revision 1.1  2007/02/12 18:09:15  marcus
 * Initial commit.
 *
 *******************************************************************/

#include "Board.h"
#include "MPRACE2.h"
#include "Exception.h"
#include "Logger.h"
#include "Driver.h"
#include "PCIDriver.h"
#include "DMABuffer.h"
#include "DMAEngineWG.h"

using namespace mprace;

const unsigned int MPRACE2::DMA0_BASE = 0x0;
const unsigned int MPRACE2::DMA1_BASE = 0x0;


bool MPRACE2::probe(Driver& dev) {
}

MPRACE2::MPRACE2(const unsigned int number) {
	// We need to open the device, map the BARs.

	try {
		// TODO: get the device number from the MPRACE2 board number
		driver = new PCIDriver(number);
	
		// The board has 2 BARs. BAR0 is the bridge, BAR1 is the Main FPGA.
		bridge = static_cast<unsigned int *>( driver->mmapArea(0) );
		main = static_cast<unsigned int *>( driver->mmapArea(1) );
		
		// The driver returns the size in bytes, we convert it to words
		bridge_size = (driver->getAreaSize(0) >> 2);
		main_size = (driver->getAreaSize(1) >> 2);
		
		// Initialize the DMAEngine
		dma = new DMAEngineWG( *driver, bridge+DMA0_BASE, bridge+DMA1_BASE, 0x0, 0x0 );
	} catch (...) {
		throw Exception( Exception::UNKNOWN );
	}
}

MPRACE2::~MPRACE2() {
	// Unmap the BARs, close the device
	driver->unmapArea(0);
	driver->unmapArea(1);
	delete dma;
	delete driver;
}

void MPRACE2::write(unsigned int address, unsigned int value) {

	if (address < main_size)
		*(main+address) = value;
	else
		throw Exception( Exception::ADDRESS_OUT_OF_RANGE );	
	
	if (log != 0)
		log->write(address,value);
		
	return;	
}

unsigned int MPRACE2::read(unsigned int address) {
	unsigned int value;
	
	if (address < main_size)
		value = *(main+address);
	else
		throw Exception( Exception::ADDRESS_OUT_OF_RANGE );	
	
	if (log != 0)
		log->read(address,value);
		
	return value;	
}

void MPRACE2::writeBlock(const unsigned int address, const unsigned int *data, const unsigned int count, const bool inc) {
	unsigned int i,j;

	// Check address range
	if ((address < main_size) && ((address+count) < main_size)) {
		/* for performance, we take the comparison out of the loop, 
		 * and repeat the code. 
		 */
		if (inc) {
			for( i=0; i<count ; i++ )
				*(main+address+i) = *(data+i);
		} else {
			for( i=0; i<count ; i++ )
				*(main+address) = *(data+i);
		}
	}
	else
		throw Exception( Exception::ADDRESS_OUT_OF_RANGE );	
	
	if (log != 0)
		log->writeBlock(address,data,count);

}

void MPRACE2::readBlock(const unsigned int address, unsigned int *data, const unsigned int count, const bool inc) {
	unsigned int i,j;

	if ((address < main_size) && ((address+count) < main_size)) {
		/* for performance, we take the comparison out of the loop, 
		 * and repeat the code. 
		 */
		if (inc) {
			for( i=0 ; i<count ; i++ )
				*(data+i) = *(main+address+i);
		} else {
			for( i=0 ; i<count ; i++ )
				*(data+i) = *(main+address);
		}
	}
	else
		throw Exception( Exception::ADDRESS_OUT_OF_RANGE );	

	if (log != 0)
		log->readBlock(address,data,count);
}

void MPRACE2::writeDMA(const unsigned int address, const DMABuffer& buf, const unsigned int count, const unsigned int offset, const bool inc, const bool lock ) {
	dma->host2board(address,buf,count,offset,inc,lock);
}

void MPRACE2::readDMA(const unsigned int address, DMABuffer& buf, const unsigned int count, const unsigned int offset, const bool inc, const bool lock ) {
	dma->board2host(address,buf,count,offset,inc,lock);
}
