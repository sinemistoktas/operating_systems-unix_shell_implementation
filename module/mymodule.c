#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/slab.h>

// Meta Information
MODULE_LICENSE("GPL");
MODULE_AUTHOR( /* TODO: type here your own name as a string */ );
MODULE_DESCRIPTION( /* TODO: type here your kernel module description as a string */ );


// A function that runs when the module is first loaded
int simple_init(void) {
	struct task_struct *ts;

	ts = get_pid_task(find_get_pid(4), PIDTYPE_PID);

	printk("Hello from the kernel);
	printk("command: %s\n", ts->comm);
	return 0;
}


// A function that runs when the module is removed
void simple_exit(void) {
	printk("Goodbye from the kernel);
}


module_init(simple_init);
module_exit(simple_exit);
