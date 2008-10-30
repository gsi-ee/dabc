/**
 * This is a full rewrite of the pciDriver.
 * New default is to support kernel 2.6, using kernel 2.6 APIs.
 *
 * $Revision: 1.9 $
 * $Date: 2008/05/16 10:40:53 $
 * 
 */

/*
 * Change History:
 * 
 * $Log: pciDriver_base.c,v $
 * Revision 1.9  2008/05/16 10:40:53  adamczew
 * *** empty log message ***
 *
 * Revision 1.8  2008/04/07 16:09:05  adamczew
 * *** empty log message ***
 *
 * Revision 1.12  2008-01-24 14:21:36  marcus
 * Added a CLEAR_INTERRUPT_QUEUE ioctl.
 * Added a sysfs attribute to show the outstanding IRQ queues.
 *
 * Revision 1.11  2008-01-24 12:53:11  marcus
 * Corrected wait_event condition in waiti_ioctl. Improved the loop too.
 *
 * Revision 1.10  2008-01-14 10:39:39  marcus
 * Set some messages as debug instead of normal.
 *
 * Revision 1.9  2008-01-11 10:18:28  marcus
 * Modified interrupt mechanism. Added atomic functions and queues, to address race conditions. Removed unused interrupt code.
 *
 * Revision 1.8  2007-07-17 13:15:55  marcus
 * Removed Tasklets.
 * Using newest map for the ABB interrupts.
 *
 * Revision 1.7  2007-07-06 15:56:04  marcus
 * Change default status for OLD_REGISTERS to not defined.
 *
 * Revision 1.6  2007-07-05 15:29:59  marcus
 * Corrected issue with the bar mapping for interrupt handling.
 * Added support up to kernel 2.6.20
 *
 * Revision 1.5  2007-05-29 07:50:18  marcus
 * Split code into 2 files. May get merged in the future again....
 *
 * Revision 1.4  2007/03/01 17:47:34  marcus
 * Fixed bug when the kernel memory was less than one page, it was not locked properly, recalling an old mapping issue in this case.
 *
 * Revision 1.3  2007/03/01 17:01:22  marcus
 * comment fix (again).
 *
 * Revision 1.2  2007/03/01 17:00:25  marcus
 * Changed some comment in the log.
 *
 * Revision 1.1  2007/03/01 16:57:43  marcus
 * Divided driver file to ease the interrupt hooks for the user of the driver.
 * Modified Makefile accordingly.
 *
 * From pciDriver.c:
 * Revision 1.11  2006/12/11 16:15:43  marcus
 * Fixed kernel buffer mmapping, and driver crash when application crashes.
 * Buffer memory is now marked reserved during allocation, and mmaped with
 * remap_xx_range.
 *
 * Revision 1.10  2006/11/21 09:50:49  marcus
 * Added PROGRAPE4 vendor/device IDs.
 *
 * Revision 1.9  2006/11/17 18:47:36  marcus
 * Removed MERGE_SGENTRIES flag, now it is selected at runtime with 'type'.
 * Removed noncached in non-prefetchable areas, to allow the use of MTRRs.
 *
 * Revision 1.8  2006/11/17 16:41:21  marcus
 * Added slot number to the PCI info IOctl.
 *
 * Revision 1.7  2006/11/13 12:30:34  marcus
 * Added a IOctl call, to confiure the interrupt response. (testing pending).
 * Basic interrupts are now supported, using a Tasklet and Completions.
 *
 * Revision 1.6  2006/11/08 21:30:02  marcus
 * Added changes after compile tests in kernel 2.6.16
 *
 * Revision 1.5  2006/10/31 07:57:38  marcus
 * Improved the pfn calculation in nopage(), to deal with some possible border
 * conditions. It was really no issue, because they are normally page-aligned
 * anyway, but to be on the safe side.
 *
 * Revision 1.4  2006/10/30 19:37:40  marcus
 * Solved bug on kernel memory not mapping properly.
 *
 * Revision 1.3  2006/10/18 11:19:20  marcus
 * Added kernel 2.6.8 support based on comments from Joern Adamczewski (GSI).
 *
 * Revision 1.2  2006/10/18 11:04:15  marcus
 * Bus Master is only activated when we detect a specific board.
 *
 * Revision 1.1  2006/10/10 14:46:51  marcus
 * Initial commit of the new pciDriver for kernel 2.6
 *
 * Revision 1.9  2006/10/05 11:30:46  marcus
 * Prerelease. Added bus and devfn to pciInfo for compatibility.
 *
 * Revision 1.8  2006/09/25 16:51:07  marcus
 * Added PCI config IOctls, and implemented basic mmap functions.
 *
 * Revision 1.7  2006/09/20 11:12:41  marcus
 * Added Merge SG entries
 *
 * Revision 1.6  2006/09/19 17:22:18  marcus
 * backup commit.
 *
 * Revision 1.5  2006/09/18 17:13:11  marcus
 * backup commit.
 *
 * Revision 1.4  2006/09/15 15:44:41  marcus
 * backup commit.
 *
 * Revision 1.3  2006/08/15 11:40:02  marcus
 * backup commit.
 *
 * Revision 1.2  2006/08/12 18:28:42  marcus
 * Sync with the laptop
 *
 * Revision 1.1  2006/08/11 15:30:46  marcus
 * Sync with the laptop
 *
 */

#include <linux/version.h>

/* Check macros and kernel version first */
#ifndef KERNEL_VERSION
#error "No KERNEL_VERSION macro! Stopping."
#endif

#ifndef LINUX_VERSION_CODE
#error "No LINUX_VERSION_CODE macro! Stopping."
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,8)
#error "This driver has been test only for Kernel 2.6.8 or above."
#endif

/* Required includes */
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sysfs.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <asm/scatterlist.h>
#include <linux/vmalloc.h>
#include <linux/stat.h>
#include <linux/interrupt.h>
#include <linux/wait.h>

#include "pciDriver.h"

/*******************************/
/* Defines controlling options */
/*******************************/

#define ENABLE_IRQ
#define NOTASKLETS
//#define OLD_REGISTERS


/* Some very basic config stuff */
/* The name of the module */
#define MODNAME "pciDriver"

/* Major number is allocated dynamically */
/* Minor number */
#define MINORNR 0

/* Node name of the char device */
#define NODENAME "fpga"
#define NODENAMEFMT "fpga%d"

/* Maximum number of devices*/
#define MAXDEVICES 4


/*************************************************************************/
/* First, declare function prototypes for easily reading the code */

/* prototypes for vm_operations */
void pcidriver_vm_open(struct vm_area_struct * area);
void pcidriver_vm_close(struct vm_area_struct * area);
struct page *pcidriver_vm_nopage(struct vm_area_struct * area, unsigned long address, int * type);

/* prototypes for file_operations */
static struct file_operations pcidriver_fops;
int pcidriver_ioctl( struct inode  *inode, struct file   *filp, unsigned int cmd, unsigned long arg );
int pcidriver_mmap( struct file *filp, struct vm_area_struct *vmap );
int pcidriver_open(struct inode *inode, struct file *filp );
int pcidriver_release(struct inode *inode, struct file *filp);

/* prototypes for device operations */
static struct pci_driver __devinitdata pcidriver_driver;
static int __devinit pcidriver_probe(struct pci_dev *pdev, const struct pci_device_id *id);
static void __devexit pcidriver_remove(struct pci_dev *pdev);

/* prototype for interrupt handling */
irqreturn_t pcidriver_irq_handler( int irq, void *dev_id, struct pt_regs *regs);

/* prototypes for sysfs operations */
static ssize_t pcidriver_show_mmap_mode(struct class_device *cls, char *buf);
static ssize_t pcidriver_store_mmap_mode(struct class_device *cls, char *buf, size_t count);
static ssize_t pcidriver_show_mmap_area(struct class_device *cls, char *buf);
static ssize_t pcidriver_store_mmap_area(struct class_device *cls, char *buf, size_t count);
static ssize_t pcidriver_show_kmem_count(struct class_device *cls, char *buf);
static ssize_t pcidriver_show_kbuffers(struct class_device *cls, char *buf);
static ssize_t pcidriver_store_kmem_alloc(struct class_device *cls, char *buf, size_t count);
static ssize_t pcidriver_store_kmem_free(struct class_device *cls, char *buf, size_t count);
static ssize_t pcidriver_show_umappings(struct class_device *cls, char *buf);
static ssize_t pcidriver_store_umem_unmap(struct class_device *cls, char *buf, size_t count);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
/* For kernels >=2.6.13 - not implemented yet */
 static ssize_t pcidriver_show_kmem_entry(struct class_device *cls, char *buf);
 static ssize_t pcidriver_show_umem_entry(struct class_device *cls, char *buf);
#endif

/* prototypes for module operations */
static int __init pcidriver_init(void);
static void pcidriver_exit(void);

/*************************************************************************/
/* Some nice defines that make code more readable */
/* This is to print nice info in the log */
#ifdef DEBUG
 #define mod_info( args... ) \
    do { printk( KERN_INFO "%s - %s : ", MODNAME , __FUNCTION__ );\
    printk( args ); } while(0)
 #define mod_info_dbg( args... ) \
    do { printk( KERN_INFO "%s - %s : ", MODNAME , __FUNCTION__ );\
    printk( args ); } while(0)
#else
 #define mod_info( args... ) \
    do { printk( KERN_INFO "%s: ", MODNAME );\
    printk( args ); } while(0)
 #define mod_info_dbg( args... ) 
#endif

#define mod_crit( args... ) \
    do { printk( KERN_CRIT "%s: ", MODNAME );\
    printk( args ); } while(0)

/*************************************************************************/
/* Private data types and structures */

/* Define an entry in the kmem list (this list is per device) */
/* This list keeps references to the allocated kernel buffers */
typedef struct {
	int id;
	struct list_head list;
	dma_addr_t dma_handle;
	unsigned long cpua;
	unsigned long size;
	struct class_device_attribute sysfs_attr;	/* initialized when adding the entry */
} pcidriver_kmem_entry_t;

/* Define an entry in the umem list (this list is per device) */
/* This list keeps references to the SG lists for each mapped userspace region */
typedef struct {
	int id;
	struct list_head list;
	unsigned int nr_pages;		/* number of pages for this user memeory area */
	struct page **pages;		/* list of pointers to the pages */
	unsigned int nents;			/* actual entries in the scatter/gatter list (NOT nents for the map function, but the result) */
	struct scatterlist *sg;		/* list of sg entries */
	struct class_device_attribute sysfs_attr;	/* initialized when adding the entry */
} pcidriver_umem_entry_t;

/* Hold the driver private data */
typedef struct  {
	dev_t devno;						/* device number (major and minor) */
	struct pci_dev *pdev;				/* PCI device */
	struct class_device *class_dev;		/* Class device */
	struct cdev cdev;					/* char device struct */
	int mmap_mode;						/* current mmap mode */
	int mmap_area;						/* current PCI mmap area */

#ifdef ENABLE_IRQ
	int irq_enabled;					/* Non-zero if IRQ is enabled */
	int irq_count;						/* Just an IRQ counter */

	wait_queue_head_t irq_queues[ PCIDRIVER_INT_MAXSOURCES ];
										/* One queue per interrupt source */
	atomic_t irq_outstanding[ PCIDRIVER_INT_MAXSOURCES ];
										/* Outstanding interrupts per queue */
	volatile unsigned int *bars_kmapped[6];		/* PCI BARs mmapped in kernel space */

#endif
	
	spinlock_t kmemlist_lock;			/* Spinlock to lock kmem list operations */
	struct list_head kmem_list;			/* List of 'kmem_list_entry's associated with this device */
	atomic_t kmem_count;				/* id for next kmem entry */

	spinlock_t umemlist_lock;			/* Spinlock to lock umem list operations */
	struct list_head umem_list;			/* List of 'umem_list_entry's associated with this device */
	atomic_t umem_count;				/* id for next umem entry */

	
} pcidriver_privdata_t;

