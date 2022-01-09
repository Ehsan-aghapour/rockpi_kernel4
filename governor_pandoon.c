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

#include <linux/random.h>
wait_queue_head_t wq;
int flag = 0;

/* Default constants for DevFreq-pandoon (DFP) */
#define DFSO_UPTHRESHOLD	(90)
#define DFSO_DOWNDIFFERENCTIAL	(5)


static struct task_struct *wait_thread;

struct frequencies{
        long unsigned int gf;
        unsigned int f1;
        unsigned int f2;
	bool capturing;
};
//bool cap=false;
#define next_state _IO('p','n')
#define capture_freqs _IOR('p','c',struct frequencies *)
#define next_freq _IOW('p','f', int*)


struct frequencies freqs;
char value = '0';
//char etx_array[20]="try_proc_array\n";
static int len = 1;
int f_index=0;

static ssize_t read_proc(struct file *filp, char __user *buffer, size_t length,loff_t * offset);
static ssize_t write_proc(struct file *filp, const char *buff, size_t len, loff_t * off);
static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

static struct file_operations proc_fops = {
       // .open = open_proc,
        .read = read_proc,
        .write = write_proc,
        .unlocked_ioctl=my_ioctl,
       // .release = release_proc
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
    copy_to_user( buffer, &value, sizeof(value));
 
    return length;;
}
 
static ssize_t write_proc(struct file *filp, const char *buff, size_t len, loff_t * off)
{
    printk(KERN_INFO "proc file wrote.....\n");
    copy_from_user(&value,buff,sizeof(value));
    printk(KERN_INFO "value=%c\n",value);
    if (value=='0')
	freqs.capturing=false;
    if (value=='1')
	freqs.capturing=true;
    return len;
}


static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg){
	switch(cmd){
		case next_state:
			flag = 1 ;
			wake_up_interruptible (&wq);
			break;
		
		case next_freq:
			flag=2;
			copy_from_user(&f_index, (int*)arg, sizeof(int));
			wake_up_interruptible (&wq);
			break;
		case capture_freqs:
			copy_to_user((struct frequencies*) arg, &freqs, sizeof(freqs));
			break;

	}
	return 0;


}

int func(void* input){
	struct devfreq *df=(struct devfreq*) input;
	u32 flags=0;
	//int i=1;
	int index=0;
	int index1=0;
	int index2=0;
	long unsigned int f1=237143000;
	//long unsigned int f2=767000000;
	unsigned int freq_req1,freq_req2;	
	struct cpufreq_frequency_table *freq_table1;
	struct cpufreq_frequency_table *freq_table2; 

	if(pset1==0){
		//freq= 237143000;
		//return 0;
	}
	if(pset2==0){
		freq_table1=globpolicy1->freq_table;
		while(true){
			
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
 
	}
	else{
		freq_table1=globpolicy1->freq_table;
		freq_table2=globpolicy2->freq_table;
		while(true){
			
			f1=df->profile->freq_table[index++];
			freq_req1 = freq_table1[index1].frequency;
			freq_req2 = freq_table2[index2].frequency;
			df->profile->target(df->dev.parent, &f1, flags);
			__cpufreq_driver_target(globpolicy1, freq_req1, CPUFREQ_RELATION_H);
			__cpufreq_driver_target(globpolicy2, freq_req2, CPUFREQ_RELATION_H);
			freqs.gf=f1;
			freqs.f1=freq_req1;
		        freqs.f2=freq_req2;
			//freqs.capturing=cap;
			get_random_bytes(&index, sizeof(int)-1);
			get_random_bytes(&index1, sizeof(int)-1);
			get_random_bytes(&index2, sizeof(int)-1);
			index=index%8;
			index1=index1%9;
			index2=index2%7;
			//udelay(16000);
			wait_event_interruptible(wq, flag != 0);
			if(flag==2){
				index1=f_index;
				index1=index1%9;
			}
			flag=0;
		}
	}


	return 0;
}
static int devfreq_pandoon_func(struct devfreq *df,
					unsigned long *freq)
{
	//int lev,newlev;
	//unsigned long cur_freq;
	static bool first=1;
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

static struct devfreq_governor devfreq_pandoon = {
	.name = "pandoon",
	.get_target_freq = devfreq_pandoon_func,
	.event_handler = devfreq_pandoon_handler,
};

static int __init devfreq_pandoon_init(void)
{

	proc_create("pandoon",0666,NULL,&proc_fops);	
	init_waitqueue_head(&wq);
	freqs.capturing=false;
	return devfreq_add_governor(&devfreq_pandoon);
}
subsys_initcall(devfreq_pandoon_init);

static void __exit devfreq_pandoon_exit(void)
{
	int ret;

	ret = devfreq_remove_governor(&devfreq_pandoon);
	if (ret)
		pr_err("%s: failed remove governor %d\n", __func__, ret);
	remove_proc_entry("pandoon",NULL);
	return;
}
module_exit(devfreq_pandoon_exit);
MODULE_LICENSE("GPL");
