/*
 *  linux/drivers/devfreq/governor_pandoon.c
 *
 *  Copyright (C) 2011 Samsung Electronics
 *	MyungJoo Ham <myungjoo.ham@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/devfreq.h>
#include <linux/math64.h>
#include <linux/delay.h>
#include <linux/cpufreq_pandoon.h>
#include <linux/proc_fs.h>
#include<linux/slab.h>                 //kmalloc()
#include<linux/uaccess.h>              //copy_to/from_user()
#include "governor.h"
#include <linux/sched.h>
#include <linux/ioctl.h>
#include <linux/kthread.h>

#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/kernel.h>


#include <linux/random.h>
wait_queue_head_t wq;
int flag = 0;

/* Default constants for DevFreq-pandoon (DFP) */
#define DFSO_UPTHRESHOLD	(90)
#define DFSO_DOWNDIFFERENCTIAL	(5)



#define mem_size        32

dev_t dev_pandoon=0;
static struct class *dev_class_pandoon;
static struct cdev pandoon_cdev;
uint8_t *kernel_buffer;


static struct task_struct *wait_thread;

struct frequencies{
        uint64_t gf;
        uint32_t f1;
        uint32_t f2;
	uint64_t capturing;
};
//bool cap=false;
union ptr{
	int8_t* a;
	uint64_t padding;
};

#define Pandoon_MAGIC (0xAA)
#define next_state _IO(Pandoon_MAGIC,'c')
//#define capture_freqs _IOR(Pandoon_MAGIC,'c',struct frequencies *)
#define capture_freqs _IOR(Pandoon_MAGIC,'n',struct frequencies )
//#define next_freq _IOW(Pandoon_MAGIC,'f', int*)
#define next_freq _IOW(Pandoon_MAGIC,'f', int)
//new
//#define Apply_freqs _IOW(Pandoon_MAGIC,'a', int8_t**)
#define Apply_freqs _IOW(Pandoon_MAGIC,'a', union ptr)


struct frequencies freqs;
char value = '0';
//char etx_array[20]="try_proc_array\n";
static int len = 1;
int f_index=0;
//new
int8_t dfreqs[3];
bool upd=false;
bool prnt=false;

static ssize_t read_proc(struct file *filp, char __user *buffer, size_t length,loff_t * offset);
static ssize_t write_proc(struct file *filp, const char *buff, size_t len, loff_t * off);
static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg);




static int pandoon_open(struct inode *inode, struct file *file)

{

       if((kernel_buffer = kmalloc(mem_size , GFP_KERNEL)) == 0){
            printk(KERN_INFO "Cannot allocate memory in kernel\n");
            return -1;
        }
        printk(KERN_INFO "Device File Opened...!!!\n");
        return 0;
}

 

static int pandoon_close(struct inode *i, struct file *f)

{

        kfree(kernel_buffer);
        printk(KERN_INFO "Device File Closed...!!!\n");
        return 0;

}



static struct file_operations dev_fops = {
        .open = pandoon_open,
	.owner=THIS_MODULE,
        .read = read_proc,
        .write = write_proc,
#ifdef CONFIG_COMPAT
	.compat_ioctl = my_ioctl,
#endif
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
	.ioctl = my_ioctl,
#else
	.unlocked_ioctl = my_ioctl,
#endif
        .release = pandoon_close
};




static ssize_t read_proc(struct file *filp, char __user *buffer, size_t length,loff_t * offset)
{
    printk(KERN_INFO "proc file read.....\n");
    if(len)
        len=0;
    else{
        len=1;
        return 0;
    }
    if(copy_to_user( buffer, &value, sizeof(value))){
	printk("read error\n");
	return -1;
    }
 
    return length;;
}
 
static ssize_t write_proc(struct file *filp, const char *buff, size_t len, loff_t * off)
{
    printk(KERN_INFO "proc file wrote.....\n");
    if(copy_from_user(&value,buff,sizeof(value))){
	printk("write error\n");
	return -1;
    }
    printk(KERN_INFO "value=%c\n",value);
    if (value=='0'){
	prnt=false;
	upd=false;
	freqs.capturing=0;
	printk("capture off, update manner off");
    }
    if (value=='1'){
	upd=false;
	freqs.capturing=1;
	printk("capture on, update manner off");
    }
    if (value=='2'){
	upd=true;
	printk("update manner on");
    }
    if (value=='3'){
	upd=true;
	freqs.capturing=1;
	printk("capture on, update manner on");
    }
    if (value=='4'){
	prnt=true;
    }


    return len;
}