/* prototypes for IRQ related functions */
#ifdef ENABLE_IRQ
/* sysfs functions */
static ssize_t pcidriver_show_irq_count(struct class_device *cls, char *buf);
static ssize_t pcidriver_show_irq_queues(struct class_device *cls, char *buf);
#endif


/* prototypes for internal driver functions */
int pcidriver_kmem_alloc( pcidriver_privdata_t *privdata, kmem_handle_t *kmem_handle );
int pcidriver_kmem_free(  pcidriver_privdata_t *privdata, kmem_handle_t *kmem_handle );
int pcidriver_kmem_sync(  pcidriver_privdata_t *privdata, kmem_sync_t *kmem_sync );
int pcidriver_kmem_free_all(  pcidriver_privdata_t *privdata );
pcidriver_kmem_entry_t *pcidriver_kmem_find_entry( pcidriver_privdata_t *privdata, kmem_handle_t *kmem_handle );
pcidriver_kmem_entry_t *pcidriver_kmem_find_entry_id( pcidriver_privdata_t *privdata, int id );
int pcidriver_kmem_free_entry( pcidriver_privdata_t *privdata, pcidriver_kmem_entry_t *kmem_entry );

int pcidriver_umem_sgmap( pcidriver_privdata_t *privdata, umem_handle_t *umem_handle );
int pcidriver_umem_sgunmap( pcidriver_privdata_t *privdata, pcidriver_umem_entry_t *umem_entry );
int pcidriver_umem_sgget( pcidriver_privdata_t *privdata, umem_sglist_t *umem_sglist );
int pcidriver_umem_sync( pcidriver_privdata_t *privdata, umem_handle_t *umem_handle );
pcidriver_umem_entry_t *pcidriver_umem_find_entry_id( pcidriver_privdata_t *privdata, int id );

int pcidriver_pci_read( pcidriver_privdata_t *privdata, pci_cfg_cmd *pci_cmd );
int pcidriver_pci_write( pcidriver_privdata_t *privdata, pci_cfg_cmd *pci_cmd );
int pcidriver_pci_info( pcidriver_privdata_t *privdata, pci_board_info *pci_info );

int pcidriver_mmap_pci( pcidriver_privdata_t *privdata, struct vm_area_struct *vmap , int bar );
int pcidriver_mmap_kmem( pcidriver_privdata_t *privdata, struct vm_area_struct *vmap );

void pcidriver_interrupt_tasklet(unsigned long data);

#ifdef ENABLE_IRQ
int pcidriver_irq_acknowledge(pcidriver_privdata_t *privdata);

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,9)
/* Not defined yet for these kernels, but needed. 
 * So I have to recreate it here in that case */
int wait_for_completion_interruptible(struct completion *x);
#endif
#endif

/*************************************************************************/
/* Static data */
/* Hold the allocated major & minor numbers */
static dev_t pcidriver_devt;

/* Holds the class to add sysfs support */
/* 
 * simple_class was promoted in kernel 2.6.13 to be the standard class
 * behavior, so we have to check which version to use.
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
 static struct class *pcidriver_class;
#else
 static struct class_simple *pcidriver_class;
#endif

/* Number of devices allocated */
static atomic_t pcidriver_deviceCount;

/*
 * This is the table of PCI devices handled by this driver by default
 * If you want to add devices dynamically to this list, do:
 * 
 *   echo "vendor device" > /sys/bus/pci/drivers/pciDriver/new_id
 * where vendor and device are in hex, without leading '0x'.
 * 
 * For more info, see <kernel-source>/Documentation/pci.txt
 */

/* Identifies the mpRACE-1 boards */
#define MPRACE1_VENDOR_ID 0x10b5
#define MPRACE1_DEVICE_ID 0x9656

/* Identifies the PCI-X Test boards */
#define PCIXTEST_VENDOR_ID 0x10dc
#define PCIXTEST_DEVICE_ID 0x0156

/* Identifies the PCIe-PLDA Test board */
#define PCIEPLDA_VENDOR_ID 0x1556
#define PCIEPLDA_DEVICE_ID 0x1100

/* Identifies the PCIe-ABB Test board */
#define PCIEABB_VENDOR_ID 0x10dc
#define PCIEABB_DEVICE_ID 0x0153

/* Identifies the PCI-X PROGRAPE4 */
#define PCIXPG4_VENDOR_ID 0x1679
#define PCIXPG4_DEVICE_ID 0x0001

/* Identifies the PCI-64 PROGRAPE4 */
#define PCI64PG4_VENDOR_ID 0x1679
#define PCI64PG4_DEVICE_ID 0x0005
 
static struct pci_device_id pcidriver_ids[] = {
	{ PCI_DEVICE( MPRACE1_VENDOR_ID , MPRACE1_DEVICE_ID ) },		// mpRACE-1
	{ PCI_DEVICE( PCIXTEST_VENDOR_ID , PCIXTEST_DEVICE_ID ) },		// pcixTest
	{ PCI_DEVICE( PCIEPLDA_VENDOR_ID , PCIEPLDA_DEVICE_ID ) },		// PCIePLDA
	{ PCI_DEVICE( PCIEABB_VENDOR_ID , PCIEABB_DEVICE_ID ) },		// PCIeABB
	{ PCI_DEVICE( PCIXPG4_VENDOR_ID , PCIXPG4_DEVICE_ID ) },		// PCI-X PROGRAPE 4
	{ PCI_DEVICE( PCI64PG4_VENDOR_ID , PCI64PG4_DEVICE_ID ) },		// PCI-64 PROGRAPE 4
	{0,0,0,0},
};

/* Sysfs attributes */
static CLASS_DEVICE_ATTR(mmap_mode, (S_IRUGO | S_IWUGO) ,pcidriver_show_mmap_mode,pcidriver_store_mmap_mode);
static CLASS_DEVICE_ATTR(mmap_area, (S_IRUGO | S_IWUGO) ,pcidriver_show_mmap_area,pcidriver_store_mmap_area);
static CLASS_DEVICE_ATTR(kmem_count,S_IRUGO,pcidriver_show_kmem_count,NULL);
static CLASS_DEVICE_ATTR(kbuffers,S_IRUGO,pcidriver_show_kbuffers,NULL);
static CLASS_DEVICE_ATTR(kmem_alloc,S_IWUGO,NULL,pcidriver_store_kmem_alloc);
static CLASS_DEVICE_ATTR(kmem_free,S_IWUGO,NULL,pcidriver_store_kmem_free);
static CLASS_DEVICE_ATTR(umappings,S_IRUGO,pcidriver_show_umappings,NULL);
static CLASS_DEVICE_ATTR(umem_unmap,S_IWUGO,NULL,pcidriver_store_umem_unmap);

#ifdef ENABLE_IRQ
static CLASS_DEVICE_ATTR(irq_count,S_IRUGO,pcidriver_show_irq_count,NULL);
static CLASS_DEVICE_ATTR(irq_queues,S_IRUGO,pcidriver_show_irq_queues,NULL);
#endif

/*************************************************************************/
/* Module device table associated with this driver */
MODULE_DEVICE_TABLE(pci, pcidriver_ids);

/* Module init and exit points */
module_init(pcidriver_init);
module_exit(pcidriver_exit);

/* Module info */
MODULE_AUTHOR("Guillermo Marcus");
MODULE_DESCRIPTION("Simple PCI Driver");
MODULE_LICENSE("GPL v2");

/* Module Entry functions */
static int __init pcidriver_init(void)
{
	int err;

	/* Initialize the device count */
	atomic_set(&pcidriver_deviceCount, 0);

	/* Allocate character device region dynamically */
	err = alloc_chrdev_region( &pcidriver_devt, MINORNR, MAXDEVICES, NODENAME );
	if (err) {
		mod_info( "Couldn't allocate chrdev region. Module not loaded.\n" );
		goto init_alloc_fail;
	}
	mod_info( "Major %d allocated to nodename '%s'\n", MAJOR(pcidriver_devt), NODENAME );

	/* Register driver class */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
	pcidriver_class = class_create(THIS_MODULE, NODENAME);
#else
	pcidriver_class = class_simple_create(THIS_MODULE, NODENAME);
#endif

	if (IS_ERR(pcidriver_class)) {
		mod_info("No sysfs support. Module not loaded.\n");
		goto init_class_fail;
	}
		
	/* Register PCI driver */
	err = pci_register_driver(&pcidriver_driver);
/* RedHat returns the number of devices instead of zero. Then we better 
 * check for an error as <0.*/
	if (err<0) {
		mod_info( "Couldn't register PCI driver. Module not loaded.\n" );
		goto init_pcireg_fail;
	}
	
	/* Success */
	mod_info( "Module loaded\n" );
	return 0;

init_pcireg_fail:
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
	class_destroy(pcidriver_class);
#else
	class_simple_destroy(pcidriver_class);
#endif
init_class_fail:
	unregister_chrdev_region(pcidriver_devt,MAXDEVICES);
init_alloc_fail:
	return err;
}

/* Module exit function */
static void pcidriver_exit(void)
{
	if (pcidriver_class != NULL)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
		class_destroy(pcidriver_class);
#else
		class_simple_destroy(pcidriver_class);
#endif
		
	pci_unregister_driver(&pcidriver_driver);
	unregister_chrdev_region(pcidriver_devt,MAXDEVICES);
	mod_info( "Module unloaded\n" );
}

/*************************************************************************/
/* Driver functions */

/* This is the PCI driver struct info.
 * It defines the PCI entry points.
 * Will be registered at module init.
 */
static struct pci_driver __devinitdata pcidriver_driver = {
	.name = MODNAME,
	.id_table = pcidriver_ids,
	.probe = pcidriver_probe,
	.remove = pcidriver_remove,
};

/* This function is called when installing the driver for a device */
static int __devinit pcidriver_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	int err;
	int devno;
	int i;
	pcidriver_privdata_t *privdata;
	int devid;
#ifdef ENABLE_IRQ	
	unsigned char int_pin, int_line;
	unsigned long bar_addr, bar_len, bar_flags;
