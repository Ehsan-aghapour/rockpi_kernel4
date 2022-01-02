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



#include <linux/cpufreq.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>

extern bool pset1;
extern bool pset2;

extern struct cpufreq_policy *globpolicy1;
extern struct cpufreq_policy *globpolicy2;


