#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc fs.h>
#include <asm/uaccess.h>
#include <linux/sched.h>


#define BUFFER SIZE 128
#define PROC NAME "hello"