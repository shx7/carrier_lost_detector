/**********************************************************
*
*   PURPOSE: Hijacking netif_carrier_on function in kernel
*            by module function hijacked_netif_carrier_on
*
**********************************************************/ 
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/kprobes.h>

// Hardcoded addresses of netif function. See in /boot/System.map
#define NETIF_CARRIER_ON_ADDR  (unsigned char *)0xffffffff812A9093
#define NETIF_CARRIER_OFF_ADDR (unsigned char *)0XFFFFFFFF812A8DCB

// Count of bytes to be replaces as trampoline
#define PATCH_BYTES_COUNT 9

static unsigned char hijacking_bytes[PATCH_BYTES_COUNT] = {0};
static unsigned char orig_netif_on_bytes[PATCH_BYTES_COUNT] = {0};
static unsigned char orig_netif_off_bytes[PATCH_BYTES_COUNT] = {0};

static void hijacked_netif_carrier_on(struct net_device *dev) {
    printk(KERN_ALERT"[ANIMA]: netif_carrier_on hijacked\n");
    printk(KERN_ALERT"[ANIMA]: dev name \"%s\"\n", dev->name);
    printk(KERN_ALERT"[ANIMA]: dev state 0x%lx\n", dev->state);
} 

static void hijacked_netif_carrier_off(struct net_device *dev) {
    printk(KERN_ALERT"[ANIMA]: netif_carrier_off hijacked\n");
    printk(KERN_ALERT"[ANIMA]: dev name \"%s\"\n", dev->name);
    printk(KERN_ALERT"[ANIMA]: dev state 0x%lx\n", dev->state);
}

inline void read_original_bytes(unsigned char *func_ptr, unsigned char *buff) {
    int i;
    for (i = 0; i < PATCH_BYTES_COUNT; i++) {
        buff[i] = *(func_ptr + i);
    }
}

inline void write_original_bytes(unsigned char *func_ptr, unsigned char *buff) {
    int i;
    for (i = 0; i < PATCH_BYTES_COUNT; i++) {
        *(func_ptr + i) = buff[i];
    }
}

inline void write_patch(unsigned char *addr) {
    int i = 0; 
    for (i = 0; i < PATCH_BYTES_COUNT; i++) {
        *(addr + i) = hijacking_bytes[i];
    }
}

inline void create_patch(unsigned long func_ptr, unsigned char *out_buf) {
    int i = 0;
    // Write MOV instruction
    out_buf[0] = 0x48;
    out_buf[1] = 0xC7;
    out_buf[2] = 0xC0; 

    // Write address
    for (i = 3; i < 7; i++) {
#ifdef ANIMA_DEBUG
        printk(KERN_ALERT"Next ptr char code %x\n", (unsigned char)((func_ptr & (0xFF << (i-3)*8)) >> (i-3)*8));
#endif
        out_buf[i] = (unsigned char)((func_ptr & (0xFF << (i-3)*8)) >> (i-3)*8);
    } 

    // Write JMP instruction
    out_buf[7] = 0xFF;
    out_buf[8] = 0xE0; 
}

static int __init anima_init(void) {
#ifdef ANIMA_DEBUG
    int i = 0;
    // Create patch buffers
    create_patch((unsigned long)(hijacked_netif_carrier_on), hijacking_bytes);

    printk(KERN_ALERT"Original bytes:\n");
    for (i = 0; i < PATCH_BYTES_COUNT; i++) {
        printk(KERN_ALERT"%x\t", *((unsigned char *)(NETIF_CARRIER_ON_ADDR + i)));
    }

    printk(KERN_ALERT"My function address is %pK\n", hijacked_netif_carrier_on);
    printk(KERN_ALERT"Patch bytes:\n");
    for (i = 0; i < PATCH_BYTES_COUNT; i++) {
        printk(KERN_ALERT"%x\t", hijacking_bytes[i]);
    }
#endif

    // Save original bytes
    read_original_bytes(NETIF_CARRIER_ON_ADDR, orig_netif_on_bytes);
    read_original_bytes(NETIF_CARRIER_OFF_ADDR, orig_netif_off_bytes); 

    write_cr0 (read_cr0 () & (~ 0x10000));
    create_patch((unsigned long)(hijacked_netif_carrier_on), hijacking_bytes);
    write_patch(NETIF_CARRIER_ON_ADDR); 

    create_patch((unsigned long)(hijacked_netif_carrier_off), hijacking_bytes);
    write_patch(NETIF_CARRIER_OFF_ADDR);
    write_cr0 (read_cr0 () | 0x10000);

    printk(KERN_ALERT"[ANIMA]: in kernel, function hijacked.\n");
    return 0;
}

static void __exit anima_exit(void) {
    write_cr0 (read_cr0 () & (~ 0x10000));
    write_original_bytes(NETIF_CARRIER_ON_ADDR, orig_netif_on_bytes); 
    write_original_bytes(NETIF_CARRIER_OFF_ADDR, orig_netif_on_bytes); 
    write_cr0 (read_cr0 () | 0x10000);

    printk(KERN_ALERT"[ANIMA]: leaving the kernel...\n");
}

module_init(anima_init);
module_exit(anima_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lyblinskii A.");
MODULE_DESCRIPTION("netif_carrier_on hijacking");