#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
static int my_ioctl(struct inode *i, struct file *file, unsigned int cmd, unsigned long arg)
#else
static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
#endif
{

	long r=1000;
	//printk("my_ioctl called");
	void __user *u_arg;
	//printk("ioctl is called\n");
	
#ifdef CONFIG_COMPAT
	if (is_compat_task())
		u_arg = compat_ptr(arg);
	else
		u_arg = (void __user *)arg;
#else
	u_arg = (void __user *)arg;
#endif
	switch(cmd){
		case next_state:
			//printk("next state called!");
			flag = 1 ;
			wake_up_interruptible (&wq);
			r=0;
			break;
		
		case next_freq:
			//printk("next_freq called");			
			flag=2;
			if(copy_from_user(&f_index, (int*)u_arg, sizeof(int))){
				printk("copy from user error!\n");
				return -1;
				
			}
			wake_up_interruptible (&wq);
			r=1;
			break;
		case capture_freqs:
			//printk("capture freqs has been called,capturing:%llu,%llu,%u,%u",freqs.capturing,freqs.gf,freqs.f1,freqs.f2);
			if(copy_to_user((struct frequencies*) u_arg, &freqs, sizeof(freqs))){
				printk("copy to user error\n");
				return -2;
			}
			r=2;
			break;

		//new
		case Apply_freqs:
			//printk("Apply freqs called0,%d,%d,%d",dfreqs[0],dfreqs[1],dfreqs[2]);
			flag=3;
			if(copy_from_user( &dfreqs , (int8_t**)u_arg , 3*sizeof(int8_t) )){
				printk("apply freqs, copy from user error\n");
				return -3;
			}
			//printk("Apply freqs called1,%d,%d,%d",dfreqs[0],dfreqs[1],dfreqs[2]);
			wake_up_interruptible (&wq);
			r=3;
			break;


	}
	return r;


}

static bool first=1;

