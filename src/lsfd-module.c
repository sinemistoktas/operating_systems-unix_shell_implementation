// comp 304 spring 2025 project 1 part 4
// Kernel module to provide file descriptor info

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/sched.h>

#define BUFFER SIZE 128
#define PROC NAME "lsfd"

ssize_t proc_read(struct file *file, char __user *usr_buf, size_t count, loff_t *pos);

static struct file_operations proc_ops = {
    .owner = THIS_MODULE,
    .read = proc_read,
};

/* This function is called when the module is loaded. */
int proc_init(void){
    /* creates the /proc/hello entry */
    proc_create(PROC_NAME, 0666, NULL, &proc_ops);

    return 0;
}

/* This function is called when the module is removed. */
void proc exit(void){
    /* removes the /proc/hello entry */
    remove_proc_entry(PROC_NAME, NULL);
}

