/*
 * Copyright (C) 2019 - www.ignitarium.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sysfs.h>
#include <linux/gpio.h>
#include <linux/kobject.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/mutex.h>

struct kobject *kobj_ref;
static struct class *soft_pwm_class;

// Variables to check status of registration of devices
// in sysfs
static int status1;
static int status2;
static int status3;

// Pin assignment
unsigned int ch1 = 466;  //yaw
unsigned int ch2 = 298;  //pitch
unsigned int ch3 = 388;  //roll

// Pointer for creating devices.
// One device for each channel
struct device *dev1;
struct device *dev2;
struct device *dev3;

// Character devices
static dev_t first;
static dev_t second;
static dev_t third;

// Delay variables
int min_t;
int mid_t;
int max_t;

// Variables to store finally sorted pin numbers
int first_pin;
int second_pin;
int third_pin;

unsigned int period = 20000;
unsigned int pulse1 = 1500;
unsigned int pulse2 = 1500;
unsigned int pulse3 = 1500;

int k;  // Counter for copying

DEFINE_MUTEX(mu_lock);

// Struct to store pwm pins and pulse width
typedef struct pwm {
	int pin;
	int t;
} pwm;

// A utility function to swap two elements
void swp(pwm *xp, pwm *yp)
{
	pwm temp = *xp;
	*xp = *yp;
	*yp = temp;
}

// An optimized version of Bubble Sort
int n = 3;
void bubbleSort(pwm arr[], int n)
{
	int i, j;
	int swapped;

	for (i = 0; i < n-1; i++) {
		swapped = 0;
		for (j = 0; j < n-i-1; j++) {
			if (arr[j].t > arr[j+1].t) {
				swp(&arr[j], &arr[j+1]);
				swapped = 1;
			}
		}
	// IF no two elements were swapped by inner loop, then break
		if (swapped == 0)
			break;
	}

// Copy sorted pulse widths to other variables
min_t = arr[0].t;
mid_t = arr[1].t;
max_t = arr[2].t;

// Sorting pin numbers according to pulse width
first_pin = arr[0].pin;
second_pin = arr[1].pin;
third_pin = arr[2].pin;
}

pwm pwm_arr[3]; // Struct array with pins and pulse width
pwm pwm_sort[3]; // Array of structs for sorting operation

static struct task_struct *thread_st;

static int thread_fn(void *unused)
{
	while (!kthread_should_stop()) {
			// Update pulse values in each cycle.
			// This piece of code can be moved to respective
			// store functions
			pwm_arr[0].t = pulse1;
			pwm_arr[1].t = pulse2;
			pwm_arr[2].t = pulse3;

			// Copying of array of structs followed by sorting
			mutex_lock(&mu_lock);
			for (k = 0; k <= 2; k++)
				pwm_sort[k] = pwm_arr[k];
			bubbleSort(pwm_sort, n);
			mutex_unlock(&mu_lock);
			local_irq_disable();
			preempt_disable();

			// All pins are set HIGH initially.
			// They are set low one by one in the
			// increasing order of pulse width
			
			gpio_set_value(first_pin, true);
			gpio_set_value(second_pin, true);
			gpio_set_value(third_pin, true);
			udelay(min_t);
			
			gpio_set_value(first_pin, false);
			udelay(mid_t-min_t);

			gpio_set_value(second_pin, false);
			udelay(max_t-mid_t);

			gpio_set_value(third_pin, false);
			preempt_enable();
			local_irq_enable();
			usleep_range((period - max_t) - 2000,
				     (period - max_t) + 2000);
	}
	printk(KERN_INFO "Thread Stopping\n");
	do_exit(0);
	return 0;
}

// Show and Store Functions
static ssize_t period_show(struct kobject *kobj, struct kobj_attribute *attr,
	char *buf)
{
	return sprintf(buf, "%d\n", period);
}

static ssize_t period_store(struct kobject *kobj, struct kobj_attribute *attr,
	const char *buf, size_t count)
{
	sscanf(buf, "%du", &period);
	printk(KERN_INFO "%d", period);
	return count;
}

// Yaw
static ssize_t pulse_show1(struct kobject *kobj, struct kobj_attribute *attr,
	char *buf)
{
	return sprintf(buf, "%d\n", pwm_arr[0].t);
}

static ssize_t pulse_store1(struct kobject *kobj, struct kobj_attribute *attr,
	const char *buf, size_t count)
{
	mutex_lock(&mu_lock);
	sscanf(buf, "%du", &pulse1);
	mutex_unlock(&mu_lock);
	return count;
}

// Pitch
static ssize_t pulse_show2(struct kobject *kobj, struct kobj_attribute *attr,
	char *buf)
{
	return sprintf(buf, "%d\n", pwm_arr[1].t);
}

static ssize_t pulse_store2(struct kobject *kobj, struct kobj_attribute *attr,
	const char *buf, size_t count)
{
	mutex_lock(&mu_lock);
	sscanf(buf, "%du", &pulse2);
	mutex_unlock(&mu_lock);
	return count;
}

// Roll
static ssize_t pulse_show3(struct kobject *kobj, struct kobj_attribute *attr,
	char *buf)
{
	return sprintf(buf, "%d\n", pwm_arr[2].t);
}

static ssize_t pulse_store3(struct kobject *kobj, struct kobj_attribute *attr,
	const char *buf, size_t count)
{
	mutex_lock(&mu_lock);
	sscanf(buf, "%du", &pulse3);
	mutex_unlock(&mu_lock);
	return count;
}

// Common attribute for period
static struct kobj_attribute period_attribute =
	__ATTR(period, 0644, period_show, period_store);

// Attrubute for yaw
static struct kobj_attribute pulse1_attribute =
	__ATTR(pulse, 0644, pulse_show1, pulse_store1);

// Attrubute for pitch
static struct kobj_attribute pulse2_attribute =
	__ATTR(pulse, 0644, pulse_show2, pulse_store2);

// Attrubute for roll
static struct kobj_attribute pulse3_attribute =
	__ATTR(pulse, 0644, pulse_show3, pulse_store3);

// Attribute group for each device.
// Each group contains respective period and pusle attributes
static struct attribute *attrs1[] = {
	&period_attribute.attr,
	&pulse1_attribute.attr,
	NULL,
};

static struct attribute_group attr_group1 = {
	.attrs = attrs1,
};

static struct attribute *attrs2[] = {
	&period_attribute.attr,
	&pulse2_attribute.attr,
	NULL,
};

static struct attribute_group attr_group2 = {
	.attrs = attrs2,
};

static struct attribute *attrs3[] = {
	&period_attribute.attr,
	&pulse3_attribute.attr,
	NULL,
};

static struct attribute_group attr_group3 = {
	.attrs = attrs3,
};

static int __init start_kernel(void)
{
	printk(KERN_INFO "Loading Module\n");
	printk(KERN_INFO "Driver initialised");

	// Character driver Major and Minor number allocation
	if (alloc_chrdev_region(&first, 0, 1, "soft_pwm") < 0) {
		return -1;
	}
	if (alloc_chrdev_region(&second, 0, 1, "soft_pwm") < 0) {
		return -1;
	}
	if (alloc_chrdev_region(&third, 0, 1, "soft_pwm") < 0) {
		return -1;
	}

	// Creating class named soft_pwm
	if ((soft_pwm_class = class_create(THIS_MODULE, "soft_pwm")) == NULL) {
		unregister_chrdev_region(first, 1);
		unregister_chrdev_region(second, 1);
		unregister_chrdev_region(third, 1);
		return -1;
	}

	// Creating each deviced inside the class
	dev1 = device_create(soft_pwm_class, NULL, first, NULL, "pwm_yaw");
	if (dev1) {
		status1 = sysfs_create_group(&dev1->kobj, &attr_group1);
		if (status1 == 0)
			printk(KERN_INFO "Registered device pwm_yaw");
		else
			device_unregister(dev1);
	} else {
		status1 = -ENODEV;
	}
	dev2 = device_create(soft_pwm_class, NULL, second, NULL, "pwm_pitch");
	if (dev2) {
		status2 = sysfs_create_group(&dev2->kobj, &attr_group2);
		if (status2 == 0)
			printk(KERN_INFO "Registered device pwm_pitch");
		else
			device_unregister(dev2);
	} else {
		status2 = -ENODEV;
	}
	dev3 = device_create(soft_pwm_class, NULL, third, NULL, "pwm_roll");
	if (dev3) {
		status3 = sysfs_create_group(&dev3->kobj, &attr_group3);
		if (status3 == 0)
			printk(KERN_INFO "Registered device pwm_roll");
		else
			device_unregister(dev3);
	} else {
		status3 = -ENODEV;
	}

	// Creating thread for pwm
	printk(KERN_INFO "Creating Thread\n");
	thread_st = kthread_run(&thread_fn, NULL, "pwm_thread");
	if (thread_st) {
		
		gpio_request(ch1, "pwm");
		gpio_request(ch2, "pwm");
		gpio_request(ch3, "pwm");
		gpio_direction_output(ch1, false);
		gpio_direction_output(ch2, false);
		gpio_direction_output(ch3, false);
		printk(KERN_INFO "Thread Created successfully\n");
	} else {
		printk(KERN_ERR "Thread creation failed\n");
	}
	mutex_init(&mu_lock);

	// Assigning pins to array of structs
	pwm_arr[0].pin = ch1;
	pwm_arr[1].pin = ch2;
	pwm_arr[2].pin = ch3;
	return 0;
 }

static void __exit stop_kernel(void)
{
	printk(KERN_INFO "cleaning up\n");
	
	gpio_free(ch1);
	gpio_free(ch2);
	gpio_free(ch3);
	
	if (thread_st) {
		kthread_stop(thread_st);
		printk(KERN_INFO "Thread stopped");
	}

	device_destroy(soft_pwm_class, first);
	device_destroy(soft_pwm_class, second);
	device_destroy(soft_pwm_class, third);

	class_destroy(soft_pwm_class);

	unregister_chrdev_region(first, 1);
	unregister_chrdev_region(second, 1);
	unregister_chrdev_region(third, 1);

	printk(KERN_INFO "Device unregistered");
	printk(KERN_INFO "Removing LKM");
	kobject_put(kobj_ref);
}

module_init(start_kernel);
module_exit(stop_kernel);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Akhil P");