int func(void* input){
	int ret = -EINVAL;
	struct devfreq *df=(struct devfreq*) input;
	u32 flags=0;
	//int i=1;
	int index=0;
	int index1=0;
	int index2=0;
	long unsigned int f1=237143000;
	//long unsigned int f2=767000000;
	unsigned int freq_req1,freq_req2;	
	long unsigned int cur_f;
	struct cpufreq_frequency_table *freq_table1;
	struct cpufreq_frequency_table *freq_table2; 
	printk("0_gpu governor:1:pset1:%d,pset2:%d",pset1,pset2);
	if(pset1==0){
		//freq= 237143000;
		printk("1_gpu governor:2:pset1:%d,pset2:%d",pset1,pset2);
		first=1;
		return 0;
	}
	printk("2_gpu governor:3:pset1:%d,pset2:%d",pset1,pset2);
	/*
	if(pset2==0){
		printk("gpu governor:4:pset1:%d,pset2:%d");
		freq_table1=globpolicy1->freq_table;
		while(true){
			printk("gpu governor:5:pset1:%d,pset2:%d");
			f1=df->profile->freq_table[index++];
			freq_req1 = freq_table1[index1++].frequency;
			df->profile->target(df->dev.parent, &f1, flags);
			__cpufreq_driver_target(globpolicy1, freq_req1, CPUFREQ_RELATION_H);
			index=index%8;
			index1=index1%7;
			//udelay(16000);
			wait_event_interruptible(wq, flag != 0);
			flag=0;
		}
 
	}*/
	if(pset1==1 && pset2==1){
		printk("3_gpu governor:2:pset1:%d,pset2:%d",pset1,pset2);
		freq_table1=globpolicy1->freq_table;
		freq_table2=globpolicy2->freq_table;
		while(true){
			//printk("gpu governor:3:pset1:%d,pset2:%d",pset1,pset2);
			f1=df->profile->freq_table[index++];
			freq_req1 = freq_table1[index1].frequency;
			freq_req2 = freq_table2[index2].frequency;
			if(!upd)
				df->profile->target(df->dev.parent, &f1, flags);
			
			else{
				mutex_lock(&df->lock);
				update_devfreq2(df,f1);
				mutex_unlock(&df->lock);
			}

	

			down_write(&globpolicy1->rwsem);

			ret = __cpufreq_driver_target(globpolicy1, freq_req1, CPUFREQ_RELATION_H);

			up_write(&globpolicy1->rwsem);

			//printk("ret1:%d",ret);


			down_write(&globpolicy2->rwsem);

			ret = __cpufreq_driver_target(globpolicy2, freq_req2, CPUFREQ_RELATION_H);

			up_write(&globpolicy2->rwsem);

			//printk("ret1:%d",ret);

			///cpufreq_driver_target(globpolicy1, freq_req1, CPUFREQ_RELATION_H,0);
			///cpufreq_driver_target(globpolicy2, freq_req2, CPUFREQ_RELATION_H,0);
			//__cpufreq_driver_target(globpolicy1, freq_req1, CPUFREQ_RELATION_H,0);
			//__cpufreq_driver_target(globpolicy2, freq_req2, CPUFREQ_RELATION_H,0);
			//__cpufreq_driver_target(globpolicy1, freq_req1, CPUFREQ_RELATION_L);
			//__cpufreq_driver_target(globpolicy2, freq_req2, CPUFREQ_RELATION_L);

			

			//real freqs settings:
			
			if (df->profile->get_cur_freq){
				df->profile->get_cur_freq(df->dev.parent, &cur_f);
				freqs.gf=cur_f;
			}
			else
				freqs.gf = df->previous_freq;
			

			freqs.f1=globpolicy1->cur;
			freqs.f2=globpolicy2->cur;


			/*wanted freqs settings:
			freqs.gf=f1;
			freqs.f1=freq_req1;
		        freqs.f2=freq_req2;
			//freqs.capturing=cap;*/
			
			get_random_bytes(&index, sizeof(int)-1);
			get_random_bytes(&index1, sizeof(int)-1);
			get_random_bytes(&index2, sizeof(int)-1);
			index=index%8;
			index1=index1%9;
			index2=index2%7;
			//udelay(16000);

			/* check if wanted cpu is not applyed:
			if(freq_req1!=globpolicy1->cur || freq_req2 != globpolicy2->cur)
				printk("Difff");*/

			wait_event_interruptible(wq, flag != 0);
			if(prnt){
				unsigned long cur_freq=0;
				if (df->profile->get_cur_freq)
					df->profile->get_cur_freq(df->dev.parent, &cur_freq);
				else
					cur_freq = df->previous_freq;
				printk("want gpu freq:%lu,really:%lu,cpu53:%u,cpu73:%u",f1,cur_freq,freq_req2,freq_req1);

			}
			if(flag==2){
				index1=f_index;
				index1=index1%9;
			}
			//new
			if(flag==3){
				index=dfreqs[0];
				index1=dfreqs[1];
				index2=dfreqs[2];
			}
			flag=0;
		}
	}

	first=1;
	return 0;
}
static int devfreq_pandoon_func(struct devfreq *df,
					unsigned long *freq)
{
	//int lev,newlev;
	//unsigned long cur_freq;
	if (first){
		first=0;
		//pthread_t pt;
		wait_thread=kthread_create(func,(void*)df,"wthread");
		if (wait_thread) {
	                printk("Thread Created successfully\n");
        	        wake_up_process(wait_thread);
        	} else
                	printk(KERN_INFO "Thread creation failed\n");
		return 0;
	}else
		return 0;


//newehsan
/*
	if (!df->max_freq)
		*freq = UINT_MAX;
	else
		*freq = df->max_freq;
*/	



	
	////__cpufreq_driver_target(policy, policy->max, CPUFREQ_RELATION_H);
	
	

	//return 0;
	//struct devfreq_freqs freqs;
	/*if (df->profile->get_cur_freq)
		df->profile->get_cur_freq(df->dev.parent, &cur_freq);
	else
		cur_freq = df->previous_freq;*/

	//lev = devfreq_get_freq_level(df, cur_freq);
	/*f1=cur_freq+150000000;
	if (f1 > 767000000)
		f1=103750000;

	*freq=f1;
	return 0;*/

	

	/*while(i<10000){
		i++;
		f1=df->profile->freq_table[index++];
		df->profile->target(df->dev.parent, &f1, flags);
		index=index%8;
		udelay(16000);
	}
	return 0;*/
	
/*
	if (cur_freq < 415000000){
		*freq=667000000;
		return 0;
	}
	else{
		*freq = 332000000;
		return 0;
	}*/

/*
	if (cur_freq == 0){
		*freq=667000000;
		return 0;
	}

	switch (cur_freq){

	case 103750000:
		*freq=237143000;
        case 237143000:
		*freq=332000000;
	case 332000000:
		*freq=415000000;
	case 415000000:
		*freq=550000000;
	case 550000000:
		*freq=667000000;
	case 667000000:
		*freq=767000000;

	default:
		*freq=415000000;

	}
	return 0;
*/	
/*
	for (lev = 0; lev < df->profile->max_state; lev++)
			if (cur_freq == df->profile->freq_table[lev])
				break;
	newlev=lev+1;
	if (newlev > df->profile->max_state)
		newlev= newlev % (df->profile->max_state);
	*freq=df->profile->freq_table[newlev];
	return 0;*/
	
}