#endif

	/* At the moment there is no difference between these boards here, other than 
	 * printing a different message in the log.
	 * 
	 * However, there is some difference in the interrupt handling functions.
	 */	
	if ( (id->vendor == MPRACE1_VENDOR_ID) &&
		(id->device == MPRACE1_DEVICE_ID))
	{	
		/* It is a mpRACE-1 */
		mod_info( "Found mpRACE-1 at %s\n", pdev->dev.bus_id );
		/* Set bus master */
		pci_set_master(pdev);
	}
	else if ((id->vendor == PCIXTEST_VENDOR_ID) &&
		(id->device == PCIXTEST_DEVICE_ID))
	{
		/* It is a PCI-X Test board */
		mod_info( "Found PCI-X test board at %s\n", pdev->dev.bus_id );
	}
	else if ((id->vendor == PCIEPLDA_VENDOR_ID) &&
		(id->device == PCIEPLDA_DEVICE_ID))
	{
		/* It is a PCI-X Test board */
		mod_info( "Found PCIe PLDA test board at %s\n", pdev->dev.bus_id );
	}
	else if ((id->vendor == PCIEABB_VENDOR_ID) &&
		(id->device == PCIEABB_DEVICE_ID))
	{
		/* It is a PCI-X Test board */
		mod_info( "Found PCIe ABB test board at %s\n", pdev->dev.bus_id );
	}
	else if ((id->vendor == PCIXPG4_VENDOR_ID) &&
		(id->device == PCIXPG4_DEVICE_ID))
	{
		/* It is a PCI-X PROGRAPE4 board */
		mod_info( "Found PCI-X PROGRAPE-4 board at %s\n", pdev->dev.bus_id );
	}
	else if ((id->vendor == PCI64PG4_VENDOR_ID) &&
		(id->device == PCI64PG4_DEVICE_ID))
	{
		/* It is a PCI-64 PROGRAPE4 board */
		mod_info( "Found PCI-64b/66 PROGRAPE-4 board at %s\n", pdev->dev.bus_id );
	}
	else
	{
		/* It is something else */
		mod_info( "Found unknown board (%x:%x) at %s\n", id->vendor, id->device, pdev->dev.bus_id );
	}
	
	
	/* Enable the device */
	err = pci_enable_device(pdev);
	if (err) {
		mod_info( "Couldn't enable device\n" );
		goto probe_pcien_fail;
	}
				
	/* Set Memory-Write-Invalidate support */
	err = pci_set_mwi(pdev);
	if (err)
		mod_info( "MWI not supported. Continue without enabling MWI.\n" );

	/* Get / Increment the device id */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,9)
	devid = atomic_inc_return(&pcidriver_deviceCount) - 1;
#else
	atomic_inc(&pcidriver_deviceCount);
	devid = atomic_read(&pcidriver_deviceCount) - 1;
#endif
	if ( devid >= MAXDEVICES ) {
		mod_info( "Maximum number of devices reached! Increase MAXDEVICES.\n" );
		err = -ENOMSG;
		goto probe_maxdevices_fail;
	}
		
	/* Set private data pointer for this device */
	privdata = (pcidriver_privdata_t*)kmalloc(sizeof(*privdata), GFP_KERNEL);
	if (!privdata) {
		err = -ENOMEM;
		goto probe_nomem;
	}
	
	/* Initialize private data structures */
	memset( privdata, 0, sizeof( *privdata ) );
	
	INIT_LIST_HEAD(&(privdata->kmem_list));
	spin_lock_init(&(privdata->kmemlist_lock));
	atomic_set(&privdata->kmem_count, 0);

	INIT_LIST_HEAD(&(privdata->umem_list));
	spin_lock_init(&(privdata->umemlist_lock));
	atomic_set(&privdata->umem_count, 0);

	pci_set_drvdata( pdev, privdata );
	privdata->pdev = pdev;

	/* Device add to sysfs */
	devno = MKDEV( MAJOR(pcidriver_devt), MINOR(pcidriver_devt) + devid );
	privdata->devno = devno;
	if (pcidriver_class != NULL) {

	/* FIXME: some error checking missing here */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
		privdata->class_dev = class_device_create(pcidriver_class, NULL, devno, &(pdev->dev), NODENAMEFMT, MINOR(pcidriver_devt) + devid );
		class_set_devdata( privdata->class_dev, privdata );
#else
		privdata->class_dev = class_simple_device_add(pcidriver_class, devno, &(pdev->dev), NODENAMEFMT, MINOR(pcidriver_devt) + devid );
		privdata->class_dev->class_data = privdata;
#endif
		mod_info("Device /dev/%s%d added\n",NODENAME,MINOR(pcidriver_devt) + devid);
	}
		
	/* Setup mmaped BARs into kernel space */
#ifdef ENABLE_IRQ
	/* at the moment this is used only to INT ack, therefore it is inside 
	 * the ENABLE_IRQ define */
	for(i=0;i<6;i++)
		privdata->bars_kmapped[i] = NULL;
	
	for(i=0;i<6;i++) {
		
		bar_addr = pci_resource_start( privdata->pdev, i);
		bar_len  = pci_resource_len( privdata->pdev, i);
		bar_flags = pci_resource_flags( privdata->pdev, i);

		/* check if it is a valid BAR, skip if not */
		if ((bar_addr == 0) || (bar_len==0))
			continue;

		/* Skip IO regions (map only mem regions) */
		if (bar_flags & IORESOURCE_IO)
			continue;
		
		/* Check if the region is available */
		err = pci_request_region( privdata->pdev, i, NULL );
		if (err) {
			mod_info( "Failed to request BAR memory region.\n" );
			goto probe_reqregion_fail;
		}

		/* Map it into kernel space. */
		/* For x86 this is just a dereference of the pointer, but that is
		 * not portable. So we need to do the portable way. Thanks Joern!
		 */
		
		/* respect the cacheable-bility of the region */
		if (bar_flags & IORESOURCE_PREFETCH)
			privdata->bars_kmapped[i] = ioremap( bar_addr, bar_len );
		else
			privdata->bars_kmapped[i] = ioremap_nocache( bar_addr, bar_len );
		
		/* check for error */
		if (privdata->bars_kmapped[i] == NULL) {
			err = -EIO;
			mod_info( "Failed to remap BAR%d into kernel space.\n", i );
			goto probe_remap_fail;
		}
	}
#endif
	
	/* Initialize the interrupt handler for this device */
#ifdef ENABLE_IRQ
	/* Initialize the wait queues */
	for(i=0;i<PCIDRIVER_INT_MAXSOURCES;i++) {
		init_waitqueue_head( &(privdata->irq_queues[i]) );
		atomic_set( &(privdata->irq_outstanding[i]), 0 );
	}
	
	/* Initialize the irq config */
	err = pci_read_config_byte(pdev, PCI_INTERRUPT_PIN, &int_pin );
	if (err) {
		/* continue without interrupts */
		int_pin = 0;
		mod_info("Error getting the interrupt pin. Disabling interrupts for this device\n");
	}
	
	if (int_pin != 0) {
		err = pci_read_config_byte(pdev, PCI_INTERRUPT_LINE, &int_line );
		if (err) {
			int_pin = 0;
			mod_info("Error getting the interrupt line. Disabling interrupts for this device\n");		
		} else {
			/* register interrupt handler */
			err = request_irq( pdev->irq, pcidriver_irq_handler, SA_SHIRQ, MODNAME, privdata );
			
			/* request failed, continue without interrupts */
			if (err) {
				int_pin = 0;
				mod_info("Error registering the interrupt handler. Disabling interrupts for this device\n");					
			}
		}
	}

	if (int_pin != 0) {
		privdata->irq_enabled = 1;
		mod_info("Registered Interrupt Handler at pin %i, line %i, IRQ %i\n", int_pin, int_line, pdev->irq );
	} else
		privdata->irq_enabled = 0;

	class_device_create_file( privdata->class_dev, &class_device_attr_irq_count );
	class_device_create_file( privdata->class_dev, &class_device_attr_irq_queues );
#endif


	/* Populate sysfs attributes for the class device */
	class_device_create_file( privdata->class_dev, &class_device_attr_mmap_mode );
	class_device_create_file( privdata->class_dev, &class_device_attr_mmap_area );
	class_device_create_file( privdata->class_dev, &class_device_attr_kmem_count );
	class_device_create_file( privdata->class_dev, &class_device_attr_kmem_alloc );
	class_device_create_file( privdata->class_dev, &class_device_attr_kmem_free );
	class_device_create_file( privdata->class_dev, &class_device_attr_kbuffers );
	class_device_create_file( privdata->class_dev, &class_device_attr_umappings );
	class_device_create_file( privdata->class_dev, &class_device_attr_umem_unmap );

	/* Register character device */
	cdev_init( &(privdata->cdev), &pcidriver_fops );
	privdata->cdev.owner = THIS_MODULE;
	privdata->cdev.ops = &pcidriver_fops;
	err = cdev_add( &privdata->cdev, devno, 1 );
	if (err) {
		mod_info( "Couldn't add character device.\n" );
		goto probe_cdevadd_fail;
	}
		
	/* The device is now Live! */	
	/* Success */
	return 0;

probe_cdevadd_fail:
probe_remap_fail:
probe_reqregion_fail:
	for(i=0;i<6;i++) {
		if (privdata->bars_kmapped[i] != NULL) {
			iounmap( privdata->bars_kmapped[i] );
			pci_release_region( pdev, i );
		}
	}
	kfree(privdata);
probe_nomem:
	atomic_dec(&pcidriver_deviceCount);
probe_maxdevices_fail:
	pci_disable_device(pdev);
probe_pcien_fail:	
 	return err;
}

/* This function is called when disconnecting a device */
static void __devexit pcidriver_remove(struct pci_dev *pdev)
{
	pcidriver_privdata_t *privdata;
	int i;

	/* Get private data from the device */
	privdata = pci_get_drvdata(pdev);

	/* Removing sysfs attributes from class device */
	class_device_remove_file( privdata->class_dev, &class_device_attr_mmap_mode );
	class_device_remove_file( privdata->class_dev, &class_device_attr_mmap_area );
	class_device_remove_file( privdata->class_dev, &class_device_attr_kmem_count );
	class_device_remove_file( privdata->class_dev, &class_device_attr_kmem_alloc );
	class_device_remove_file( privdata->class_dev, &class_device_attr_kmem_free );
	class_device_remove_file( privdata->class_dev, &class_device_attr_kbuffers );
	class_device_remove_file( privdata->class_dev, &class_device_attr_umappings );
	class_device_remove_file( privdata->class_dev, &class_device_attr_umem_unmap );
	
	/* Free all allocated kmem buffers before leaving */
	pcidriver_kmem_free_all( privdata );

#ifdef ENABLE_IRQ
	/* Unregister the sysfs entries */
	class_device_remove_file( privdata->class_dev, &class_device_attr_irq_count );
	class_device_remove_file( privdata->class_dev, &class_device_attr_irq_queues );

	/* Releasing the IRQ handler */
	if (privdata->irq_enabled != 0)
		free_irq( privdata->pdev->irq, privdata );
		
	/* Unmap and release the remapped BARs */
	for(i=0;i<6;i++) {
		if (privdata->bars_kmapped[i] != NULL) {
			iounmap( privdata->bars_kmapped[i] );
			pci_release_region( privdata->pdev, i );
		}
	}

#endif

	/* Removing Character device */
	cdev_del(&(privdata->cdev));

	/* Removing the device from sysfs */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
	class_device_destroy(pcidriver_class, privdata->devno);
#else
	class_simple_device_remove(privdata->devno);
#endif

	/* Releasing privdata */
	kfree(privdata);
	
	/* Disabling PCI device */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20)
	/* Starting in 2.6.20, is_enabled does no longer exists.
	 * Disable is now an atomic counter, so it is safe to just call it. */
	pci_disable_device(pdev);	
#else
	if (pdev->is_enabled)
		pci_disable_device(pdev);	
#endif

	mod_info( "Device at %s removed\n", pdev->dev.bus_id );
}

/*************************************************************************/
/* VM operations */
/* These are used during kmem mapping, as pages are mmaped on need to user space */

struct vm_operations_struct pcidriver_vm_operations = {
  .open = pcidriver_vm_open,
  .close = pcidriver_vm_close,
  .nopage = pcidriver_vm_nopage
};

void pcidriver_vm_open(struct vm_area_struct * vma) {
	mod_info_dbg("Page opened\n");
}

void pcidriver_vm_close(struct vm_area_struct * vma) {
	mod_info_dbg("Page closed\n");
}

/* Maps physical memory to user space */
/* 
 * This method works only when everything goes fine. If the app crashes, 
 * this creates several problems. Therefore, this method for mapping
 * kernel memory into user space is deprecated.
 */
struct page *pcidriver_vm_nopage(struct vm_area_struct *vma, unsigned long address, int * type) {

