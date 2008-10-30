
#include <vector>
#include "DMABuffer.h"
#include "DMAEngine.h"
#include "DMAEngineWG.h"
#include "DMADescriptor.h"
#include "DMADescriptorWG.h"
#include "DMADescriptorListWG.h"
#include "Driver.h"
#include "PCIDriver.h"
#include "pciDriver/lib/pciDriver.h"
#include <iostream>

using namespace mprace;
using namespace std;

const unsigned int DMAEngineWG::STATUS_BUSY = 0x00000002;
const unsigned int DMAEngineWG::STATUS_DONE = 0x00000001;
const unsigned int DMAEngineWG::CTRL_RESET = 0x0000000A;
const unsigned int DMAEngineWG::CTRL_END   = 0x00000100;
const unsigned int DMAEngineWG::CTRL_INC   = 0x00008000;
const unsigned int DMAEngineWG::CTRL_UPA   = 0x00100000;
const unsigned int DMAEngineWG::CTRL_LAST  = 0x01000000;
const unsigned int DMAEngineWG::CTRL_V     = 0x02000000;
const unsigned int DMAEngineWG::CTRL_EDI   = 0x10000000;
const unsigned int DMAEngineWG::CTRL_EEI   = 0x20000000;
const unsigned int DMAEngineWG::CTRL_ESEI  = 0x40000000;
const unsigned int DMAEngineWG::CTRL_BAR   = 0x00010000;	// new bitfiles!

const unsigned int DMAEngineWG::INTE_CH0   = 0x00000002;
const unsigned int DMAEngineWG::INTE_CH1   = 0x00000001;

const unsigned int DMAEngineWG::IRQ_SRC_CH0   = 0;
const unsigned int DMAEngineWG::IRQ_SRC_CH1   = 1;

DMAEngineWG::DMAEngineWG( Driver& drv, unsigned int *base0, unsigned int *base1, unsigned int *INTE, unsigned int *INTS )
	: DMAEngine(drv)
{
	// Setup base channel pointers
	channel[0] = base0;
	channel[1] = base1;

	// Setup Interrupt Register pointers
	inte = INTE;
	ints = INTS;

	// Reset both DMA channels
	reset(0);
	reset(1);
	
	// Default value for use Interrupts
	useInterrupts = false;
}

DMAEngineWG::~DMAEngineWG()
{
}

void DMAEngineWG::reset(const unsigned int ch)
{
	(channel[ch])[7] = CTRL_RESET | CTRL_V | CTRL_BAR;	
}

void DMAEngineWG::write(const unsigned int ch, const DMADescriptorWG& d)
{
	DMADescriptorWG::descriptor *desc = d.native;
	
	(channel[ch])[0] = desc->per_addr_h;
	(channel[ch])[1] = desc->per_addr_l;
	(channel[ch])[2] = desc->host_addr_h;
	(channel[ch])[3] = desc->host_addr_l;
	(channel[ch])[4] = desc->next_bda_h;
	(channel[ch])[5] = desc->next_bda_l;
	(channel[ch])[6] = desc->length;
	(channel[ch])[7] = desc->control;		// control is written at the end, starts DMA
}

DMAEngine::DMAStatus DMAEngineWG::getStatus(const unsigned int ch)
{
	// Read the status register of the channel
	unsigned long status = (channel[ch])[8];
	
	if ((status & STATUS_BUSY) && (status & STATUS_DONE))
		return ERROR;
	else if ( status & STATUS_BUSY )
		return BUSY;
	else
		return IDLE;
}

void DMAEngineWG::enableInterrupt(const unsigned int ch)
{
	unsigned int mask;
	
	switch (ch) {
		case 0:
			mask = INTE_CH0;
			break;
		case 1:
			mask = INTE_CH1;
			break;
	}
	
	*inte |= mask;
}

void DMAEngineWG::disableInterrupt(const unsigned int ch)
{
	unsigned int mask;
	
	switch (ch) {
		case 0:
			mask = !INTE_CH0;
			break;
		case 1:
			mask = !INTE_CH1;
			break;
	}
	
	*inte &= mask;
}

