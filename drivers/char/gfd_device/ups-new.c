#include <linux/module.h>   /*module_init()*/
#include <linux/kernel.h>	/* printk() */
#include <linux/init.h>		/* __init __exit */
#include <linux/fs.h>		/* file_operation */
#include <asm/uaccess.h>	/* copy_to_user, copy_from_user */
#include <linux/miscdevice.h>/*miscdevice*/
#include <linux/errno.h>	/* Error number */
#include <linux/delay.h>	/* Error number */
#include <linux/slab.h>	/* kmalloc*/

#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>

#include <linux/sched.h>   //wake_up_process()
#include <linux/kthread.h> //kthread_create()、kthread_run()
#include <linux/err.h>           //IS_ERR()、PTR_ERR()
#include <linux/interrupt.h>

#include <linux/reboot.h>	//pm_power_off();
#include <linux/syscalls.h>	

/*
	KERN_EMERG		用于紧急事件,一般是系统崩溃前的提示信息
	KERN_ALERT		用于需要立即采取动作的场合
	KERN_CRIT		临界状态,通常设计验证的硬件或软件操作失败
	KERN_ERR		用于报告错误状态.设备驱动程序通常会用它报告来自硬件的问题
	KERN_WARNING	就可能出现的问题提出警告.这些问题通常不会对系统造成严重破坏
	KERN_NOTICE		有必要提示的正常情况.许多安全相关的情况用这个级别汇报
	KERN_INFO		提示性信息.有很多驱动程序在启动时用这个级别打印相关信息
	KERN_DEBUG		用于调试的信息
*/

typedef struct _UPS_CONTROL {
	struct task_struct *shutdown_task;
	wait_queue_head_t shutdown_wait;
	int ups_irqnum;
	int condition;
	unsigned char ups_flag;
	unsigned char User_Message;
} UPS_CONTROL;

UPS_CONTROL *g_UPS_Control;

static irqreturn_t ups_interrupt (int irq, void *dev_id)
{
	UPS_CONTROL *pControl = (UPS_CONTROL *)dev_id;
	
	pControl->ups_flag = true;
	pControl->condition = true;
	//wake_up_process(shutdown_task);
	wake_up_interruptible(&pControl->shutdown_wait);//唤醒进程
	disable_irq_nosync(pControl->ups_irqnum);//关闭中断立即返回
	
	return IRQ_HANDLED;
}


static int shutdown_thread(void *data)
{
	UPS_CONTROL *pControl = (UPS_CONTROL *)data;
	struct timeval begin, end;
	
	printk(KERN_CRIT "%s() free_irq\n",__FUNCTION__);
	
	wait_event_interruptible(pControl->shutdown_wait,pControl->condition);
	//free_irq(ups_irqnum, NULL);//会失败，并一直等待

	do_gettimeofday(&begin);
	for(;;)//for循环实现线程任务
	{								
		//检测超时
		do_gettimeofday(&end);
		if (abs(end.tv_sec - begin.tv_sec) > 2)	//超时 
		{
			printk(KERN_CRIT "Syncing filesystems ... ");
			sys_sync();//将内存数据同步到磁盘
			msleep(200);
			printk(KERN_CRIT "done.\n");
			
			if (orderly_poweroff(true) < 0)
			{
				printk(KERN_CRIT "envctrl: shutdown execution failed\n");
		
				kernel_power_off();	
			}
		}				
		else if (pControl->User_Message)//用户已经在关机
		{
			printk(KERN_CRIT "gfd-ups-modules:wait user shutdown\n");
			__set_current_state(TASK_INTERRUPTIBLE);//将进程从 CPU 运行队列中移除
		}
		else if (unlikely(kthread_should_stop()))//检测停止信号
		{
			pControl = NULL;
			break;//结束线程
		}
		
		schedule();//进程间切换	
	}

	printk(KERN_CRIT "key_thread_func stop\n");
	
	return 0;
}

static void shutdown_kthread_stop(UPS_CONTROL *pControl)
{
	if (pControl->shutdown_task)
	{
		printk("in shutdown_kthread_stop\n");
		pControl->condition = true;
		wake_up_interruptible(&pControl->shutdown_wait);//唤醒进程
		kthread_stop(pControl->shutdown_task);
		pControl->shutdown_task = NULL;
	}
	return ;
}

static int Device_t_open(struct inode *inode, struct file *file)
{
	printk(KERN_CRIT "UPS:%s()\n",__FUNCTION__);
	
	file->private_data = g_UPS_Control;
	
	return 0;
}

static int Device_t_release(struct inode *inode, struct file *file)
{
	printk(KERN_CRIT "UPS:%s()\n",__FUNCTION__);
	
	file->private_data = NULL;
	
	return 0;
}
static int Device_t_mmap(struct file *file, struct vm_area_struct *vma)
{
	printk(KERN_CRIT "UPS:%s()\n",__FUNCTION__);
	
	return 0;
}

static long Device_t_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	printk(KERN_CRIT "UPS:%s()\n",__FUNCTION__);

	return 0;
}
static ssize_t Device_t_write(struct file *file,const char __user *buf, size_t count, loff_t *offset)
{
	struct _UPS_CONTROL *pControl = file->private_data;
	
	printk(KERN_EMERG "UPS:%s()\n",__FUNCTION__);

	if(copy_from_user(&pControl->User_Message, buf, count))//如果成功返回0；如果失败，会在已拷贝的数据后面用0填充，直到指定的数量n
	{
			printk(KERN_CRIT "copy_from_user error = %d\n",EFAULT);
			return -EFAULT;//14	/* Bad address */
	}
	
	return count;
}


