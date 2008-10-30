#ifndef DMAENGINE_H_
#define DMAENGINE_H_

/*******************************************************************
 * Change History:
 * 
 * $Log: DMAEngine.h,v $
 * Revision 1.4  2008/04/07 16:09:02  adamczew
 * *** empty log message ***
 *
 * Revision 1.4  2007-05-29 07:50:48  marcus
 * Backup commit.
 *
 * Revision 1.3  2007/03/13 16:16:29  marcus
 * Added a virtual method to release the descriptor list. It is to be paired with fillDescriptorList, and clean the work done by it before releasing the DMABuffer.
 *
 * Revision 1.2  2007/03/02 14:58:24  marcus
 * DMAEngineWG basic functionality working.
 *
 *******************************************************************/

namespace mprace {

class Driver;
class DMABuffer;

/**
 * Abstract interface for handling DMA operations. 
 * 
 * @author  Guillermo Marcus
 * @version $Revision: 1.4 $
 * @date    $Date: 2008/04/07 16:09:02 $
 */
class DMAEngine {
public:
	/**
	 * Possible status of a DMA channel
	 */
	enum DMAStatus {
		IDLE,	//>** Channel is free
		BUSY,	//>** Channel is in use
		ERROR	//>** An error has ocurred in the channel
	};
	
	virtual ~DMAEngine() {}
	
	/**
	 * Get the current status of a DMA channel.
	 * 
	 * @param channel The channel to query
	 */
	virtual DMAStatus getStatus(const unsigned int channel)=0;

	/**
	 * Reset the status of a DMA channel.
	 * 
	 * @param channel The channel to reset
	 */
	virtual void reset(const unsigned int channel)=0;

	/**
	 * Performs a DMA transaction from host to board.
	 * 
	 * @param addr   The address in the board.
	 * @param buf    The DMA buffer in the host.
	 * @param count  Number of bytes to transfer.
	 * @param offset Initial dword to transfer.
	 * @param inc    If the address in the board is incremented or not.
	 * @param lock   If the function waits until the transaction is complete or not.
	 */
	virtual void host2board(const unsigned int addr, const DMABuffer& buf, const unsigned int count, const unsigned int offset=0, const bool inc=true, const bool lock=true )=0;

	/**
	 * Performs a DMA transaction from board to host.
	 * 
	 * @param addr   The address in the board.
	 * @param buf    The DMA buffer in the host.
	 * @param count  Number of bytes to transfer.
	 * @param offset Initial dword to transfer.
	 * @param inc    If the address in the board is incremented or not.
	 * @param lock   If the function waits until the transaction is complete or not.
	 */
	virtual void board2host(const unsigned int addr, DMABuffer& buf, const unsigned int count, const unsigned int offset=0, const bool inc=true, bool lock=true )=0;


	/**
	 * Fills the Descriptor List of the the DMA Buffer for the current engine.
	 * 
	 * @param buf The DMA Buffer.
	 */
	virtual void fillDescriptorList(DMABuffer& buf)=0;

	/**
	 * Release the Descriptor List of the the DMA Buffer for the current engine.
	 * 
	 * @param buf The DMA Buffer.
	 */
	virtual void releaseDescriptorList(DMABuffer& buf)=0;
protected:
	/**
	 * Creates a DMAEngine. Protected because only subclasses should 
	 * be instantiated.
	 */
	DMAEngine(Driver& driver) : drv(&driver) { }

	/* Avoid copy constructor, and copy assignment operator */

	/**
	 * Overrides default copy constructor, does nothing.
	 */
	DMAEngine(const DMAEngine&) {}

	/**
	 * Overrides default assignment operation, does nothing.
	 */
	DMAEngine& operator=(const DMAEngine&) { return *this; }
	
	Driver *drv;
	
}; /* class DMAEngine */

} /* namespace mprace */


#endif /*DMAENGINE_H_*/