	pcidriver_kmem_entry_t *kmem_entry;
	struct page *page = NOPAGE_SIGBUS;
	unsigned long pfn_offset,pfn;
		
	/* Get private data for the page */
	kmem_entry = vma->vm_private_data;

	/* All checks where done during the mmap_kmem call, so we can safely
	 * just map the offset of the vm area to the offset of the region,
	 * that is guaranteed to be contiguos */

	pfn_offset = (address) - (vma->vm_start);
	pfn = (__pa(kmem_entry->cpua) + pfn_offset ) >> PAGE_SHIFT;

	mod_info_dbg("vma_start: %08lx, vm_end: %08lx, addr: %08lx, cpua: %08lx\n",
		vma->vm_start, vma->vm_end, address, kmem_entry->cpua );

	mod_info_dbg("pfn_offset: %08lx, pfn: %08lx\n",
		pfn_offset, pfn );
	
	if (!pfn_valid(pfn)) {
		mod_info("Invalid pfn in nopage() - 0x%lx \n", pfn);
		return NOPAGE_SIGBUS;
	}
		
	page = pfn_to_page( pfn );

	get_page(page);
	if (type)
		*type = VM_FAULT_MINOR;

	mod_info_dbg("Page mapped in nopage\n");

	return page;
}

/*************************************************************************/
/* File operations */

static struct file_operations pcidriver_fops = {
	.owner = THIS_MODULE,
	.ioctl = pcidriver_ioctl,
	.mmap = pcidriver_mmap,
	.open = pcidriver_open,
	.release = pcidriver_release,
};

/* Functions to handle all (character) file operations */

int pcidriver_open(struct inode *inode, struct file *filp )
{
	pcidriver_privdata_t *privdata;

	/* Set the private data area for the file */
	privdata = container_of( inode->i_cdev, pcidriver_privdata_t, cdev);
	filp->private_data = privdata;

	/* Success */
	return 0;
}

int pcidriver_release(struct inode *inode, struct file *filp)
{
	pcidriver_privdata_t *privdata;

	/* Get the private data area */
	privdata = filp->private_data;

	/* Success */
	return 0;
}

/* This function handles all mmap file operations */
int pcidriver_mmap( struct file *filp, struct vm_area_struct *vma ) {
	pcidriver_privdata_t *privdata;
	int ret = 0, bar;

	mod_info_dbg("Entering mmap\n");

	/* Get the private data area */
	privdata = filp->private_data;

	/* Check the current mmap mode */
	switch (privdata->mmap_mode) {
		case PCIDRIVER_MMAP_PCI:
			/* Mmap a PCI region */
			switch (privdata->mmap_area) {
				case PCIDRIVER_BAR0:	bar = 0; break;
				case PCIDRIVER_BAR1:	bar = 1; break;
				case PCIDRIVER_BAR2:	bar = 2; break;
				case PCIDRIVER_BAR3:	bar = 3; break;
				case PCIDRIVER_BAR4:	bar = 4; break;
				case PCIDRIVER_BAR5:	bar = 5; break;
				default:
					mod_info("Attempted to mmap a PCI area with the wrong mmap_area value: %d\n",privdata->mmap_area);
					return -EINVAL;			/* invalid parameter */
					break;
			}
			/* Do it in a separate function */
			ret = pcidriver_mmap_pci( privdata, vma , bar );
			break;
		case PCIDRIVER_MMAP_KMEM:
			/* Mmap a Kernel buffer */			
			/* Do it in a separate function */
			ret = pcidriver_mmap_kmem( privdata, vma );
			break;
		default:
			mod_info( "Invalid mmap_mode value (%d)\n",privdata->mmap_mode );
			return -EINVAL;			/* Invalid parameter (mode) */
			break;
	}

  return ret;
}


/* This function handles all ioctl file operations */
int pcidriver_ioctl( struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg )
{
	pcidriver_privdata_t *privdata;
	pci_cfg_cmd pci_cmd;
	pci_board_info pci_info;
	kmem_handle_t khandle;
	kmem_sync_t ksync;
	umem_handle_t uhandle;
	umem_sglist_t usglist;
	pcidriver_umem_entry_t *umem_entry;
	int temp, ret;
#ifdef ENABLE_IRQ
	unsigned int irq_source;
#endif
	
	/* Get the private data area */
	privdata = filp->private_data;

	/* Select the appropiate command */
	switch (cmd) {
		case PCIDRIVER_IOC_MMAP_MODE:
			/* get single value from user space */
			temp = arg;								/* argument is the direct value */

			/* validate input */
			switch (temp) {
				case PCIDRIVER_MMAP_PCI:
				case PCIDRIVER_MMAP_KMEM:
					/* change the mode */
					privdata->mmap_mode = temp;
					break;
				default:
					/* invalid parameter */
					return -EINVAL;
					break;
			}
			break;
		case PCIDRIVER_IOC_MMAP_AREA:
			/* get single value from user space */
			temp = arg;								/* argument is the direct value */

			/* validate input */
			switch (temp) {
				case PCIDRIVER_BAR0:
				case PCIDRIVER_BAR1:
				case PCIDRIVER_BAR2:
				case PCIDRIVER_BAR3:
				case PCIDRIVER_BAR4:
				case PCIDRIVER_BAR5:
					/* change the PCI area to mmap */
					privdata->mmap_area = temp;
					break;
				default:
					/* invalid parameter */
					return -EINVAL;
					break;
			}
			break;
			
		case PCIDRIVER_IOC_PCI_CFG_RD:
			/* get data from user space */
			ret = copy_from_user( &pci_cmd, (pci_cfg_cmd *)arg, sizeof(pci_cmd) );
			if (ret) return -EFAULT;				/* invalid memory pointer */

			/* Process the command in a separate function */
			ret = pcidriver_pci_read( privdata, &pci_cmd );
			if (ret) return ret;

			/* write data to user space */
			ret = copy_to_user( (pci_cfg_cmd *)arg, &pci_cmd, sizeof(pci_cmd) );
			if (ret) return -EFAULT;				/* invalid memory pointer. This should never happen */
			break;

		case PCIDRIVER_IOC_PCI_CFG_WR:
			/* get data from user space */
			ret = copy_from_user( &pci_cmd, (pci_cfg_cmd *)arg, sizeof(pci_cmd) );
			if (ret) return -EFAULT;				/* invalid memory pointer */

			/* Process the command in a separate function */
			ret = pcidriver_pci_write( privdata, &pci_cmd );
			if (ret) return ret;
			
			/* write data to user space */
			ret = copy_to_user( (pci_cfg_cmd *)arg, &pci_cmd, sizeof(pci_cmd) );
			if (ret) return -EFAULT;				/* invalid memory pointer. This should never happen */
			break;

		case PCIDRIVER_IOC_PCI_INFO:
			/* get data from user space */
			ret = copy_from_user( &pci_info, (pci_board_info *)arg, sizeof(pci_info) );
			if (ret) return -EFAULT;				/* invalid memory pointer */

			/* Process the command in a separate function */
			ret = pcidriver_pci_info( privdata, &pci_info );
			if (ret) return ret;
			
			/* write data to user space */
			ret = copy_to_user( (pci_board_info *)arg, &pci_info, sizeof(pci_info) );
			if (ret) return -EFAULT;				/* invalid memory pointer. This should never happen */
			break;
			
		case PCIDRIVER_IOC_KMEM_ALLOC:
			/* get data from user space */
			ret = copy_from_user( &khandle, (kmem_handle_t *)arg, sizeof(khandle) );
			if (ret) return -EFAULT;				/* invalid memory pointer */

			/* Perform this ioctl in a separate function */
			ret = pcidriver_kmem_alloc( privdata, &khandle );
			if (ret) return ret;

			/* write data to user space */
			ret = copy_to_user( (kmem_handle_t *)arg, &khandle, sizeof(khandle) );
			if (ret) return -EFAULT;				/* invalid memory pointer. This should never happen */
			
			break;	

		case PCIDRIVER_IOC_KMEM_FREE:
			/* get data from user space */
			ret = copy_from_user( &khandle, (kmem_handle_t *)arg, sizeof(khandle) );
			if (ret) return -EFAULT;				/* invalid memory pointer */

			/* Perform this ioctl in a separate function */
			ret = pcidriver_kmem_free( privdata, &khandle );
			if (ret) return ret;

			break;

		case PCIDRIVER_IOC_KMEM_SYNC:
			/* get data from user space */
			ret = copy_from_user( &ksync, (kmem_sync_t *)arg, sizeof(kmem_sync_t) );
			if (ret) return -EFAULT;				/* invalid memory pointer */

			ret = pcidriver_kmem_sync( privdata, &ksync );
			if (ret) return ret;

			break;

		case PCIDRIVER_IOC_UMEM_SGMAP:
			/* get data from user space */
			ret = copy_from_user( &uhandle, (umem_handle_t *)arg, sizeof(uhandle) );
			if (ret) return -EFAULT;				/* invalid memory pointer */

			/* Perform this ioctl in a separate function */
			ret = pcidriver_umem_sgmap( privdata, &uhandle );
			if (ret) return ret;

			/* write data to user space */
			ret = copy_to_user( (umem_handle_t *)arg, &uhandle, sizeof(uhandle) );
			if (ret) return -EFAULT;				/* invalid memory pointer. This should never happen */
		
			break;

		case PCIDRIVER_IOC_UMEM_SGUNMAP:
			/* get data from user space */
			ret = copy_from_user( &uhandle, (umem_handle_t *)arg, sizeof(uhandle) );
			if (ret) return -EFAULT;				/* invalid memory pointer */

			/* Find the associated umem_entry for this buffer */
			umem_entry = pcidriver_umem_find_entry_id( privdata, uhandle.handle_id );
			if (umem_entry == NULL)
				return -EINVAL;					/* umem_handle is not valid */

			/* Perform this ioctl in a separate function */
			ret = pcidriver_umem_sgunmap( privdata, umem_entry );
			if (ret) return ret;

			break;

		case PCIDRIVER_IOC_UMEM_SGGET:
			/* get data from user space */
			ret = copy_from_user( &usglist, (umem_sglist_t *)arg, sizeof(usglist) );
			if (ret) return -EFAULT;				/* invalid memory pointer */

			/* contains a pointer to an array, it should be copied too */
			/* size of the array comes in sgcount */
			/* allocate space for the array */
			/* This can be very big, so we use vmalloc */
			usglist.sg = vmalloc( (usglist.nents)*sizeof(umem_sgentry_t) );
			if (usglist.sg == NULL)
				return -ENOMEM;		/* not enough memory to create a copy of the target list */

			/* copy array to kernel structure */
			ret = copy_from_user( usglist.sg, ((umem_sglist_t *)arg)->sg, (usglist.nents)*sizeof(umem_sgentry_t) );
			if (ret) return -EFAULT;				/* invalid memory pointer */

			/* Perform this ioctl in a separate function */
			ret = pcidriver_umem_sgget( privdata, &usglist );
			if (ret) return ret;

			/* write data to user space */
			ret = copy_to_user( ((umem_sglist_t *)arg)->sg, usglist.sg, (usglist.nents)*sizeof(umem_sgentry_t) );
			if (ret) return -EFAULT;				/* invalid memory pointer. This should never happen */

			/* free array memory */
			vfree( usglist.sg );

			/* restore sg pointer to vma address in user space beofre copying */
			usglist.sg = ((umem_sglist_t *)arg)->sg;

			ret = copy_to_user( (umem_handle_t *)arg, &usglist, sizeof(usglist) );
			if (ret) return -EFAULT;				/* invalid memory pointer. This should never happen */

			break;

		case PCIDRIVER_IOC_UMEM_SYNC:
			/* get data from user space */
			ret = copy_from_user( &uhandle, (umem_handle_t *)arg, sizeof(uhandle) );
			if (ret) return -EFAULT;				/* invalid memory pointer */

			/* Perform this ioctl in a separate function */
			ret = pcidriver_umem_sync( privdata, &uhandle );
			if (ret) return ret;

			break;

		case PCIDRIVER_IOC_WAITI:
#ifdef ENABLE_IRQ
			if (arg >= PCIDRIVER_INT_MAXSOURCES)
				return -EFAULT;						/* User tried to overrun the IRQ_SOURCES array */

			irq_source = arg;		/*arg is directly the irq source to wait for */

			/* Thanks to Joern for the correction and tips! */ 
			temp=1;
			while (temp) {
				wait_event_interruptible_timeout( (privdata->irq_queues[irq_source]), (atomic_read(&(privdata->irq_outstanding[irq_source])) > 0), (10*HZ/1000) );

				if (atomic_add_negative( -1, &(privdata->irq_outstanding[irq_source])) )
					atomic_inc( &(privdata->irq_outstanding[irq_source]) );
				else 
					temp =0;
			}
			
			/* It is ok, nothing more to do. Continue and exit */			
#else
			mod_info("Asked to wait for interrupt but interrupts are not enabled in the driver\n");
			return -EFAULT;
#endif
			break;

		case PCIDRIVER_IOC_CLEAR_IOQ:
#ifdef ENABLE_IRQ
			if (arg >= PCIDRIVER_INT_MAXSOURCES)
				return -EFAULT;						/* User tried to overrun the IRQ_SOURCES array */

			irq_source = arg;		/*arg is directly the irq source number */
			atomic_set( &(privdata->irq_outstanding[irq_source]), 0 );
			
#else
			mod_info("Asked to wait for interrupt but interrupts are not enabled in the driver\n");
			return -EFAULT;
#endif
			break;
			
		default:
			return -EINVAL;
	}
	
	return 0;
}

