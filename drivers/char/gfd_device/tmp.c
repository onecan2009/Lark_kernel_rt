

#include <linux/module.h>   /*module_init()*/
#include <linux/kernel.h>	/* printk() */
#include <linux/init.h>		/* __init __exit */
#include <linux/fs.h>		/* file_operation */
#include <asm/uaccess.h>	/* copy_to_user, copy_from_user */
#include <linux/miscdevice.h>/*miscdevice*/
#include <linux/errno.h>	/* Error number */


int Max(int t1, int t2)
{
	return t1 < t2 ? t2 : t1;
}

int Min(int t1, int t2)
{
	return t1 < t2 ? t1 : t2;
}