void DMAEngineWG::waitForInterrupt(const unsigned int ch)
{
	unsigned int src;
	
	switch (ch) {
	case 0:
		src = IRQ_SRC_CH0;
		break;
	case 1:
		src = IRQ_SRC_CH1;
		break;
	}
	
	drv->waitForInterrupt(src);
}

void DMAEngineWG::waitChannel(const unsigned int ch)
{
	if (useInterrupts) {
		waitForInterrupt(ch);
		disableInterrupt(ch);
	}
	else {
		unsigned long status = (channel[ch])[8];
	
		// wait for done signal
//		while ( status &  STATUS_BUSY ) {
		while ( getStatus(ch) !=  IDLE ) {
//		while (!( status & STATUS_DONE )) {
			status = (channel[ch])[8];
//			cout << "status : " << status << endl;
		}
	}
	
	// If a descriptor was saved, restore it to it correct position in the SG list.
	// This is only for UserMemory buffers
	if (saved[ch]) {
		restoreSavedData(ch);
		saved[ch] = false;		
	}
}

void DMAEngineWG::waitReadComplete()
{ 
   return waitChannel(1);
}
    
void DMAEngineWG::waitWriteComplete()
{ 
   return waitChannel(0);
   
}
  


void DMAEngineWG::restoreSavedData(unsigned int ch)
{
	DMADescriptorList *dlist = const_cast<DMADescriptorList *>(saved_data[ch].saved_buf->descriptors);
	DMADescriptorListWG& list = static_cast<DMADescriptorListWG&>(*dlist);
	
	unsigned int max_descriptor = list.getSize()-1;
	unsigned int index;
	
	if (saved_data[ch].saved_offset != 0) {
		// Restore initial descriptor
		index = saved_data[ch].saved_init_descriptor;
		list[index].setHostAddress( list[index].getHostAddress() - saved_data[ch].saved_offset );		
		list[index].setLength( list[index].getLength() + saved_data[ch].saved_offset );
	}		

	if ( saved_data[ch].saved_init_descriptor != max_descriptor )
		list[ saved_data[ch].saved_init_descriptor+1 ].setControl( CTRL_BAR );
		
	index = saved_data[ch].saved_last_descriptor;
	list[index].setLength( saved_data[ch].saved_length );
	list[index].setControl( CTRL_BAR );
	if (saved_data[ch].saved_last_descriptor == max_descriptor) {
		list[index].setNextDescriptorAddress( 0UL );
	}
	else {
		list[index].setNextDescriptorAddress( list[index+1].getPhysicalAddress() );
	}
}

void DMAEngineWG::fillDescriptorList(DMABuffer& buf)
{
	if (buf.getType() == DMABuffer::KERNEL) {
		// No descriptor list is needed for a Kernel buffer
		buf.descriptors = NULL;
	}

	if (buf.getType() == DMABuffer::KERNEL_PIECES) {
		DMADescriptorListWG *dlist = new DMADescriptorListWG(buf, DMADescriptorListWG::USER);
		buf.descriptors = dlist;
		DMADescriptorListWG& list = *dlist;

		unsigned long long base_ha = buf.kBuf->getPhysicalAddress();
		unsigned int step = buf.size() / buf.kernel_pieces;
		unsigned int count = 0;
		
		for(int i=0 ; i<buf.kernel_pieces ; i++) {
			DMADescriptorWG *descriptor;
			descriptor = &(list[i]);

			descriptor->setHostAddress( base_ha + i*step );
			descriptor->setLength( (i == (buf.kernel_pieces-1)) ? buf.size()-count : step );
			descriptor->setControl( CTRL_BAR );
			descriptor->setPeripheralAddress(0UL);
			descriptor->setNextDescriptorAddress( (i == (buf.kernel_pieces-1)) ? 0UL : list[i+1].getPhysicalAddress() );
			count += step;			
		}
	}
	
	
	if (buf.getType() == DMABuffer::USER) {
		DMADescriptorListWG *dlist = new DMADescriptorListWG(buf, DMADescriptorListWG::USER);
		buf.descriptors = dlist;
		DMADescriptorListWG& list = *dlist;
		
		// Set the pointer to the next descriptor
		for( int i=0 ; i < buf.uBuf->getSGcount() ; i++ ) {
			DMADescriptorWG *descriptor;
			descriptor = &list[i];
			
			descriptor->setHostAddress( buf.uBuf->getSGentryAddress(i) );
			descriptor->setLength( buf.uBuf->getSGentrySize(i) );
			descriptor->setControl( CTRL_BAR );
			descriptor->setPeripheralAddress(0UL);
			descriptor->setNextDescriptorAddress( (i == (buf.uBuf->getSGcount()-1)) ? 0UL : list[i+1].getPhysicalAddress() );
		}		
	}
}