/*************************************************************************/
/* Internal driver functions */
int pcidriver_mmap_pci( pcidriver_privdata_t *privdata, struct vm_area_struct *vmap , int bar )
{
	int ret = 0;
	unsigned long bar_addr;
	unsigned long bar_length, vma_size;
	unsigned long bar_flags;
	
	mod_info_dbg("Entering mmap_pci\n");
	
	/* Get info of the BAR to be mapped */
	bar_addr = pci_resource_start( privdata->pdev, bar );
	bar_length = pci_resource_len( privdata->pdev, bar );
	bar_flags = pci_resource_flags( privdata->pdev, bar );

	/* Check sizes */
	vma_size = (vmap->vm_end - vmap->vm_start);
	if ( (vma_size != bar_length) &&
	   ((bar_length < PAGE_SIZE) && (vma_size != PAGE_SIZE)) ) {
		mod_info( "mmap size is not correct! bar: %lu - vma: %lu\n", bar_length, vma_size );
		return -EINVAL;
	}

	if (bar_flags & IORESOURCE_IO) {
		/* Unlikely case, we will mmap a IO region */

		/* IO regions are never cacheable */
#ifdef pgprot_noncached
		vmap->vm_page_prot = pgprot_noncached(vmap->vm_page_prot);
#endif

		/* Map the BAR */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,12)
		ret = io_remap_pfn_range( 
					vmap, 
					vmap->vm_start, 
					(bar_addr >> PAGE_SHIFT), 
					bar_length, 
					vmap->vm_page_prot );
#else
		ret = io_remap_page_range( 
					vmap, 
					vmap->vm_start, 
					bar_addr, 
					bar_length, 
					vmap->vm_page_prot );
#endif

	} else {

		/* Normal case, mmap a memory region */

		/* Ensure this VMA is non-cached, if it is not flaged as prefetchable.
		 * If it is prefetchable, caching is allowed and will give better performance.
		 * This should be set properly by the BIOS, but we want to be sure. */
		/* adapted from drivers/char/mem.c, mmap function. */ 
#ifdef pgprot_noncached
/* Setting noncached disables MTRR registers, and we want to use them.
 * So we take this code out. This can lead to caching problems if and only if
 * the System BIOS set something wrong. Check LDDv3, page 425.
 */
//		if (!(bar_flags & IORESOURCE_PREFETCH))
//			vmap->vm_page_prot = pgprot_noncached(vmap->vm_page_prot);
#endif
	
		/* Map the BAR */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10)
		ret = remap_pfn_range( 
					vmap, 
					vmap->vm_start, 
					(bar_addr >> PAGE_SHIFT), 
					bar_length,
					vmap->vm_page_prot );
#else
		ret = remap_page_range( 
					vmap, 
					vmap->vm_start, 
					bar_addr, 
					bar_length, 
					vmap->vm_page_prot );
#endif	
	}

	if (ret) {
		mod_info("remap_pfn_range failed\n");
		return -EAGAIN;
	}
	
	return 0;	/* success */
}

int pcidriver_mmap_kmem( pcidriver_privdata_t *privdata, struct vm_area_struct *vma )
{
	unsigned long vma_size;
	pcidriver_kmem_entry_t *kmem_entry;
	int ret;
	
	mod_info_dbg("Entering mmap_kmem\n");

	/* Get latest entry on the kmem_list */	
	spin_lock( &(privdata->kmemlist_lock) );
	if (list_empty( &(privdata->kmem_list) )) {
		spin_unlock( &(privdata->kmemlist_lock) );
		mod_info( "Trying to mmap a kernel memory buffer without creating it first!\n" );
		return -EFAULT;
	}
	kmem_entry = list_entry(privdata->kmem_list.prev, pcidriver_kmem_entry_t, list );
	spin_unlock( &(privdata->kmemlist_lock) );

	mod_info_dbg("Got kmem_entry with id: %d\n", kmem_entry->id );

	/* Check sizes */
	vma_size = (vma->vm_end - vma->vm_start);
	if ( (vma_size != kmem_entry->size) &&
		((kmem_entry->size < PAGE_SIZE) && (vma_size != PAGE_SIZE)) ) {
		mod_info("kem_entry size(%lu) and vma size do not match(%lu)\n",kmem_entry->size,vma_size);
		return -EINVAL;
	}
	/* FIXME: Do I need to check extra permissions? */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,15)
	vma->vm_flags |= (VM_RESERVED);
#else
	vma->vm_flags |= (VM_PFNMAP);
#endif

#if 0
	/*
	 * This is DISABLED.
	 * Used only together the nopage method to map the memory pages,
	 * but creates stability problems when the App crashes. Therefore,
	 * this method is deprecated.
	 */
	/* Set VM operations */

	vma->vm_private_data = kmem_entry;
	vma->vm_ops = &pcidriver_vm_operations;
	pcidriver_vm_open(vma);

	return 0;	/* Success */
#endif

#ifdef pgprot_noncached
	// This is coherent memory, so it must not be cached.
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
#endif

	/* Map the Buffer to the VMA */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10)
	ret = remap_pfn_range(
					vma,
					vma->vm_start,
					__pa(kmem_entry->cpua) >> PAGE_SHIFT,
					kmem_entry->size,
					vma->vm_page_prot );
#else
	ret = remap_page_range(
					vma,
					vma->vm_start,
					__pa(kmem_entry->cpua),
					kmem_entry->size,
					vma->vm_page_prot );
#endif

	if (ret) {
		mod_info("kmem remap failed: %d\n", ret);
		return -EAGAIN;
	}
		
	return ret;
}

int pcidriver_pci_read( pcidriver_privdata_t *privdata, pci_cfg_cmd *pci_cmd )
{
	int ret = 0;
	
	switch (pci_cmd->size) {
		case PCIDRIVER_PCI_CFG_SZ_BYTE:
			ret = pci_read_config_byte( privdata->pdev, pci_cmd->addr, &(pci_cmd->val.byte) );
			break;
		case PCIDRIVER_PCI_CFG_SZ_WORD:
			ret = pci_read_config_word( privdata->pdev, pci_cmd->addr, &(pci_cmd->val.word) );
			break;
		case PCIDRIVER_PCI_CFG_SZ_DWORD:
			ret = pci_read_config_dword( privdata->pdev, pci_cmd->addr, &(pci_cmd->val.dword) );
			break;
		default:
			return -EINVAL;		/* Wrong size setting */
			break;
	}
	
	return ret;
}

int pcidriver_pci_write( pcidriver_privdata_t *privdata, pci_cfg_cmd *pci_cmd )
{
	int ret = 0;
	
	switch (pci_cmd->size) {
		case PCIDRIVER_PCI_CFG_SZ_BYTE:
			ret = pci_write_config_byte( privdata->pdev, pci_cmd->addr, pci_cmd->val.byte );
			break;
		case PCIDRIVER_PCI_CFG_SZ_WORD:
			ret = pci_write_config_word( privdata->pdev, pci_cmd->addr, pci_cmd->val.word );
			break;
		case PCIDRIVER_PCI_CFG_SZ_DWORD:
			ret = pci_write_config_dword( privdata->pdev, pci_cmd->addr, pci_cmd->val.dword );
			break;
		default:
			return -EINVAL;		/* Wrong size setting */
			break;
	}
	
	return ret;
}

int pcidriver_pci_info( pcidriver_privdata_t *privdata, pci_board_info *pci_info )
{
	int ret = 0;
	int bar;
	
	pci_info->vendor_id = privdata->pdev->vendor;
	pci_info->device_id = privdata->pdev->device;
	pci_info->bus = privdata->pdev->bus->number;
	pci_info->slot = PCI_SLOT(privdata->pdev->devfn);
	pci_info->devfn = privdata->pdev->devfn;
	
	ret = pci_read_config_byte(privdata->pdev, PCI_INTERRUPT_PIN, &(pci_info->interrupt_pin) );
	if (ret)
		return ret;

	ret = pci_read_config_byte(privdata->pdev, PCI_INTERRUPT_LINE, &(pci_info->interrupt_line) );
	if (ret)
		return ret;
	
	for(bar=0;bar<6;bar++) {
		pci_info->bar_start[ bar ] = pci_resource_start( privdata->pdev, bar );
		pci_info->bar_length[ bar ] = pci_resource_len( privdata->pdev, bar );		
	}
	
	return 0; /* Success */
}



int pcidriver_kmem_alloc(pcidriver_privdata_t *privdata, kmem_handle_t *kmem_handle )
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
	int temp;
	char temps[50];
#endif
	pcidriver_kmem_entry_t *kmem_entry;
	void *retptr;
	unsigned long start, end, i;
	struct page *kpage;
	
	/* First, allocate memory for the kmem_entry */
	kmem_entry = (pcidriver_kmem_entry_t *)kmalloc(sizeof(pcidriver_kmem_entry_t), GFP_KERNEL);
	if (!kmem_entry)
		goto kmem_alloc_entry_fail;
		
	memset( kmem_entry, 0, sizeof( *kmem_entry ) );
	
	/* ...and allocate the DMA memory */
	/* note this is a memory pair, referencing the same area: the cpu address (cpua) 
	 * and the PCI bus address (pa). The CPU and PCI addresses may not be the same. 
	 * The CPU sees only CPU addresses, while the device sees only PCI addresses.
	 * CPU address is used for the mmap (internal to the driver), and
	 * PCI address is the address passed to the DMA Controller in the device.
	 */
	retptr = pci_alloc_consistent( privdata->pdev, kmem_handle->size, &(kmem_entry->dma_handle) );
	if (retptr == NULL)
		goto kmem_alloc_mem_fail;
	kmem_entry->cpua = (unsigned long)retptr;
	kmem_handle->pa = (unsigned long)(kmem_entry->dma_handle);
	
	/* initialize some values... */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,9)
	kmem_entry->id = atomic_inc_return(&privdata->kmem_count) - 1;
