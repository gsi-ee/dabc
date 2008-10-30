#ifndef MPRACE2_H_
#define MPRACE2_H_

/*******************************************************************
 * Change History:
 * 
 * $Log: MPRACE2.h,v $
 * Revision 1.4  2008/04/07 16:09:02  adamczew
 * *** empty log message ***
 *
 * Revision 1.2  2007-03-02 14:58:25  marcus
 * DMAEngineWG basic functionality working.
 *
 * Revision 1.1  2007/02/12 18:09:17  marcus
 * Initial commit.
 *
 *******************************************************************/

#include "DMAEngineWG.h"

namespace mprace {

class DMAEngineWG;

/**
 * Implements the library interface for the MPRACE-2.
 * The address space provides access to the Main FPGA only. All other
 * memory areas must be accessed thru the specific device interface
 * associated with it.
 * 
 * @author  Guillermo Marcus
 * @version $Revision: 1.4 $
 * @date    $Date: 2008/04/07 16:09:02 $
 */
class MPRACE2 : public Board {
public:
	/**
	 * Probes a Driver if it handles an MPRACE2 Board.
	 * @param device The device to probe.
	 * @return true if is an MPRACE2 board, false otherwise.
	 */
	static bool probe(Driver& dev);

	/**
	 * Creates an MPRACE2 board object.
	 * @param number The number of the board to initialize
	 * @todo How are boards enumerated?
	 */
	MPRACE2(const unsigned int number);

	/**
	 * Releases a board.
	 */
	virtual ~MPRACE2();
	
	/**
	 * Write a value to the board address space.
	 * 
	 * @param address Address in the board.
	 * @param value Value to write.
	 * @exception mprace::Exception On error.
	 */
	inline void write(unsigned int address, unsigned int value);
	
	/**
	 * Read a value from the board address space.
	 * 
	 * @param address Address in the board.
	 * @return The value read from the board.
	 * @exception mprace::Exception On error.
	 */
	inline unsigned int read(unsigned int address);

	/**
	 * Write multiple values to the board address space.
	 * 
	 * @param address Address in the board.
	 * @param data Values to write.
	 * @param count Number of values to write, in dwords.
	 * @param inc Increment the address on the FPGA side (default: true).
	 * @exception mprace::Exception On error.
	 */
	inline void writeBlock(const unsigned int address, const unsigned int *data, const unsigned int count, const bool inc = true );

	/**
	 * Read multiple values from the board address space.
	 * 
	 * @param address Address in the board.
	 * @param data An array where to read values into.
	 * @param count Number of values to read from the board's address space, in dwords.
	 * @param inc Increment the address on the FPGA side (default: true).
	 * @exception mprace::Exception On error.
	 */
	inline void readBlock(const unsigned int address, unsigned int *data, const unsigned int count, const bool inc = true );

	/**
	 * Write multiple values from a DMA Buffer to the board address space.
	 * 
	 * @param address Address in the board.
	 * @param data Values to write.
	 * @param count Number of values to write, in dwords.
	 * @param offset Offset in the DMA buffer, in dwords (default = 0).
	 * @param inc Increment the address on the FPGA side (default: true).
	 * @param lock Lock until the write finishes (default: true)
	 * @exception mprace::Exception On error.
	 */
	inline void writeDMA(const unsigned int address, const DMABuffer& buf, const unsigned int count, const unsigned int offset, const bool inc = true, const bool lock=true );

	/**
	 * Read multiple values from the board address space to a DMA Buffer.
	 * 
	 * @param address Address in the board.
	 * @param data An array where to read values into.
	 * @param count Number of values to read from the board's address space, in dwords.
	 * @param offset Offset in the DMA buffer, in dwords (default = 0).
	 * @param inc Increment the address on the FPGA side (default: true).
	 * @param lock Lock until the write finishes (default: true)
	 * @exception mprace::Exception On error.
	 */
	inline void readDMA(const unsigned int address, DMABuffer& buf, const unsigned int count, const unsigned int offset, const bool inc = true, const bool lock=true );

	/**
	 * Get the DMA Engine for this board.
	 */
	virtual inline DMAEngine& getDMAEngine() { return *dma; }

protected:
	const static unsigned int DMA0_BASE;
	const static unsigned int DMA1_BASE;

	unsigned int *bridge;
	unsigned int bridge_size;	/** Size of the Bridge area, in 32-bit words */
	
	unsigned int *main;
	unsigned int main_size;		/** Size of the Main area, in 32-bit words */

	DMAEngineWG *dma;			/** The DMAEngine of this board */
	
	/* Avoid copy constructor, and copy assignment operator */
	
	/**
	 * Overrides default copy constructor, does nothing.
	 */
	MPRACE2(const MPRACE2&) {};

	/**
	 * Overrides default assignment operation, does nothing.
	 */
	MPRACE2& operator=(const MPRACE2&) { return *this; };
}; /* class MPRACE2 */

} /* namespace mprace */

#endif /*MPRACE2_H_*/