void DMAEngineWG::releaseDescriptorList(DMABuffer& buf)
{
	// Release the Kernel Buffer associated with the Descriptor List of the buffer
	if ((buf.getType() == DMABuffer::USER) || (buf.getType() == DMABuffer::KERNEL_PIECES)) {
		delete buf.descriptors;
	}	
}

void DMAEngineWG::host2board(const unsigned int addr, const DMABuffer& buf, const unsigned int count, const unsigned int offset, const bool inc, const bool lock)
{
	this->reset(0);

#ifndef OLD_REGISTERS
	if (useInterrupts)
		this->enableInterrupt(0);
	else
		this->disableInterrupt(0);
#endif
	
	if (buf.getType() == DMABuffer::KERNEL) {
		// Single descriptor, easy
		pciDriver::KernelMemory *kb = buf.kBuf;
		DMADescriptorWG d;
		unsigned long control;
		
		d.setHostAddress( kb->getPhysicalAddress() + offset );
		d.setPeripheralAddress(addr*4);
		d.setNextDescriptorAddress(0L);
		d.setLength(count*4);
		control = CTRL_LAST | CTRL_BAR;
#ifndef OLD_REGISTERS
		control |= CTRL_UPA | CTRL_V;
		control |= (useInterrupts) ? CTRL_EDI : 0x0;
#endif		
		control |= (inc) ? CTRL_INC : 0x0;
		d.setControl(control);
		
		saved[0] = false;

		// host2board is channel 0		
		this->write(0,d);
	}

	if ((buf.getType() == DMABuffer::USER) || (buf.getType() == DMABuffer::KERNEL_PIECES)) {
		sendDescriptorList(addr,buf,count,offset,inc,0);
	}
		
	if (lock)
		this->waitChannel(0);
}

void DMAEngineWG::board2host(const unsigned int addr, DMABuffer& buf, const unsigned int count, const unsigned int offset, const bool inc, const bool lock)
{
	this->reset(1);

#ifndef OLD_REGISTERS
	if (useInterrupts)
		this->enableInterrupt(1);
	else
		this->disableInterrupt(1);
#endif

	if (buf.getType() == DMABuffer::KERNEL) {
		// Single descriptor, easy
		pciDriver::KernelMemory *kb = buf.kBuf;
		DMADescriptorWG d;
		unsigned long control;
		
		d.setPeripheralAddress(addr*4);
		d.setHostAddress( kb->getPhysicalAddress() + offset );
		d.setNextDescriptorAddress(0);
		d.setLength(count*4);
		control = CTRL_LAST | CTRL_BAR;
#ifndef OLD_REGISTERS
		control |= CTRL_UPA | CTRL_V;
		control |= (useInterrupts) ? CTRL_EDI : 0x0;
#endif		
		control |= (inc) ? CTRL_INC : 0x0;
		d.setControl(control);
		
		saved[1] = false;
		
		// board2host is channel 1
		this->write(1,d);
	}

	if ((buf.getType() == DMABuffer::USER) || (buf.getType() == DMABuffer::KERNEL_PIECES)) {
		sendDescriptorList(addr,buf,count,offset,inc,1);
	}
			
	if (lock)
		this->waitChannel(1);
}

