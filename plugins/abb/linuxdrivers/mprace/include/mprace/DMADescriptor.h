#ifndef DMADESCRIPTOR_H_
#define DMADESCRIPTOR_H_

/*******************************************************************
 * Change History:
 * 
 * $Log: DMADescriptor.h,v $
 * Revision 1.2  2008/04/07 16:09:01  adamczew
 * *** empty log message ***
 *
 * Revision 1.2  2008-03-20 13:26:56  marcus
 * Added support for DMA descriptors to be in user memory.
 *
 *******************************************************************/

namespace mprace {

/**
 * Abstract class to group all DMADescriptor subclasses. 
 * 
 * @author  Guillermo Marcus
 * @version $Revision: 1.2 $
 * @date    $Date: 2008/04/07 16:09:01 $
 */
class DMADescriptor {
public:
	virtual ~DMADescriptor() {}
protected:
	DMADescriptor() {}
};

}

#endif /*DMADESCRIPTOR_H_*/
