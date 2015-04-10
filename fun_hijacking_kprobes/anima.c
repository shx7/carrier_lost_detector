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
#include <linux/types.h>
#include <linux/kprobes.h>
#include <linux/rtnetlink.h>
#include <linux/proc_fs.h>

#define DEV_NAME               "eth0"
#define MODE_PROC_FILE         "hijacking_mode"
#define LINK_CHANGE_PROC_FILE  "hij_chg_linkg_state"
#define NETIF_CARRIER_ON_ADDR  (unsigned char *)0xFFFFFFFF812A9093
#define NETIF_CARRIER_OFF_ADDR (unsigned char *)0xFFFFFFFF812A8DCB
#define MODULE_NAME            "link_chg_module"

enum ModuleState {
    HIJACKING_PRESENT,
    HIJACKING_OFF
};

static __u32 hijacking_state = HIJACKING_OFF;
module_param_named(hijacking_state, hijacking_state, uint, 0644);
MODULE_PARM_DESC(hijacking_state, "hijacking state");

struct net_device *p_dev = NULL;
char   device_copied = 0;
struct net_device dev;

void anima_netdev_state_change(struct net_device *dev) {
    call_netdevice_notifiers(NETDEV_CHANGE, dev);
    rtmsg_ifinfo(RTM_NEWLINK, dev, 0);
}

void change_link_state_up(struct net_device *dev) { 
    dev->state = 7;
    anima_netdev_state_change(dev); 
}

void change_link_state_down(struct net_device *dev) { 
    dev->state = 3;
    anima_netdev_state_change(dev); 
}

int pre_handler(struct kprobe *p, struct pt_regs *regs) {
    printk(KERN_ALERT"[ANIMA]: pre_handler:\n addr:%p rax:0x%lx\n", p->addr, regs->ax);
    p_dev = (struct net_device *)(regs->di);

    printk(KERN_ALERT"[ANIMA]: dev->name %s\n", p_dev->name);
    printk(KERN_ALERT"[ANIMA]: dev->state %lx\n", p_dev->state);

    if (strncmp(p_dev->name, DEV_NAME, IFNAMSIZ) == 0) {
        printk(KERN_ALERT"[ANIMA]: eth0 detected\n");
        dev = *p_dev; 
        device_copied = 1;

        if (hijacking_state == HIJACKING_PRESENT) {
            // Drop request
            p_dev->reg_state = NETREG_UNINITIALIZED;
            printk(KERN_ALERT"[ANIMA]: netif request dropped\n");
        } else { 
            printk(KERN_ALERT"[ANIMA]: netif request passed\n");
        }
    } else {
        printk(KERN_ALERT"[ANIMA]: not eth0\n");
    }
    return 0;    
}

static struct kprobe kp_off;
static struct kprobe kp_on = {
    .pre_handler = pre_handler,
    .post_handler = NULL,
    .fault_handler = NULL,
    .addr = NETIF_CARRIER_ON_ADDR,
};

static struct proc_dir_entry *proc_mode_file;
static struct proc_dir_entry *proc_link_chg_file;
static struct proc_dir_entry *proc_module_entry;
#define MSG_LEN 1024
static char msg[MSG_LEN] = {0};

static int proc_mode_read(char *page, char **start,
    off_t off, int count,
    int *eof, void *data)
{
    int len;

    if (hijacking_state == HIJACKING_PRESENT) {
        len = sprintf(page, "HIJACKING_PRESENT\n");
    } else {
        len = sprintf(page, "HIJACKING_OFF\n");
    } 

    return len;
}

static int proc_mode_write(struct file *file,
    const char *buffer,
    unsigned long count, 
    void *data)
{
    int len;

    if (count > MSG_LEN) {
        len = MSG_LEN;
    } else {
        len = count;
    }

    if (copy_from_user(msg, buffer, len))
        return -EFAULT;

    if (strncmp(msg, "1", 1) == 0) {
        printk(KERN_ALERT"[ANIMA]: hijacking mode changed to HIJACKING_PRESENT\n");
        hijacking_state = HIJACKING_PRESENT;
    } else {
        printk(KERN_ALERT"[ANIMA]: hijacking mode changed to HIJACKING_OFF\n");
        hijacking_state = HIJACKING_OFF;
    } 

    return len;
}

