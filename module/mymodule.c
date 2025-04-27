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




module_init(simple_init);
module_exit(simple_exit);