#else
	atomic_inc(&privdata->kmem_count);
	kmem_entry->id = atomic_read(&privdata->kmem_count) - 1;
#endif
	kmem_entry->size = kmem_handle->size;
	kmem_handle->handle_id = kmem_entry->id;


#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,15)
	/*
	 * This is no longer needed starting in kernel 2.6.15,
	 * as it removed the PG_RESERVED flag bit. 
	 * 
	 * This is documented in the kernel change log, you can also check
	 * the following links:
	 * 
	 * http://lwn.net/Articles/161204/
	 * http://lists.openfabrics.org/pipermail/general/2007-March/034101.html
	 */

	/*
	 *  Go over the pages of the kmem buffer, and mark them as reserved.
	 * This is needed, otherwise mmaping the kernel memory to user space
	 * will fail silently (mmaping /dev/null) when using remap_xx_range.
	 */
	start = __pa(kmem_entry->cpua) >> PAGE_SHIFT;
	end = __pa(kmem_entry->cpua + kmem_entry->size) >> PAGE_SHIFT;
	if (start == end) {
		kpage = pfn_to_page(start);
		SetPageReserved(kpage);
	} else {
		for(i=start;i<end;i++) {
			kpage = pfn_to_page(i);
			SetPageReserved(kpage);
		}
	}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
	/* allocate space for the name of the attribute */
	snprintf(temps,50,"kbuf%d",kmem_entry->id);
	temp = strlen(temps)+1;
	kmem_entry->sysfs_attr.attr.name = (char*)kmalloc(sizeof(char*)*temp, GFP_KERNEL);
	if (!kmem_entry->sysfs_attr.attr.name)
		goto kmem_alloc_name_fail;
#endif
				
	/* Add the kmem_entry to the list of the device */
	spin_lock( &(privdata->kmemlist_lock) );
	list_add_tail( &(kmem_entry->list), &(privdata->kmem_list) );
	spin_unlock( &(privdata->kmemlist_lock) );
			
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
	/* Add a sysfs attribute for the kmem buffer */
	/* Does not makes sense for <2.6.13, as we have no mmap support before this version */
	kmem_entry->sysfs_attr.attr.mode = S_IRUGO;
	kmem_entry->sysfs_attr.attr.owner = THIS_MODULE;
	kmem_entry->sysfs_attr.show = pcidriver_show_kmem_entry;
	kmem_entry->sysfs_attr.store = NULL;
			
	/* name and add attribute */
	sprintf(kmem_entry->sysfs_attr.attr.name,"kbuf%d",kmem_entry->id);
	class_device_create_file( privdata->class_dev, &(kmem_entry->sysfs_attr) );
#endif

	return 0;	/* success */
	
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
kmem_alloc_name_fail:
		kfree( kmem_entry->sysfs_attr.attr.name );
		pci_free_consistent(privdata->pdev, kmem_entry->size, (void *)(kmem_entry->cpua), kmem_entry->dma_handle );
#endif
kmem_alloc_mem_fail:
		kfree(kmem_entry);
kmem_alloc_entry_fail:
		return -ENOMEM;
}

int pcidriver_kmem_free( pcidriver_privdata_t *privdata, kmem_handle_t *kmem_handle )
{
	pcidriver_kmem_entry_t *kmem_entry;

	/* Find the associated kmem_entry for this buffer */
	kmem_entry = pcidriver_kmem_find_entry( privdata, kmem_handle );
	if (kmem_entry == NULL)
		return -EINVAL;					/* kmem_handle is not valid */

	return pcidriver_kmem_free_entry( privdata, kmem_entry );
}

int pcidriver_kmem_free_all(  pcidriver_privdata_t *privdata )
{
	struct list_head *ptr, *next;
	pcidriver_kmem_entry_t *kmem_entry;
	
	/* iterate safely over the entries and delete them */
	list_for_each_safe( ptr, next, &(privdata->kmem_list) ) {
		kmem_entry = list_entry(ptr, pcidriver_kmem_entry_t, list );
		pcidriver_kmem_free_entry( privdata, kmem_entry ); 		/* spin lock inside! */
	}

	return 0;
	
}

int pcidriver_kmem_sync( pcidriver_privdata_t *privdata, kmem_sync_t *kmem_sync )
{
	pcidriver_kmem_entry_t *kmem_entry;

	/* Find the associated kmem_entry for this buffer */
	kmem_entry = pcidriver_kmem_find_entry( privdata, &(kmem_sync->handle) );
	if (kmem_entry == NULL)
		return -EINVAL;					/* kmem_handle is not valid */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,11)
	switch (kmem_sync->dir) {
		case PCIDRIVER_DMA_TODEVICE:
			pci_dma_sync_single_for_device( privdata->pdev, kmem_entry->dma_handle, kmem_entry->size, PCI_DMA_TODEVICE );
			break;
		case PCIDRIVER_DMA_FROMDEVICE:
			pci_dma_sync_single_for_cpu( privdata->pdev, kmem_entry->dma_handle, kmem_entry->size, PCI_DMA_FROMDEVICE );
			break;
		case PCIDRIVER_DMA_BIDIRECTIONAL:
			pci_dma_sync_single_for_device( privdata->pdev, kmem_entry->dma_handle, kmem_entry->size, PCI_DMA_BIDIRECTIONAL );
			pci_dma_sync_single_for_cpu( privdata->pdev, kmem_entry->dma_handle, kmem_entry->size, PCI_DMA_BIDIRECTIONAL );
			break;
		default:
			return -EINVAL;				/* wrong direction parameter */
	}
#else
	switch (kmem_sync->dir) {
		case PCIDRIVER_DMA_TODEVICE:
			pci_dma_sync_single( privdata->pdev, kmem_entry->dma_handle, kmem_entry->size, PCI_DMA_TODEVICE );
			break;
		case PCIDRIVER_DMA_FROMDEVICE:
			pci_dma_sync_single( privdata->pdev, kmem_entry->dma_handle, kmem_entry->size, PCI_DMA_FROMDEVICE );
			break;
		case PCIDRIVER_DMA_BIDIRECTIONAL:
			pci_dma_sync_single( privdata->pdev, kmem_entry->dma_handle, kmem_entry->size, PCI_DMA_BIDIRECTIONAL );
			break;
		default:
			return -EINVAL;				/* wrong direction parameter */
	}
#endif

	return 0;	/* success */
}

int pcidriver_kmem_free_entry( pcidriver_privdata_t *privdata, pcidriver_kmem_entry_t *kmem_entry )
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
	/* Remove the sysfs attribute */
	class_device_remove_file( privdata->class_dev, &(kmem_entry->sysfs_attr) );
	kfree( kmem_entry->sysfs_attr.attr.name );
#endif


	/* Go over the pages of the kmem buffer, and mark them as not reserved */
#if 0
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,15)
	/*
	 * This code is DISABLED.
	 * Apparently, it is not needed to unreserve them. Doing so here
	 * hangs the machine. Why?
	 * 
	 * Uhm.. see links:
	 * 
	 * http://lwn.net/Articles/161204/
	 * http://lists.openfabrics.org/pipermail/general/2007-March/034101.html
	 * 
	 * I insist, this should be enabled, but doing so hangs the machine.
	 * Literature supports the point, and there is even a similar problem (see link)
	 * But this is not the case. It seems right to me. but obviously is not.
	 * 
	 * Anyway, this goes away in kernel >=2.6.15.
	 */
	unsigned long start = __pa(kmem_entry->cpua) >> PAGE_SHIFT;
	unsigned long end = __pa(kmem_entry->cpua + kmem_entry->size) >> PAGE_SHIFT;
	unsigned long i;
	for(i=start;i<end;i++) {
		struct page *kpage = pfn_to_page(i);
		ClearPageReserved(kpage);
	}	
#endif
#endif		

	/* Release DMA memory */
	pci_free_consistent( privdata->pdev, kmem_entry->size, (void *)(kmem_entry->cpua), kmem_entry->dma_handle );

	/* Remove the kmem list entry */
	spin_lock( &(privdata->kmemlist_lock) );
	list_del( &(kmem_entry->list) );
	spin_unlock( &(privdata->kmemlist_lock) );

	/* Release kmem_entry memory */
	kfree(kmem_entry);
	
	return 0;
}

pcidriver_kmem_entry_t *pcidriver_kmem_find_entry(pcidriver_privdata_t *privdata, kmem_handle_t *kmem_handle ) {
	struct list_head *ptr;
	pcidriver_kmem_entry_t *entry;

	/* should I implement it better using the handle_id? */
	
	spin_lock( &(privdata->kmemlist_lock) );
	list_for_each( ptr, &(privdata->kmem_list) ) {
		entry = list_entry(ptr, pcidriver_kmem_entry_t, list );
		
		if (entry->dma_handle == kmem_handle->pa) {
			spin_unlock( &(privdata->kmemlist_lock) );
			return entry;
		}
	}
	spin_unlock( &(privdata->kmemlist_lock) );
	return NULL;
}

pcidriver_kmem_entry_t *pcidriver_kmem_find_entry_id(pcidriver_privdata_t *privdata, int id ) {
	struct list_head *ptr;
	pcidriver_kmem_entry_t *entry;
	
	spin_lock( &(privdata->kmemlist_lock) );
	list_for_each( ptr, &(privdata->kmem_list) ) {
		entry = list_entry(ptr, pcidriver_kmem_entry_t, list );
		
		if (entry->id == id) {
			spin_unlock( &(privdata->kmemlist_lock) );
			return entry;
		}
	}

	spin_unlock( &(privdata->kmemlist_lock) );
	return NULL;
}


