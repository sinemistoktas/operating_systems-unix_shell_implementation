#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/fdtable.h>

// Meta Information
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sinemis & Yamaç");
MODULE_DESCRIPTION("COMP 304 SPRING 2025 PROJECT 1: lsfd Kernel Module");

#define BUFFER_SIZE 8192 // max size of output buffer

static struct proc_dir_entry *proc_entry; // pointer to /proc/lsfd
static char *output_buffer;
static size_t output_size; // current size of output buffer
static pid_t target_pid = -1; // default PID -1

// Forward declarations
static int simple_init(void);
static void simple_exit(void);
static ssize_t lsfd_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos);
static ssize_t lsfd_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos);




// A function that runs when the module is first loaded -> init
int simple_init(void) {
	struct task_struct *ts;

	ts = get_pid_task(find_get_pid(4), PIDTYPE_PID);

	printk("Hello from the kernel");
	printk("command: %s\n", ts->comm);


	output_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
    if (!output_buffer)
        return -ENOMEM;

    proc_entry = proc_create("lsfd", 0666, NULL, &fops);
    if (!proc_entry) {
        kfree(output_buffer);
        return -ENOMEM;
    }

    printk(KERN_INFO "lsfd module loaded successfully.\n");
	return 0;
}


// A function that runs when the module is removed
void simple_exit(void) {
	proc_remove(proc_entry);
    kfree(output_buffer);
	printk(KERN_INFO "Goodbye from the kernel\n");
}


module_init(simple_init);
module_exit(simple_exit);