static int proc_link_chg_write (struct file *file,
    const char *buffer,
    unsigned long count, 
    void *data)
{
    int len;

    if (count > MSG_LEN) {
        len = MSG_LEN;
    } else {
        len = count;
    }

    if (copy_from_user(msg, buffer, len))
        return -EFAULT;

    if (device_copied == 0) {
        printk("[ANIMA]: device has not been copied. Can't do this\n");
        return len;
    }

    if (p_dev != NULL) { 
        if (strncmp(msg, "0", 1) == 0) {
            printk(KERN_ALERT"[ANIMA]: link state changed DOWN\n");
            change_link_state_down(&dev);
        } else if (strncmp(msg, "1", 1) == 0) {
            printk(KERN_ALERT"[ANIMA]: link state changed UP\n");
            change_link_state_up(&dev);
        } else {
            printk(KERN_ALERT"[ANIMA]: no link state change\n");
        }
    } else {
        printk(KERN_ALERT"[ANIMA]: dev struct is null, no change\n");
    }

    return len; 
}

static int __init anima_init(void) {
   printk(KERN_ALERT"[ANIMA]: in kernel\n"); 

   kp_off = kp_on;
   kp_off.addr = NETIF_CARRIER_OFF_ADDR;

   if ((register_kprobe(&kp_on)) < 0) {
       printk(KERN_ALERT"[ANIMA]: error while kprobe for netif_carrier_on register\n");
       return -1;
   } 

   if ((register_kprobe(&kp_off)) < 0) {
       printk(KERN_ALERT"[ANIMA]: error while kprobe for netif_carrier_off register\n");
       return -1;
   }

   // Register proc entry
   proc_module_entry = proc_mkdir(MODULE_NAME, NULL);
   if (proc_module_entry == NULL) { 
       remove_proc_entry(MODULE_NAME, NULL);
       printk(KERN_ALERT"[ANIMA]: can not create proc file:\"%s\"\n", MODULE_NAME);
       return -1;
   } 

   proc_mode_file = create_proc_entry(MODE_PROC_FILE, 0444, proc_module_entry);
   if (proc_mode_file == NULL) {
       remove_proc_entry(MODE_PROC_FILE, proc_module_entry);
       printk(KERN_ALERT"[ANIMA]: can not create proc file:\"%s\"\n", MODE_PROC_FILE);
       return -1;
   } else {
       proc_mode_file->read_proc = proc_mode_read;
       proc_mode_file->write_proc = proc_mode_write;
   }

   proc_link_chg_file = create_proc_entry(LINK_CHANGE_PROC_FILE, 0444, proc_module_entry);
   if (proc_mode_file == NULL) {
       remove_proc_entry(LINK_CHANGE_PROC_FILE, proc_module_entry);
       printk(KERN_ALERT"[ANIMA]: can not create proc file:\"%s\"\n", LINK_CHANGE_PROC_FILE);
       return -1;
   } else {
       proc_link_chg_file->write_proc = proc_link_chg_write;
   } 

   printk(KERN_ALERT"[ANIMA]: kprobe registered\n");
   return 0;
}

static void __exit anima_exit(void) {
   unregister_kprobe(&kp_on);
   unregister_kprobe(&kp_off);
   remove_proc_entry(MODE_PROC_FILE, proc_module_entry);
   remove_proc_entry(LINK_CHANGE_PROC_FILE, proc_module_entry);
   remove_proc_entry(MODULE_NAME, NULL);
   printk(KERN_ALERT"[ANIMA]: leaving the kernel...\n");
}

module_init(anima_init);
module_exit(anima_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lyblinskii A.");
MODULE_DESCRIPTION("netif_carrier_on hijacking");