int pcidriver_umem_sgmap( pcidriver_privdata_t *privdata, umem_handle_t *umem_handle )
{
  
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
	int temp;
	char temps[50];
#endif
	int i,res;
	struct page **pages;
	struct scatterlist *sg;
	pcidriver_umem_entry_t *umem_entry;
	unsigned int nents,nr_pages;
	unsigned long count;

	/*
	 * We do some checks first. Then, the following is necessary to create a
	 * Scatter/Gather list from a user memory area:
	 *  - Determine the number of pages
	 *  - Get the pages for the memory area
	 * 	- Lock them.
	 *  - Create a scatter/gather list of the pages 
	 *  - Map the list from memory to PCI bus addresses
	 * 
	 * Then, we:
	 *  - Create an entry on the umem list of the device, to cache the mapping.
	 *  - Create a sysfs attribute that gives easy access to the SG list
	 */

	/* User tryed an overflow! */
	if ((umem_handle->vma + umem_handle->size) < umem_handle->vma)
		return -EINVAL;
	
	/* zero-size?? */
	if (umem_handle->size == 0)
		return -EINVAL;

	/* Direction is better ignoring during mapping. */
	/* We assume bidirectional buffers always, except when sync'ing */

	/* calculate the number of pages */
	nr_pages = ((umem_handle->vma & ~PAGE_MASK) + umem_handle->size + ~PAGE_MASK) >> PAGE_SHIFT;

	/* Allocate space for the page information */
	/* This can be very big, so we use vmalloc */
	pages = vmalloc( nr_pages * sizeof(*pages) );
	if (pages == NULL)
		return -ENOMEM;

	/* Allocate space for the scatterlist */
	/* We do not know how many entries will be, but the maximum is nr_pages. */
	/* This can be very big, so we use vmalloc */
	sg = vmalloc( nr_pages * sizeof(*sg) );
	if (sg == NULL)
		goto umem_sgmap_pages;

  //mod_info( "\npcidriver_umem_sgmap for address %x, size=%d , sg=%x\n", umem_handle->vma, umem_handle->size,sg);
 
	/* Get the page information */
	down_read(&current->mm->mmap_sem);
	res = get_user_pages(
				current,
				current->mm,
				umem_handle->vma,
				nr_pages,
				1,
				0,  /* do not force, FIXME: shall I? */
				pages,
				NULL );				
	up_read(&current->mm->mmap_sem);

	/* Error, not all pages mapped */
	if (res < nr_pages) {
		mod_info("Could not map all user pages (%d of %d)\n",res,nr_pages);
		nr_pages = res;
		goto umem_sgmap_unmap;
	}

	/* Lock the pages, then populate the SG list with the pages */
	sg[0].page = pages[0];
	sg[0].offset = (umem_handle->vma & ~PAGE_MASK);
	sg[0].length = (umem_handle->size > (PAGE_SIZE-sg[0].offset)) ? (PAGE_SIZE-sg[0].offset) : umem_handle->size;
	count = umem_handle->size - sg[0].length;
	/**Added JA: fix lock also first page!***/
   if ( !PageReserved(pages[0]) )
         {
			SetPageLocked(pages[0]);
			//mod_info( " -- SetPageLocked #%d for page %x \n", 0, pages[0]);
         }
      else
        mod_info( "!!!! pcidriver_umem_sgmap-- SetPageLocked #%d finds reserved page!!!", 0);

   
   /*****/
   for(i=1;i<nr_pages;i++) {
		/* Lock page first */
		if ( !PageReserved(pages[i]) )
         {
			SetPageLocked(pages[i]);
			//mod_info( " -- SetPageLocked #%d for page %x \n", i, pages[i]);
         }
      else
        mod_info( "!!!! pcidriver_umem_sgmap-- SetPageLocked #%d finds reserved page!!!", i);

         
		/* Populate the list */
		sg[i].page = pages[i];
		sg[i].offset = 0;
		sg[i].length = (count > PAGE_SIZE) ? PAGE_SIZE : count;
		count -= sg[i].length;
	}

	/* Use the page list to populate the SG list */
	/* SG entries may be merged, res is the number of used entries */
	/* We have originally nr_pages entries in the sg list */
	res = pci_map_sg( privdata->pdev, sg, nr_pages, PCI_DMA_BIDIRECTIONAL );
	nents = res;

	/* An error */
	if (nents == 0)
		goto umem_sgmap_unmap;

	/* Add an entry to the umem_list of the device, and update the handle with the id */
	/* Allocate space for the new umem entry */
	umem_entry = kmalloc( sizeof(*umem_entry), GFP_KERNEL);
	if (umem_entry == NULL)
		goto umem_sgmap_entry;

	/* Fill entry to be added to the umem list */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,9)
	umem_entry->id = atomic_inc_return(&privdata->umem_count) - 1;
#else
	atomic_inc(&privdata->umem_count);
	umem_entry->id = atomic_read(&privdata->umem_count) - 1;
#endif
	umem_entry->nr_pages = nr_pages;	/* Will be needed when unmapping */
	umem_entry->pages = pages;
	umem_entry->nents = nents;	
	umem_entry->sg = sg;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
	/* allocate space for the name of the attribute */
	snprintf(temps,50,"umem%d",umem_entry->id);
	temp = strlen(temps)+1;
	umem_entry->sysfs_attr.attr.name = (char*)kmalloc(sizeof(char*)*temp, GFP_KERNEL);
	if (!umem_entry->sysfs_attr.attr.name)
		goto umem_sgmap_name_fail;
#endif

	/* Add entry to the umem list */
	spin_lock( &(privdata->umemlist_lock) );
	list_add_tail( &(umem_entry->list), &(privdata->umem_list) );
	spin_unlock( &(privdata->umemlist_lock) );
	
	/* Update the Handle with the Handle ID of the entry */
	umem_handle->handle_id = umem_entry->id;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
	/* Add a sysfs attribute for the umem entry */
	umem_entry->sysfs_attr.attr.mode = S_IRUGO;
	umem_entry->sysfs_attr.attr.owner = THIS_MODULE;
	umem_entry->sysfs_attr.show = pcidriver_show_umem_entry;
	umem_entry->sysfs_attr.store = NULL;
			
	/* name and add attribute */
	sprintf(umem_entry->sysfs_attr.attr.name,"umem%d",umem_entry->id);
	class_device_create_file( privdata->class_dev, &(umem_entry->sysfs_attr) );
#endif
	
	return 0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
umem_sgmap_name_fail:
	kfree(umem_entry);
#endif
umem_sgmap_entry:
	pci_unmap_sg( privdata->pdev, sg, nr_pages, PCI_DMA_BIDIRECTIONAL );
umem_sgmap_unmap:
	/* release pages */
	if (nr_pages > 0) {
		for(i=0;i<nr_pages;i++)
			page_cache_release(pages[i]);
	}
	vfree(sg);
umem_sgmap_pages:
	vfree(pages);
	return -ENOMEM;

}

int pcidriver_umem_sgunmap( pcidriver_privdata_t *privdata, pcidriver_umem_entry_t *umem_entry )
{
	int i;
  //mod_info( "\npcidriver_umem_sgunmap for sg %x,\n", umem_entry->sg);
 
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
	/* Remove the sysfs attribute */
	class_device_remove_file( privdata->class_dev, &(umem_entry->sysfs_attr) );
	kfree( umem_entry->sysfs_attr.attr.name );
#endif
			
	/* Unmap user memory */
	pci_unmap_sg( privdata->pdev, umem_entry->sg, umem_entry->nr_pages, PCI_DMA_BIDIRECTIONAL );

	/* Release the pages */
	if (umem_entry->nr_pages > 0) {
		for(i=0;i<(umem_entry->nr_pages);i++) {			
			/* Mark pages as Dirty and unlock it */
			if ( !PageReserved( umem_entry->pages[i] )) {
				SetPageDirty( umem_entry->pages[i] );
				ClearPageLocked( umem_entry->pages[i] );
			}
         else
            {
              mod_info( "!!!!!! Page #%d was reserved, not clear page lock!", i, umem_entry->pages[i]);
            }
			/* and release it from the cache */
			page_cache_release( umem_entry->pages[i] );
         //mod_info( " -- page_cache_release #%d for page %x \n", i, umem_entry->pages[i]);

      }
	}

	/* Remove the umem list entry */
	spin_lock( &(privdata->umemlist_lock) );
	list_del( &(umem_entry->list) );
	spin_unlock( &(privdata->umemlist_lock) );

	/* Release SG list and page list memory */
	/* These two are in the vm area of the kernel */
	vfree( umem_entry->pages );
	vfree( umem_entry->sg );

	/* Release umem_entry memory */
	kfree(umem_entry);
	
	return 0;
}

int pcidriver_umem_sgunmap_all( pcidriver_privdata_t *privdata )
{
	struct list_head *ptr, *next;
	pcidriver_umem_entry_t *umem_entry;
	
	/* iterate safely over the entries and delete them */
	list_for_each_safe( ptr, next, &(privdata->umem_list) ) {
		umem_entry = list_entry(ptr, pcidriver_umem_entry_t, list );
		pcidriver_umem_sgunmap( privdata, umem_entry ); 		/* spin lock inside! */
	}

	return 0;
}

int pcidriver_umem_sgget( pcidriver_privdata_t *privdata, umem_sglist_t *umem_sglist )
{
	int i;
	pcidriver_umem_entry_t *umem_entry;
	struct scatterlist *sg;
	int idx;
	dma_addr_t cur_addr;
	unsigned int cur_size;
	
	/* Find the associated umem_entry for this buffer */
	umem_entry = pcidriver_umem_find_entry_id( privdata, umem_sglist->handle_id );
	if (umem_entry == NULL)
		return -EINVAL;					/* umem_handle is not valid */

	/* Check if passed SG list is enough */
	if (umem_sglist->nents < umem_entry->nents)
		return -EINVAL;					/* sg has not enough entries */

	/* Copy the SG list to the user format */
	if (umem_sglist->type == PCIDRIVER_SG_MERGED) {
		/* Merge entries that are contiguous into a single entry */
		/* Non-optimal but fast for most cases */
		/* First one always true */
		sg=umem_entry->sg;
		umem_sglist->sg[0].addr = sg_dma_address( sg );
		umem_sglist->sg[0].size = sg_dma_len( sg );
		sg++;
		idx = 0;
	
		/* Iterate over the SG entries */
		for(i=1; i< umem_entry->nents; i++, sg++ ) {
			cur_addr = sg_dma_address( sg );
			cur_size = sg_dma_len( sg );

			/* Check if entry fits after current entry */
			if (cur_addr == (umem_sglist->sg[idx].addr + umem_sglist->sg[idx].size)) {
				umem_sglist->sg[idx].size += cur_size;
				continue;
			}
		
			/* Check if entry fits before current entry */
			if ((cur_addr + cur_size) == umem_sglist->sg[idx].addr) {
				umem_sglist->sg[idx].addr = cur_addr;
				umem_sglist->sg[idx].size += cur_size;			
				continue;
			}
		
			/* None of the above, add new entry */
			idx++;
			umem_sglist->sg[idx].addr = cur_addr;
			umem_sglist->sg[idx].size = cur_size;
		}
		/* Set the used size of the SG list */
		umem_sglist->nents = idx+1;
	}
	else {	
		/* Assume pci_map_sg made a good job (ehem..) and just copy it.
		 * actually, now I assume it just gives them plainly to me. */
		for(i=0, sg=umem_entry->sg ; i< umem_entry->nents; i++, sg++ ) {
			umem_sglist->sg[i].addr = sg_dma_address( sg );
			umem_sglist->sg[i].size = sg_dma_len( sg );
		}
	}

	return 0;
}

int pcidriver_umem_sync( pcidriver_privdata_t *privdata, umem_handle_t *umem_handle )
{
	pcidriver_umem_entry_t *umem_entry;

	/* Find the associated umem_entry for this buffer */
	umem_entry = pcidriver_umem_find_entry_id( privdata, umem_handle->handle_id );
	if (umem_entry == NULL)
		return -EINVAL;					/* umem_handle is not valid */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,11)
	switch (umem_handle->dir) {
		case PCIDRIVER_DMA_TODEVICE:
			pci_dma_sync_sg_for_device( privdata->pdev, umem_entry->sg, umem_entry->nents, PCI_DMA_TODEVICE );
			break;
		case PCIDRIVER_DMA_FROMDEVICE:
			pci_dma_sync_sg_for_cpu( privdata->pdev, umem_entry->sg, umem_entry->nents, PCI_DMA_FROMDEVICE );
			break;
		case PCIDRIVER_DMA_BIDIRECTIONAL:
			pci_dma_sync_sg_for_device( privdata->pdev, umem_entry->sg, umem_entry->nents, PCI_DMA_BIDIRECTIONAL );
			pci_dma_sync_sg_for_cpu( privdata->pdev, umem_entry->sg, umem_entry->nents, PCI_DMA_BIDIRECTIONAL );
			break;
		default:
			return -EINVAL;				/* wrong direction parameter */
	}
#else
	switch (umem_handle->dir) {
		case PCIDRIVER_DMA_TODEVICE:
			pci_dma_sync_sg( privdata->pdev, umem_entry->sg, umem_entry->nents, PCI_DMA_TODEVICE );
			break;
		case PCIDRIVER_DMA_FROMDEVICE:
			pci_dma_sync_sg( privdata->pdev, umem_entry->sg, umem_entry->nents, PCI_DMA_FROMDEVICE );
			break;
		case PCIDRIVER_DMA_BIDIRECTIONAL:
			pci_dma_sync_sg( privdata->pdev, umem_entry->sg, umem_entry->nents, PCI_DMA_BIDIRECTIONAL );
			break;
		default:
			return -EINVAL;				/* wrong direction parameter */
	}
