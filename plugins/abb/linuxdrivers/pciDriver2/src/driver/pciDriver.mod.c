#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
};

static const struct modversion_info ____versions[]
__attribute_used__
__attribute__((section("__versions"))) = {
	{ 0x9bf47d98, "struct_module" },
	{ 0xdbd8f75a, "cdev_del" },
	{ 0xe88515f, "pci_bus_read_config_byte" },
	{ 0x5a34a45c, "__kmalloc" },
	{ 0xfa33087f, "cdev_init" },
	{ 0x2c440944, "up_read" },
	{ 0xa61ebb64, "pci_release_region" },
	{ 0xd6ee688f, "vmalloc" },
	{ 0xc8b57c27, "autoremove_wake_function" },
	{ 0x6033e858, "class_device_destroy" },
	{ 0x25b8e225, "class_device_create" },
	{ 0xe8a9487, "malloc_sizes" },
	{ 0xb0241e8f, "pci_disable_device" },
	{ 0xcb68f1b3, "_spin_lock" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0x2fd1d81c, "vfree" },
	{ 0x5ce79464, "pci_bus_write_config_word" },
	{ 0x1d26aa98, "sprintf" },
	{ 0x51e77c97, "pfn_valid" },
	{ 0xe6e1a553, "down_read" },
	{ 0xa140a1a1, "pci_set_master" },
	{ 0xde0bdcff, "memset" },
	{ 0xeed53c3d, "pfn_to_page" },
	{ 0xdd132261, "printk" },
	{ 0x859204af, "sscanf" },
	{ 0x85f8a266, "copy_to_user" },
	{ 0x91f61e54, "dma_free_coherent" },
	{ 0xcc77c699, "class_device_create_file" },
	{ 0x135d06bc, "pci_bus_write_config_dword" },
	{ 0xea0fde2b, "class_create" },
	{ 0x9eac042a, "__ioremap" },
	{ 0x6934e930, "_spin_unlock" },
	{ 0x637bc548, "dma_alloc_coherent" },
	{ 0xd51d0dcc, "cdev_add" },
	{ 0xf5fc21d0, "pci_set_mwi" },
	{ 0xe8069e66, "kmem_cache_alloc" },
	{ 0x3762cb6e, "ioremap_nocache" },
	{ 0x6172ade, "pci_bus_read_config_word" },
	{ 0x405cf3f0, "pci_bus_read_config_dword" },
	{ 0xbd721b27, "class_device_remove_file" },
	{ 0x17750b6f, "get_user_pages" },
	{ 0x2cf190e3, "request_irq" },
	{ 0xd62c833f, "schedule_timeout" },
	{ 0x608555e1, "pci_unregister_driver" },
	{ 0xc43808db, "init_waitqueue_head" },
	{ 0xb7a3d11d, "__wake_up" },
	{ 0xece46dcc, "pci_bus_write_config_byte" },
	{ 0x37a0cba, "kfree" },
	{ 0xa5809a48, "remap_pfn_range" },
	{ 0xcaa48082, "prepare_to_wait" },
	{ 0xedc03953, "iounmap" },
	{ 0xa9393cfc, "__pci_register_driver" },
	{ 0x96958a89, "put_page" },
	{ 0xcf21f365, "class_destroy" },
	{ 0x8e4d809b, "finish_wait" },
	{ 0xaf25400d, "snprintf" },
	{ 0x5ee1210f, "pci_enable_device" },
	{ 0x3302b500, "copy_from_user" },
	{ 0x3d4f4661, "pci_request_region" },
	{ 0xe931e292, "dma_ops" },
	{ 0x29537c9e, "alloc_chrdev_region" },
	{ 0xf20dabd8, "free_irq" },
};

static const char __module_depends[]
__attribute_used__
__attribute__((section(".modinfo"))) =
"depends=";

MODULE_ALIAS("pci:v000010B5d00009656sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010DCd00000156sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001556d00001100sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010DCd00000153sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001679d00000001sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v00001679d00000005sv*sd*bc*sc*i*");

MODULE_INFO(srcversion, "BDE6085680D0C7CBF1D2A03");
