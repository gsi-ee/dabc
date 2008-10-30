/*
 * Change History:
 * 
 * $Log: pciDriver_int.c,v $
 * Revision 1.8  2008/04/07 16:09:05  adamczew
 * *** empty log message ***
 *
 * Revision 1.8  2008-01-24 12:51:34  marcus
 * Removed DMA reset from int. routine (must be in library).
 *
 * Revision 1.7  2008-01-11 10:18:28  marcus
 * Modified interrupt mechanism. Added atomic functions and queues, to address race conditions. Removed unused interrupt code.
 *
 * Revision 1.6  2007-11-04 20:58:22  marcus
 * Added interrupt generator acknowledge.
 * Fixed wrong operator.
 *
 * Revision 1.5  2007-10-31 15:42:21  marcus
 * Added IG ack for testing, may be removed later.
 *
 * Revision 1.4  2007-07-17 13:15:56  marcus
 * Removed Tasklets.
 * Using newest map for the ABB interrupts.
 *
 * Revision 1.3  2007-07-05 15:30:30  marcus
 * Added support for both register maps of the ABB.
 *
 * Revision 1.2  2007-05-29 07:50:18  marcus
 * Split code into 2 files. May get merged in the future again....
 *
 * Revision 1.1  2007/03/01 16:57:43  marcus
 * Divided driver file to ease the interrupt hooks for the user of the driver.
 * Modified Makefile accordingly.
 *
 */

/*
 * To separate the structures from the main driver code will complicate things
 * unneccesarily. So we just include this file in the main and continue as usual.
 */

/*
 * The ID between IRQ_SOURCE in irq_outstanding and the actual source is arbitrary.
 * Therefore, be careful when communicating with multiple implementations. 
 */

/* IRQ_SOURCES */
#define ABB_IRQ_CH0 0
#define ABB_IRQ_CH1 1
#define ABB_IRQ_IG 2

#define ABB_INT_CTRL (0x0010 >> 2)
#define ABB_INT_STAT (0x0008 >> 2)
#define ABB_INT_IG  (0x00000004)
#define ABB_INT_CH0 (0x00000002)
#define ABB_INT_CH1 (0x00000001)
#define ABB_CH0_CTRL  (108 >> 2)
#define ABB_CH1_CTRL  (72 >> 2)
#define ABB_CH_RESET (0x0201000A)
#define ABB_IG_CTRL (0x0080 >> 2)
#define ABB_IG_ACK (0x00F0)

int pcidriver_irq_acknowledge(pcidriver_privdata_t *privdata)
{
	volatile unsigned int *bar;
	
	/*
 	 * test if we must handle the interrupt
 	 * if true, acknowledge to the device appropiately.
 	 * if not, return 0
 	 */
	if ((privdata->pdev->vendor == PCIEABB_VENDOR_ID) &&
		(privdata->pdev->device == PCIEABB_DEVICE_ID))
		/* FIXME: add subvendor / subsystem ids */ 
	{
		/* this is for ABB / wenxue DMA engine */
		bar = privdata->bars_kmapped[0]; /* get BAR0 */

		mod_info_dbg("interrupt registers. ISR: %x, IER: %x\n", bar[ ABB_INT_STAT ], bar[ ABB_INT_CTRL ] );

		/* check & acknowledge channel 0 */
		if (bar[ ABB_INT_STAT ] & ABB_INT_CH0) {
			bar[ ABB_INT_CTRL ] &= !ABB_INT_CH0;
//			bar[ ABB_CH0_CTRL ] = ABB_CH_RESET;
			atomic_inc( &(privdata->irq_outstanding[ ABB_IRQ_CH0 ]) );
			wake_up_interruptible( &(privdata->irq_queues[ ABB_IRQ_CH0 ]) );
			return 1;
		}

		/* check & acknowledge channel 1 */
		if (bar[ ABB_INT_STAT ] & ABB_INT_CH1) {
			bar[ ABB_INT_CTRL ] &= !ABB_INT_CH1;
//			bar[ ABB_CH1_CTRL ] = ABB_CH_RESET;
			atomic_inc( &(privdata->irq_outstanding[ ABB_IRQ_CH1 ]) );
			wake_up_interruptible( &(privdata->irq_queues[ ABB_IRQ_CH1 ]) );
			return 1;
		}

		/* check & acknowledge interrupt generator */
		if (bar[ ABB_INT_STAT ] & ABB_INT_IG) {
			bar[ ABB_INT_CTRL ] &= !ABB_INT_IG;
			bar[ ABB_IG_CTRL ] = ABB_IG_ACK;
			atomic_inc( &(privdata->irq_outstanding[ ABB_IRQ_IG ]) );
			wake_up_interruptible( &(privdata->irq_queues[ ABB_IRQ_IG ]) );
			return 1;
		}

		mod_info_dbg("err: interrupt registers. ISR: %x, IER: %x\n", bar[ ABB_INT_STAT ], bar[ ABB_INT_CTRL ] );
		// Let pass anything else
		return 0;
	}
 	
 	return 0;
}