void DMAEngineWG::sendDescriptorList(const unsigned int addr, const DMABuffer& buf, const unsigned int count, const unsigned int offset, const bool inc, const unsigned int ch)
{
	pciDriver::UserMemory *uBuf = buf.uBuf;

	// get the list of descriptors for this buffer
	DMADescriptorList *dlist = buf.descriptors;
	DMADescriptorListWG& list = *(static_cast<DMADescriptorListWG*>(dlist));
	
	unsigned int nr_descriptors = list.getSize();
	
	unsigned int init_descriptor,last_descriptor;
	unsigned int control_word;
	DMADescriptorWG temp;
		
	control_word = CTRL_V | CTRL_BAR;
	control_word |= (inc) ? CTRL_INC : 0x0;
	#ifdef OLD_REGISTERS		
		control_word |= (useInterrupts) ? CTRL_EDI : 0x0;
	#endif

	// Check first for the fast and easy conditions
	// Optimize for the fast case
	if ((offset==0) && ((count*4)==buf.size())) {
		// set first, second and last descriptors
		last_descriptor = nr_descriptors-1;
			
		if (nr_descriptors > 2)
			list[1].setControl( control_word );

		list[last_descriptor].setControl( control_word | CTRL_LAST );
			
		saved_data[ch].saved_last_descriptor = last_descriptor;
		saved_data[ch].saved_length = list[last_descriptor].getLength();
		saved_data[ch].saved_offset = 0;
		saved_data[ch].saved_init_descriptor = 0;
		saved_data[ch].saved_buf = &buf;
		saved[ch] = true;
			
		// write first descriptor
		temp = list[0];
		temp.setPeripheralAddress( addr*4 );
		temp.setControl( (control_word | CTRL_UPA) | ((nr_descriptors == 1) ? CTRL_LAST : 0x0) );

		//list.print();
		
		// Send
		this->write(ch,temp);
	} // end fast condition: offset=0, length=max
	else {
		// We need either to apply an offset, end the buffer before its limit, or both.
		// get to the starting point
		unsigned int init_descriptor = 0;
		unsigned int byte_count=0;
		unsigned int byte_offset = offset*4;
		unsigned int block_offset=0;

		if (offset != 0) {
			while (init_descriptor < nr_descriptors) {
				if (byte_offset > byte_count + list[init_descriptor].getLength()) {
					byte_count += list[init_descriptor].getLength();
					init_descriptor++;
				} else {
					// we found the starting block (init_descriptor)
					// calculate the offset inside the block
					block_offset = byte_offset - byte_count; 
					break;
				}
			}
		}

		// Now, we walk the list until we match the required number of bytes
		unsigned int last_descriptor = init_descriptor;
				
		byte_count=count*4;
		while( last_descriptor < nr_descriptors ) {							
			if (byte_count <= list[last_descriptor].getLength()) {
				// save the current descriptor
				// we are interested only in the descriptor number and the length.
				saved_data[ch].saved_length = list[last_descriptor].getLength();
				saved_data[ch].saved_offset = block_offset;
				saved_data[ch].saved_init_descriptor = init_descriptor;
				saved_data[ch].saved_last_descriptor = last_descriptor;
				saved_data[ch].saved_buf = &buf;
				saved[ch] = true;

				// set this as truly the last descriptor
				list[last_descriptor].setLength( byte_count );
				list[last_descriptor].setNextDescriptorAddress( 0L );
				list[last_descriptor].setControl( control_word | CTRL_LAST );
				break;
			} else {
				byte_count -= list[last_descriptor].getLength();
				last_descriptor++;
			}
		}

		// Adjust control word of second descriptor
		if (init_descriptor+1 < last_descriptor)
			list[ init_descriptor+1 ].setControl( control_word );
			
		temp = list[init_descriptor];
		temp.setHostAddress( temp.getHostAddress() + block_offset );
		temp.setLength( temp.getLength() - block_offset );
		temp.setPeripheralAddress( addr*4 );
		temp.setControl( (control_word | CTRL_UPA) | ((init_descriptor == last_descriptor) ? CTRL_LAST : 0x0) );

		//list.print();
		
		// Send
		this->write(ch,temp);
	} // End conditional cases
}
