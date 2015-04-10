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

enum ModuleState {
    HIJACKING_PRESENT,
    HIJACKING_OFF
};

static __u32 hijacking_state = HIJACKING_OFF;
module_param_named(hijacking_state, hijacking_state, uint, 0644);
MODULE_PARM_DESC(hijacking_state, "hijacking state");

struct net_device *p_dev = NULL;

void change_link_state(struct net_device *dev) { 
    netdev_state_change(dev); 
}

int pre_handler(struct kprobe *p, struct pt_regs *regs) {
    printk(KERN_ALERT"[ANIMA]: pre_handler:\n addr:%p rax:0x%lx\n", p->addr, regs->ax);
    p_dev = (struct net_device *)(regs->di);

    printk(KERN_ALERT"[ANIMA]: dev->name %s\n", p_dev->name);
    printk(KERN_ALERT"[ANIMA]: dev->state %s\n", p_dev->name);

    if (strncmp(p_dev->name, DEV_NAME, IFNAMSIZ) == 0) {
        printk(KERN_ALERT"[ANIMA]: eth0 detected\n");
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
int tmp = 0;
static char msg[1024] = {0};

ssize_t procfile_write (struct file *f, const char __user *buffer, size_t len, loff_t *offset)
{
    len = copy_from_user(msg, buffer, len);
    if (msg[0] == '1') {
        printk(KERN_ALERT"[ANIMA]: hijacking on");
        hijacking_state = HIJACKING_PRESENT;
    } 

    if (msg[0] == '0') {
        printk(KERN_ALERT"[ANIMA]: hijacking off");
        hijacking_state = HIJACKING_OFF;
    } 
    return len;
}

ssize_t procfile_read (struct file *f, char __user *buffer, size_t len, loff_t *offset)
{
    if (hijacking_state == HIJACKING_PRESENT) {
        len = copy_to_user(buffer, "1", 1);
    } else {
        len = copy_to_user(buffer, "0", 1);
    }

    return len;
}

ssize_t proc_link_chg_write (struct file *f, const char __user *buffer, size_t len, loff_t *offset) {
    if (p_dev != NULL) {
        change_link_state(p_dev);
        printk("[ANIMA]: link state changed\n");
    } else {
        printk("[ANIMA]: dev struct is null, no change\n");
    }
    return len;
} 

static const struct file_operations proc_mode_file_ops = {
    .read = procfile_read,
    .write = procfile_write,
    .owner = THIS_MODULE, 
};

static const struct file_operations proc_link_chg_file_ops = {
    .write = proc_link_chg_write,
    .owner = THIS_MODULE, 
};

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
   proc_mode_file = proc_create(MODE_PROC_FILE, 0, NULL, &proc_mode_file_ops);
   if (proc_mode_file == NULL) {
       remove_proc_entry(MODE_PROC_FILE, NULL);
       printk(KERN_ALERT"[ANIMA]: can not create proc file:\"%s\"\n", MODE_PROC_FILE);
       return -1;
   }

   proc_link_chg_file = proc_create(LINK_CHANGE_PROC_FILE, 0, NULL, &proc_link_chg_file_ops);
   if (proc_mode_file == NULL) {
       remove_proc_entry(LINK_CHANGE_PROC_FILE, NULL);
       printk(KERN_ALERT"[ANIMA]: can not create proc file:\"%s\"\n", LINK_CHANGE_PROC_FILE);
       return -1;
   }


   printk(KERN_ALERT"[ANIMA]: kprobe registered\n");
   return 0;
}

static void __exit anima_exit(void) {
   unregister_kprobe(&kp_on);
   unregister_kprobe(&kp_off);
   remove_proc_entry(MODE_PROC_FILE, NULL);
   remove_proc_entry(LINK_CHANGE_PROC_FILE, NULL);
   printk(KERN_ALERT"[ANIMA]: leaving the kernel...\n");
}

module_init(anima_init);
module_exit(anima_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lyblinskii A.");
MODULE_DESCRIPTION("netif_carrier_on hijacking");