//newehsan:comment handler
/*
static int devfreq_pandoon_handler(struct devfreq *devfreq,
				unsigned int event, void *data)
{
	switch (event) {
	case DEVFREQ_GOV_START:
		devfreq_monitor_start(devfreq);
		break;

	case DEVFREQ_GOV_STOP:
		devfreq_monitor_stop(devfreq);
		break;

	case DEVFREQ_GOV_INTERVAL:
		devfreq_interval_update(devfreq, (unsigned int *)data);
		break;

	case DEVFREQ_GOV_SUSPEND:
		devfreq_monitor_suspend(devfreq);
		break;

	case DEVFREQ_GOV_RESUME:
		devfreq_monitor_resume(devfreq);
		break;

	default:
		break;
	}

	return 0;
}
*/

//new ehsan; and add this handler:
static int devfreq_pandoon_handler(struct devfreq *devfreq,
				unsigned int event, void *data)
{
	int ret = 0;

	if (event == DEVFREQ_GOV_START) {
		printk("handler called\n");
		mutex_lock(&devfreq->lock);
		ret = update_devfreq(devfreq);
		mutex_unlock(&devfreq->lock);
	}

	return ret;
}
static struct devfreq_governor devfreq_pandoon = {
	.name = "pandoon",
	.get_target_freq = devfreq_pandoon_func,
	.event_handler = devfreq_pandoon_handler,
};



static int pandoon_uevent(struct device *dev, struct kobj_uevent_env *env)

{

    add_uevent_var(env, "DEVMODE=%#o", 0666);

    return 0;

}

static int __init devfreq_pandoon_init(void)
{

	//proc_create("pandoon",0666,NULL,&proc_fops);	
	if((alloc_chrdev_region(&dev_pandoon, 0, 1, "pandoon_dev")) <0){
                printk(KERN_INFO "Cannot allocate major number for device\n");
                return -1;
        }
        printk(KERN_INFO "Major = %d Minor = %d \n",MAJOR(dev_pandoon), MINOR(dev_pandoon));
 

       cdev_init(&pandoon_cdev,&dev_fops); 
        /*Adding character device to the system*/
        if((cdev_add(&pandoon_cdev,dev_pandoon,1)) < 0){
            printk(KERN_INFO "Cannot add the device to the system\n");
            goto r_class;
        }


        /*Creating struct class*/
        if((dev_class_pandoon = class_create(THIS_MODULE,"pandoon_class")) == NULL){
            printk(KERN_INFO "Cannot create the struct class for device\n");
            goto r_class;
        }

	dev_class_pandoon->dev_uevent=pandoon_uevent;
 
        /*Creating device*/
        if((device_create(dev_class_pandoon,NULL,dev_pandoon,NULL,"pandoon_device")) == NULL){
            printk(KERN_INFO "Cannot create the Device\n");
            goto r_device;
        }
        printk(KERN_INFO "Device Driver Insert...Done!!!\n");
 

	init_waitqueue_head(&wq);
	freqs.capturing=0;
	return devfreq_add_governor(&devfreq_pandoon);


r_device:
        class_destroy(dev_class_pandoon);
r_class:
        unregister_chrdev_region(dev_pandoon,1);
        return -1;

}
subsys_initcall(devfreq_pandoon_init);

static void __exit devfreq_pandoon_exit(void)
{
	int ret;
	first=1;
	ret = devfreq_remove_governor(&devfreq_pandoon);
	if (ret)
		pr_err("%s: failed remove governor %d\n", __func__, ret);
	//remove_proc_entry("pandoon",NULL);
	device_destroy(dev_class_pandoon,dev_pandoon);
        class_destroy(dev_class_pandoon);
	cdev_del(&pandoon_cdev);
        unregister_chrdev_region(dev_pandoon, 1);
	printk(KERN_INFO "Device Driver Remove...Done!!!\n");
	return;
}
module_exit(devfreq_pandoon_exit);
//subsys_exitcall(devfreq_pandoon_exit);
MODULE_LICENSE("GPL");