#endif

	return 0;
}

pcidriver_umem_entry_t *pcidriver_umem_find_entry_id( pcidriver_privdata_t *privdata, int id )
{
	struct list_head *ptr;
	pcidriver_umem_entry_t *entry;
	
	spin_lock( &(privdata->umemlist_lock) );
	list_for_each( ptr, &(privdata->umem_list) ) {
		entry = list_entry(ptr, pcidriver_umem_entry_t, list );
		
		if (entry->id == id) {
			spin_unlock( &(privdata->umemlist_lock) );
			return entry;
		}
	}

	spin_unlock( &(privdata->umemlist_lock) );
	return NULL;
}


#ifdef ENABLE_IRQ
/*************************************************************************/
/* IRQ Handler function */
irqreturn_t pcidriver_irq_handler( int irq, void *dev_id, struct pt_regs *regs)
{
	pcidriver_privdata_t *privdata = (pcidriver_privdata_t *)dev_id;

	if ( pcidriver_irq_acknowledge(privdata) != 0) {
		privdata->irq_count++;
		return IRQ_HANDLED;
	}
	else
		return IRQ_NONE;	
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,11)
/* Copied verbatim from linux-2.6.15/kernel/sched.c */

int fastcall __sched wait_for_completion_interruptible(struct completion *x)
{
	int ret = 0;

	might_sleep();

	spin_lock_irq(&x->wait.lock);
	if (!x->done) {
		DECLARE_WAITQUEUE(wait, current);

		wait.flags |= WQ_FLAG_EXCLUSIVE;
		__add_wait_queue_tail(&x->wait, &wait);
		do {
			if (signal_pending(current)) {
				ret = -ERESTARTSYS;
				__remove_wait_queue(&x->wait, &wait);
				goto out;
			}
			__set_current_state(TASK_INTERRUPTIBLE);
			spin_unlock_irq(&x->wait.lock);
			schedule();
			spin_lock_irq(&x->wait.lock);
		} while (!x->done);
		__remove_wait_queue(&x->wait, &wait);
	}
	x->done--;
out:
	spin_unlock_irq(&x->wait.lock);

	return ret;
}
#endif

/*********************************************
 Here we include the code from pciDriver_inc.c
**********************************************/
#include "pciDriver_int.c"

#endif

/*************************************************************************/
/* Sysfs functions */


#ifdef ENABLE_IRQ
static ssize_t pcidriver_show_irq_count(struct class_device *cls, char *buf)
{
	int temp;
	
	pcidriver_privdata_t *privdata = (pcidriver_privdata_t *)cls->class_data;

	/* output will be truncated to PAGE_SIZE */
	temp = snprintf(buf,PAGE_SIZE,"%d\n", privdata->irq_count );
	return (temp > PAGE_SIZE ? PAGE_SIZE : temp);
}

static ssize_t pcidriver_show_irq_queues(struct class_device *cls, char *buf)
{
	pcidriver_privdata_t *privdata = (pcidriver_privdata_t *)cls->class_data;
	int i, temp, offset;

	/* output will be truncated to PAGE_SIZE */
	offset = snprintf(buf,PAGE_SIZE,"Queue\tOutstanding IRQs\n");
	for(i=0;i<PCIDRIVER_INT_MAXSOURCES;i++) {
		temp = snprintf(buf+offset, PAGE_SIZE-offset, "%d\t%d\n", i, atomic_read(&(privdata->irq_outstanding[i])) );
		offset += temp;
	}
	return (offset > PAGE_SIZE ? PAGE_SIZE : offset+1);
}
#endif

static ssize_t pcidriver_show_mmap_mode(struct class_device *cls, char *buf)
{
	int temp;
	
	pcidriver_privdata_t *privdata = (pcidriver_privdata_t *)cls->class_data;

	/* output will be truncated to PAGE_SIZE */
	temp = sprintf(buf,"%d\n",privdata->mmap_mode);
	return (temp > PAGE_SIZE ? PAGE_SIZE : temp);
}

static ssize_t pcidriver_store_mmap_mode(struct class_device *cls, char *buf, size_t count)
{
	pcidriver_privdata_t *privdata = (pcidriver_privdata_t *)cls->class_data;
	int temp = -1;
	
	sscanf(buf,"%d",&temp);

	/* validate input */
	switch (temp) {
		case PCIDRIVER_MMAP_PCI:
		case PCIDRIVER_MMAP_KMEM:
			privdata->mmap_mode = temp;
			break;
		default:
			break;
	}	
	return strlen(buf);
}

static ssize_t pcidriver_show_mmap_area(struct class_device *cls, char *buf)
{
	int temp;
	
	pcidriver_privdata_t *privdata = (pcidriver_privdata_t *)cls->class_data;

	/* output will be truncated to PAGE_SIZE */
	temp = sprintf(buf,"%d\n",privdata->mmap_area);
	return (temp > PAGE_SIZE ? PAGE_SIZE : temp);
}

static ssize_t pcidriver_store_mmap_area(struct class_device *cls, char *buf, size_t count)
{
	pcidriver_privdata_t *privdata = (pcidriver_privdata_t *)cls->class_data;
	int temp = -1;
	
	sscanf(buf,"%d",&temp);

	/* validate input */
	switch (temp) {
		case PCIDRIVER_BAR0:
		case PCIDRIVER_BAR1:
		case PCIDRIVER_BAR2:
		case PCIDRIVER_BAR3:
		case PCIDRIVER_BAR4:
		case PCIDRIVER_BAR5:
			privdata->mmap_area = temp;
			break;
		default:
			break;
	}	
	return strlen(buf);
}

static ssize_t pcidriver_show_kmem_count(struct class_device *cls, char *buf)
{
	int temp;
	
	pcidriver_privdata_t *privdata = (pcidriver_privdata_t *)cls->class_data;

	/* output will be truncated to PAGE_SIZE */
	temp = snprintf(buf,PAGE_SIZE,"%d\n",atomic_read(&(privdata->kmem_count)));
	return (temp > PAGE_SIZE ? PAGE_SIZE : temp);
}

static ssize_t pcidriver_store_kmem_alloc(struct class_device *cls, char *buf, size_t count)
{
	pcidriver_privdata_t *privdata = (pcidriver_privdata_t *)cls->class_data;
	kmem_handle_t kmem_handle;

	/* FIXME: validate input */	
	sscanf(buf,"%lu",&kmem_handle.size);
	pcidriver_kmem_alloc(privdata, &kmem_handle );

	return strlen(buf);
}

static ssize_t pcidriver_store_kmem_free(struct class_device *cls, char *buf, size_t count)
{
	pcidriver_privdata_t *privdata = (pcidriver_privdata_t *)cls->class_data;
	int id;
	pcidriver_kmem_entry_t *kmem_entry;
	
	/* FIXME: validate input */		
	sscanf(buf,"%u",&id);
	if ((id < 0) || (id >= atomic_read(&(privdata->kmem_count))) )
		return strlen(buf);
	
	kmem_entry = pcidriver_kmem_find_entry_id(privdata,id);
	if (kmem_entry == NULL)
		return strlen(buf);
		
	pcidriver_kmem_free_entry(privdata, kmem_entry );
	return strlen(buf);
}

static ssize_t pcidriver_show_kbuffers(struct class_device *cls, char *buf)
{
	int temp, offset=0;
	pcidriver_privdata_t *privdata = (pcidriver_privdata_t *)cls->class_data;
	struct list_head *ptr;
	pcidriver_kmem_entry_t *entry;

	/* print the header */
	temp = snprintf(buf, PAGE_SIZE, "kbuf#\tcpu addr\tsize\n");
	offset += temp;
	
	spin_lock( &(privdata->kmemlist_lock) );
	list_for_each( ptr, &(privdata->kmem_list) ) {
		entry = list_entry(ptr, pcidriver_kmem_entry_t, list );
		
		/* print entry info */
		if (offset > PAGE_SIZE) {
			spin_unlock( &(privdata->kmemlist_lock) );
			return PAGE_SIZE;
		}
			
		temp = snprintf(buf+offset, PAGE_SIZE-offset, "%3d\t%08lx\t%lu\n", entry->id, (unsigned long)(entry->dma_handle), entry->size );
		offset += temp;
	}

	spin_unlock( &(privdata->kmemlist_lock) );

	/* output will be truncated to PAGE_SIZE */
	return (offset > PAGE_SIZE ? PAGE_SIZE : offset+1);
}

static ssize_t pcidriver_show_umappings(struct class_device *cls, char *buf)
{
	int temp, offset=0;
	pcidriver_privdata_t *privdata = (pcidriver_privdata_t *)cls->class_data;
	struct list_head *ptr;
	pcidriver_umem_entry_t *entry;

	/* print the header */
	temp = snprintf(buf, PAGE_SIZE, "umap#\tn_pages\tsg_ents\n");
	offset += temp;
	
	spin_lock( &(privdata->umemlist_lock) );
	list_for_each( ptr, &(privdata->umem_list) ) {
		entry = list_entry(ptr, pcidriver_umem_entry_t, list );
		
		/* print entry info */
		if (offset > PAGE_SIZE) {
			spin_unlock( &(privdata->umemlist_lock) );
			return PAGE_SIZE;
		}
			
		temp = snprintf(buf+offset, PAGE_SIZE-offset, "%3d\t%lu\t%lu\n", entry->id, (unsigned long)(entry->nr_pages), (unsigned long)(entry->nents) );
		offset += temp;
	}

	spin_unlock( &(privdata->umemlist_lock) );

	/* output will be truncated to PAGE_SIZE */
	return (offset > PAGE_SIZE ? PAGE_SIZE : offset+1);
}

static ssize_t pcidriver_store_umem_unmap(struct class_device *cls, char *buf, size_t count)
{
	pcidriver_privdata_t *privdata = (pcidriver_privdata_t *)cls->class_data;
	int id;
	pcidriver_umem_entry_t *umem_entry;
	
	/* FIXME: validate input */		
	sscanf(buf,"%u",&id);
	if ((id < 0) || (id >= atomic_read(&(privdata->umem_count))) )
		return strlen(buf);
	
	umem_entry = pcidriver_umem_find_entry_id(privdata,id);
	if (umem_entry == NULL)
		return strlen(buf);
		
	pcidriver_umem_sgunmap(privdata, umem_entry );
	return strlen(buf);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
/* Used only on kernels >=2.6.13 - not implemented yet */
static ssize_t pcidriver_show_kmem_entry(struct class_device *cls, char *buf)
{
	int temp;
	
	pcidriver_privdata_t *privdata = (pcidriver_privdata_t *)cls->class_data;

	/* output will be truncated to PAGE_SIZE */
	temp = snprintf(buf,PAGE_SIZE,"I am in the kmem_entry show function\n");
	return (temp > PAGE_SIZE ? PAGE_SIZE : temp);
}

static ssize_t pcidriver_show_umem_entry(struct class_device *cls, char *buf)
{
	int temp;
	
	pcidriver_privdata_t *privdata = (pcidriver_privdata_t *)cls->class_data;

	/* output will be truncated to PAGE_SIZE */
	temp = snprintf(buf,PAGE_SIZE,"I am in the umem_entry show function, class_device_kobj_name: %s\n",cls->kobj.name);
	return (temp > PAGE_SIZE ? PAGE_SIZE : temp);
}
#endif
