#ifndef BRIDGE_H_
#define BRIDGE_H_

/*******************************************************************
 * Change History:
 * 
 * $Log: Bridge.h,v $
 * Revision 1.4  2008/04/07 16:09:01  adamczew
 * *** empty log message ***
 *
 * Revision 1.1  2007-02-12 18:09:17  marcus
 * Initial commit.
 *
 *******************************************************************/

namespace mprace {

/**
 * Abstract class to group functions present in a bridge chip.
 * Typically, a bridge chip can be an ASIC or an FPGA containing
 * support logic to interface a board to a host PC. This includes a
 * PCI, PCI-X or PCIe interface, a DMA Engine, and other functionality.
 * 
 * @author  Guillermo Marcus
 * @version $Revision: 1.4 $
 * @date    $Date: 2008/04/07 16:09:01 $
 */
class Bridge {
public:
}; /* class Bridge */

} /* namespace mprace */

#endif /*BRIDGE_H_*/
