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


// proc_ops struct
static struct proc_ops fops = {
    .proc_read = lsfd_read,
    .proc_write = lsfd_write,
};

// Helper function: Build FD info for a given PID
static void build_fd_info(pid_t pid)
{
    struct task_struct *task;
    struct files_struct *files;
    struct fdtable *fdt;
    struct file *f;
    int i;

	output_size = 0;
    memset(output_buffer, 0, BUFFER_SIZE);

    // Find the task_struct using PID
    task = pid_task(find_get_pid(pid), PIDTYPE_PID);
    if (!task) {
        printk(KERN_INFO "lsfd: No such process with PID %d\n", pid);
        return;
    }

	printk(KERN_INFO "lsfd: Found process %s (PID %d)\n", task->comm, pid);

    rcu_read_lock();
    files = task->files;
    if (!files) {
        rcu_read_unlock();
        printk(KERN_INFO "lsfd: Process has no open files\n");
        return;
    }

    spin_lock(&files->file_lock);
    fdt = files_fdtable(files);

    for (i = 0; i < fdt->max_fds; i++) {
        f = fdt->fd[i];
        if (f) {
            struct dentry *dentry = f->f_path.dentry;
            struct inode *inode = dentry->d_inode;
            char *tmp;
            char *pathname;

            pathname = kmalloc(256, GFP_KERNEL);
            if (!pathname)
                continue;

            tmp = d_path(&f->f_path, pathname, 256);
            if (IS_ERR(tmp)) {
                kfree(pathname);
                continue;
            }

            output_size += scnprintf(output_buffer + output_size, BUFFER_SIZE - output_size,
                                     "fd %d %s %lld bytes %s\n",
                                     i,
                                     dentry->d_name.name,
                                     inode->i_size,
                                     tmp);

            kfree(pathname);
        }
    }

    spin_unlock(&files->file_lock);
    rcu_read_unlock();
}

// Read operation: User reads from /proc/lsfd
static ssize_t lsfd_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    if (*ppos > 0 || output_size == 0)
        return 0;

    if (copy_to_user(ubuf, output_buffer, output_size))
        return -EFAULT;

    *ppos = output_size;
    return output_size;
}

// Write operation: User writes PID into /proc/lsfd
static ssize_t lsfd_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
    char kbuf[16];

    if (count >= sizeof(kbuf))
        return -EINVAL;

    if (copy_from_user(kbuf, ubuf, count))
        return -EFAULT;

    kbuf[count] = '\0';

    if (kstrtoint(kbuf, 10, &target_pid) == 0) {
        printk(KERN_INFO "lsfd: Received PID %d\n", target_pid);
        build_fd_info(target_pid);
    } else {
        printk(KERN_INFO "lsfd: Invalid PID input\n");
    }

    return count;
}


// A function that runs when the module is first loaded -> init
int simple_init(void) {
	struct task_struct *ts;

	ts = get_pid_task(find_get_pid(4), PIDTYPE_PID);

	printk("lsfd: Hello from the kernel");
	printk("command: %s\n", ts->comm);


	output_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
    if (!output_buffer)
        return -ENOMEM;

    proc_entry = proc_create("lsfd", 0666, NULL, &fops);
    if (!proc_entry) {
        kfree(output_buffer);
        return -ENOMEM;
    }

    printk(KERN_INFO "lsfd: Module loaded successfully.\n");
	return 0;
}


// A function that runs when the module is removed
void simple_exit(void) {
	proc_remove(proc_entry);
    kfree(output_buffer);
	printk(KERN_INFO "lsfd: Goodbye from the kernel\n");
}


module_init(simple_init);
module_exit(simple_exit);
