/*
 * fake_mouse_kernel.c
 *
 * 一个最简单的 Linux 内核模块，模拟假鼠标设备。
 * 直接注册到 input 子系统，不需要任何用户态库(libevdev)。
 *
 * 编译: make
 * 加载: sudo insmod fake_mouse_kernel.ko
 * 查看日志: sudo dmesg | tail
 * 验证设备: cat /proc/bus/input/devices 或 evtest
 * 卸载: sudo rmmod fake_mouse_kernel
 */

#include <linux/module.h>
#include <linux/input.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

#define DEVICE_NAME "fake_mouse_kernel"
#define MODULE_NAME "fake_mouse"

static struct input_dev *fake_mouse_dev;
static struct timer_list shake_timer;
static int moving = 1;

/* 定时器回调，每 50ms 被调用一次，产生随机移动 */
static void shake_timer_callback(struct timer_list *t)
{
    int direction;
    int amplitude;

    if (!moving)
        return;

    direction = (jiffies % 2 == 0) ? REL_X : REL_Y;
    amplitude = (s32)(jiffies % 41) - 20;

    /* 报告相对移动事件 */
    input_report_rel(fake_mouse_dev, direction, amplitude);

    /* 同步，通知内核所有事件已发送完 */
    input_sync(fake_mouse_dev);

    /* 重新调度定时器，50ms 后再次触发 */
    mod_timer(&shake_timer, jiffies + msecs_to_jiffies(50));
}

static int __init fake_mouse_init(void)
{
    int err;

    printk(KERN_INFO "%s: module loading\n", MODULE_NAME);

    /*  分配一个 input_dev 结构体 */
    fake_mouse_dev = input_allocate_device();
    if (!fake_mouse_dev) {
        printk(KERN_ERR "%s: failed to allocate input device\n", MODULE_NAME);
        return -ENOMEM;
    }

    /* 设置设备信息，这些会出现在 /proc/bus/input/devices 里
        因为是fake mouse，所以随机命名
    */
    fake_mouse_dev->name = DEVICE_NAME;
    fake_mouse_dev->phys = "fake-mouse/kernel";
    fake_mouse_dev->id.bustype = BUS_USB;
    fake_mouse_dev->id.vendor  = 0x1234;
    fake_mouse_dev->id.product = 0x5678;
    fake_mouse_dev->id.version = 0x0100;

    /*  设置支持的事件类型和事件码 */
    set_bit(EV_REL,  fake_mouse_dev->evbit);   /* 支持相对移动事件 */
    set_bit(REL_X,  fake_mouse_dev->relbit);  /* X 轴移动 */
    set_bit(REL_Y,  fake_mouse_dev->relbit);  /* Y 轴移动 */

    set_bit(EV_KEY, fake_mouse_dev->evbit);   /* 支持按键事件 */
    set_bit(BTN_LEFT, fake_mouse_dev->keybit); /* 左键 */

    /* 注册到 input 子系统，注册成功后设备可见 */
    err = input_register_device(fake_mouse_dev);
    if (err) {
        printk(KERN_ERR "%s: failed to register device\n", MODULE_NAME);
        input_free_device(fake_mouse_dev);
        return err;
    }

    /*  初始化并启动定时器，产生周期性移动事件 */
    timer_setup(&shake_timer, shake_timer_callback, 0);
    mod_timer(&shake_timer, jiffies + msecs_to_jiffies(50));

    printk(KERN_INFO "%s: registered. Use 'evtest' to verify.\n", MODULE_NAME);
    return 0;
}

static void __exit fake_mouse_exit(void)
{
    moving = 0;
    del_timer_sync(&shake_timer);
    input_unregister_device(fake_mouse_dev);
    printk(KERN_INFO "%s: module unloaded\n", MODULE_NAME);
}

module_init(fake_mouse_init);
module_exit(fake_mouse_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("fyf");
MODULE_DESCRIPTION("A fake mouse kernel module using input subsystem");