static ssize_t Device_t_read(struct file *file,char *buf,size_t count,loff_t *offp ) 
{
	//printk(KERN_EMERG "UPS:%s()\n",__FUNCTION__);
	struct _UPS_CONTROL *pControl = file->private_data;
	
	if(copy_to_user(buf,&pControl->ups_flag, count))//如果成功返回0；如果失败，会在已拷贝的数据后面用0填充，直到指定的数量n
	{
			printk(KERN_CRIT "copy_to_user error = %d\n",EFAULT);
			return -EFAULT;//14	/* Bad address */
	}
	
	return count;
}

static const struct file_operations Device_t_fops = {
	.owner = THIS_MODULE,
	.open 	= Device_t_open,
	.release	= Device_t_release,
	.unlocked_ioctl	= Device_t_ioctl,
	.mmap = Device_t_mmap,
	.write = Device_t_write,
	.read = Device_t_read,	
};

static struct miscdevice Device_t_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,//255
	.name = "ups_detect",
	.fops = &Device_t_fops,
};

static int Get_Irqnum(struct device_node *ups_node)
{
	int ups_gpionum;
	int irq_num;
	enum of_gpio_flags irq_flag;
	
	printk(KERN_EMERG "UPS:%s()\n",__func__);
	//gpio对应的编号算法如：GPIO5_B3   对应  5*32 + (B-A) * 8 + 3
	//ups_gpionum = of_get_named_gpio_flags(ups_node, "gpio8-a6", 0, &irq_flag);
	ups_gpionum = of_get_named_gpio_flags(ups_node, "gpio7-a3", 0, &irq_flag);
	printk(KERN_EMERG "get gpio id successful led = %d\n",ups_gpionum);
	
	if(!gpio_is_valid(ups_gpionum)){
	  printk(KERN_EMERG "invalid led-gpios: %d\n",ups_gpionum);
	  return -1;
	}

	irq_num = gpio_to_irq(ups_gpionum); 
	printk(KERN_EMERG "ups_irqnum: %d\n",irq_num);
	
	return irq_num;
}

static int Device_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct _UPS_CONTROL *pControl;
	
	printk(KERN_EMERG "UPS:%s()\n",__func__);
	
	pControl = kmalloc(sizeof(struct _UPS_CONTROL), GFP_KERNEL);
	if (!pControl)
	{
		printk(KERN_CRIT "%s():内存申请失败\n",__FUNCTION__);
		return -ENOMEM;
	}
	
	init_waitqueue_head(&pControl->shutdown_wait);
	
	pControl->ups_irqnum = Get_Irqnum(pdev->dev.of_node);
	if (pControl->ups_irqnum < 0)
	{
			kfree(pControl);
			return -EINVAL;//22	/* Invalid argument */
	}
	
	ret = request_irq(pControl->ups_irqnum, &ups_interrupt, IRQF_DISABLED, "ups-interrupts", (void *)pControl);
	if (ret != 0)
	{
		printk(KERN_EMERG "Register ups-interrupts failed! ret = %d\n",ret);//
		kfree(pControl);
		return ret;
	}
	
	pControl->shutdown_task = kthread_create(shutdown_thread, (void *)pControl, "shutdown");
	if (IS_ERR(pControl->shutdown_task)) {
		printk(KERN_EMERG "Unable to start kernel thread. error = %ld\n", PTR_ERR(pControl->shutdown_task));
		
		free_irq(pControl->ups_irqnum, (void *)pControl);
		pControl->shutdown_task = NULL;
		kfree(pControl);
		return PTR_ERR(pControl->shutdown_task);
	}
	wake_up_process(pControl->shutdown_task);
	
	g_UPS_Control = pControl;
	platform_set_drvdata(pdev, pControl);
	
	return 0;
}

static int Device_remove(struct platform_device *pdev)
{
	//printk(KERN_EMERG "UPS:%s()\n",__func__);
	
	struct _UPS_CONTROL *pControl = platform_get_drvdata(pdev);
	if (pControl)
	{
		platform_set_drvdata(pdev, NULL);
		g_UPS_Control = NULL;
		
		printk(KERN_CRIT "free irq ups_irqnum\n");
		free_irq(pControl->ups_irqnum, (void *)pControl);

		shutdown_kthread_stop(pControl);
		kfree(pControl);
	}
	//platform_device_add
	return 0;
}

static const struct of_device_id Device_of_match[] = {
	{ .compatible = "ups,interrupt"},
	{},
};

static struct platform_driver Device_driver = {
	.probe		= Device_probe,
	.remove		= Device_remove,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "ups_driver",
		.of_match_table	= of_match_ptr(Device_of_match),
	},
};

static int __init Device_t_init(void)
{
	int err = 0;
	err = platform_driver_register(&Device_driver);
	if (err)
		return err;

	return misc_register(&Device_t_miscdev);
}

static void __exit Device_t_exit(void)
{
	printk(KERN_CRIT "UPS:%s()\n",__func__);

	platform_driver_unregister(&Device_driver);
	
	misc_deregister(&Device_t_miscdev);
}

subsys_initcall(Device_t_init);
module_exit(Device_t_exit);

MODULE_DESCRIPTION("Driver for GFD I2C Bus");
MODULE_AUTHOR("kfx, kfx@rock-chips.com");
MODULE_LICENSE("GPL");

