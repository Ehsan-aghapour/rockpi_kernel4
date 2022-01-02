/*
 *  linux/drivers/cpufreq/cpufreq_performance.c
 *
 *  Copyright (C) 2002 - 2003 Dominik Brodowski <linux@brodo.de>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

//Ehsan
//#include "cpufreq_pandoon.h"
#include <linux/cpufreq_pandoon.h>

//#include "cpufreq_governor.h"
#include <linux/delay.h>
bool pset1=0;
bool pset2=0;

struct cpufreq_policy *globpolicy1;
struct cpufreq_policy *globpolicy2;

/*static struct task_struct *wait_thread;

int tfunc(void* in){
	struct cpufreq_policy *p=(struct cpufreq_policy*)in;
	unsigned int maxload=0;
	maxload=dbs_update(p);
	while(true){
		maxload=dbs_update(p);
		printk("maxload is:%u\n",maxload);
		msleep(2000);
	}

}
*/
int my_func(struct cpufreq_policy *policy){
	//printk("cpu governor:1:pset1:%d,pset2:%d",pset1,pset2);
	if(pset1==0){
		printk("cpu governor:2:pset1:%d,pset2:%d",pset1,pset2);
		globpolicy1=policy;	
		pset1=1;
		return 0;
	}
	//printk("cpu governor:3:pset1:%d,pset2:%d",pset1,pset2);
	if(pset1==1 && pset2==0){
		printk("cpu governor:4:pset1:%d,pset2:%d",pset1,pset2);
		if(policy==globpolicy1){
			printk("cpu governor:5:pset1:%d,pset2:%d",pset1,pset2);
			return 0;
		}
		printk("cpu governor:6:pset1:%d,pset2:%d",pset1,pset2);
		globpolicy2=policy;
		pset2=1;
		printk("cpu governor:1:pset1:%d,pset2:%d",pset1,pset2);
		/*
		static bool first=1;
		if (first){
			first=0;
			//pthread_t pt;
			wait_thread=kthread_create(tfunc,(void*)policy,"wthread");
			if (wait_thread) {
	                	printk("Thread Created successfully\n");
        	        	wake_up_process(wait_thread);
        		} else
                		printk(KERN_INFO "Thread creation failed\n");
			return 0;
		}else
			return 0;
		
		*/

		//return 0;		
	}	
	return 0;
	
}

//Ehsan
//This function prototype is differnet in rockpi
//static void cpufreq_gov_pandoon_limits(struct cpufreq_policy *policy)
static int cpufreq_gov_pandoon_limits(struct cpufreq_policy *policy, unsigned int event)
{
	/*unsigned int index;
	unsigned int i;
	unsigned int freq_req;	
	struct cpufreq_frequency_table *freq_table = policy->freq_table;
	pr_debug("setting to %u kHz\n", policy->max);
	////__cpufreq_driver_target(policy, policy->max, CPUFREQ_RELATION_H);
	index=0;
	i=0;
	while(i<2000)
	{
		i=i+1;
		//index = cpufreq_frequency_table_target(policy, freq_next, relation);
		index=index+1;
		index=index%7;
		freq_req = freq_table[index].frequency;
		__cpufreq_driver_target(policy, freq_req, CPUFREQ_RELATION_H);
		udelay(16000);		
		//usleep_range(16000,16000);
		//ndelay(10000000);
	}*/


	/*ehsan : check if policies after setting are same
	if(policy!=globpolicy1 && policy!=globpolicy2)
		printk("new policyyyyyyyy");

	if(policy==globpolicy1){
		printk("policy1---min:%u, max:%u",policy->min,policy->max);
	}
	if(policy==globpolicy2){
		printk("policy2---min:%u, max:%u",policy->min,policy->max);
	}*/
	my_func(policy);
	//Ehsan rockpi
	return 0;
	
}


#ifndef CONFIG_CPU_FREQ_DEFAULT_GOV_PANDOON
static
#endif
struct cpufreq_governor cpufreq_gov_pandoon = {
	.name		= "pandoon",
	.owner		= THIS_MODULE,
	///Ehsan: rockpi has .governor instead of .limits
	//.limits		= cpufreq_gov_pandoon_limits,
	.governor		= cpufreq_gov_pandoon_limits,
};

static int __init cpufreq_gov_pandoon_init(void)
{
	return cpufreq_register_governor(&cpufreq_gov_pandoon);
}

static void __exit cpufreq_gov_pandoon_exit(void)
{
	cpufreq_unregister_governor(&cpufreq_gov_pandoon);
}

MODULE_AUTHOR("Dominik Brodowski <linux@brodo.de>");
MODULE_DESCRIPTION("CPUfreq policy governor 'pandooni'");
MODULE_LICENSE("GPL");

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_PANDOON
struct cpufreq_governor *cpufreq_default_governor(void)
{
	return &cpufreq_gov_pandoon;
}

fs_initcall(cpufreq_gov_pandoon_init);
#else
module_init(cpufreq_gov_pandoon_init);
#endif
module_exit(cpufreq_gov_pandoon_exit);
