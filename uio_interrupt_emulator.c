#include <linux/device.h>
#include <linux/hrtimer.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uio_driver.h>
#include <linux/version.h>
#include <linux/vmalloc.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jonas Knarbakk <jonas@knarbakk.no>");
MODULE_DESCRIPTION("A simple kernel module to emulate interrupts in UIO");

#define DRIVER_NAME "uio_interrupt_emulator"

static struct uio_info* uio_info;
static struct device* uio_dev;
static uint8_t irqs_enabled;

static struct hrtimer hr_timer;
static ktime_t kt;

static enum hrtimer_restart uio_interrupt_emulator_trigger(struct hrtimer* timer) {
    if (irqs_enabled) {
        uio_event_notify(uio_info);
        irqs_enabled = 0;
    }
    hrtimer_forward_now(timer, kt);
    return HRTIMER_RESTART;
}

static void uio_interrupt_emulator_release(struct device* dev) {
    printk(KERN_INFO "Releasing interrupt UIO device\n");
    kfree(uio_dev);
}

static int uio_interrupt_emulator_irq_control(struct uio_info* dev_info, s32 irq_on) {
    printk(KERN_INFO "Userspace requested IRQ generation %d\n", irq_on);
    irqs_enabled = irq_on != 0;

    return 0;
}

static int __init uio_interrupt_emulator_init(void) {

    uio_dev = kzalloc(sizeof(struct device), GFP_KERNEL);
    dev_set_name(uio_dev, DRIVER_NAME);
    uio_dev->release = uio_interrupt_emulator_release;
    if (device_register(uio_dev) < 0) {
        put_device(uio_dev);
        return -1;
    }

    uio_info = kzalloc(sizeof(struct uio_info), GFP_KERNEL);
    uio_info->name = DRIVER_NAME;
    uio_info->version = "0.1";
    uio_info->irq = UIO_IRQ_CUSTOM;
    uio_info->irqcontrol = uio_interrupt_emulator_irq_control;

    if (uio_register_device(uio_dev, uio_info) < 0) {
        device_unregister(uio_dev);
        kfree(uio_info);
        printk(KERN_INFO "Failed to register uio device\n");
        return -1;
    }

    kt = ktime_set(0, 1e+8); // 100 milliseconds
    hrtimer_setup(&hr_timer, &uio_interrupt_emulator_trigger, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    hrtimer_start(&hr_timer, kt, HRTIMER_MODE_REL);

    pr_info(DRIVER_NAME ": Module loaded and UIO device registered\n");
    return 0;
}

static void __exit uio_interrupt_emulator_exit(void) {
    hrtimer_cancel(&hr_timer);
    uio_unregister_device(uio_info);
    device_unregister(uio_dev);
    kfree(uio_info);
    pr_info(DRIVER_NAME ": Module unloaded and UIO device unregistered\n");
}

module_init(uio_interrupt_emulator_init);
module_exit(uio_interrupt_emulator_exit);
