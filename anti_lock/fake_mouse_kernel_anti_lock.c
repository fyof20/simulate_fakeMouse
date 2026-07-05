/*
 * fake_mouse_kernel_anti_lock.c
 *
 * 防锁屏内核模块 - 模拟微小鼠标移动防止系统息屏/锁屏
 *
 * 编译: make -f Makefile.kernel_anti_lock
 * 或将本文件加入现有 Makefile 一起编译
 *
 * 加载:
 *   sudo insmod fake_mouse_kernel_anti_lock.ko
 *   sudo insmod fake_mouse_kernel_anti_lock.ko interval=30000 amplitude=2
 *
 * 查看日志: sudo dmesg | tail
 * 验证设备: cat /proc/bus/input/devices 或 evtest
 *
 * 卸载: sudo rmmod fake_mouse_kernel_anti_lock
 *
 * 参数说明:
 *   interval   移动间隔(毫秒)，默认 30000 (30秒)
 *   amplitude  移动幅度(像素)，默认 2
 */

#include <linux/module.h>
#include <linux/input.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

#define DEVICE_NAME "fake_mouse_kernel_anti_lock"
#define MODULE_NAME "fake_mouse_anti_lock"

/* 模块参数: 可通过 insmod interval=xxx amplitude=xxx 覆盖 */
static int interval = 30000;  /* 默认 30 秒 */
static int amplitude = 2;     /* 默认 2 像素 */

module_param(interval, int, 0644);
module_param(amplitude, int, 0644);

MODULE_PARM_DESC(interval, "移动间隔(毫秒)，默认 30000");
MODULE_PARM_DESC(amplitude, "移动幅度(像素)，默认 2");

static struct input_dev *fake_mouse_dev;
static struct timer_list shake_timer;

/*
 * 定时器回调: 每 interval ms 被调用一次，产生微小移动
 * 移动量极小，人眼几乎察觉不到，但系统会计为"活跃"
 */
static void shake_timer_callback(struct timer_list *t)
{
    (void)t;  /* 参数未使用，显式声明避免警告 */

    /* 发送 X 方向微小移动 */
    input_report_rel(fake_mouse_dev, REL_X, amplitude);

    /* 同步，通知内核所有事件已发送完 */
    input_sync(fake_mouse_dev);

    /* 重新调度定时器 */
    mod_timer(&shake_timer, jiffies + msecs_to_jiffies(interval));
}

static int __init fake_mouse_anti_lock_init(void)
{
    int err;

    printk(KERN_INFO "%s: module loading (interval=%d ms, amplitude=%d px)\n",
           MODULE_NAME, interval, amplitude);

    /* 分配 input_dev 结构体 */
    fake_mouse_dev = input_allocate_device();
    if (!fake_mouse_dev) {
        printk(KERN_ERR "%s: failed to allocate input device\n", MODULE_NAME);
        return -ENOMEM;
    }

    /* 设置设备信息 */
    fake_mouse_dev->name = DEVICE_NAME;
    fake_mouse_dev->phys = "fake-mouse-anti-lock/kernel";
    fake_mouse_dev->id.bustype = BUS_USB;
    fake_mouse_dev->id.vendor  = 0x1234;
    fake_mouse_dev->id.product = 0x5678;
    fake_mouse_dev->id.version = 0x0100;

    /* 设置支持的事件类型和事件码 */
    set_bit(EV_REL,  fake_mouse_dev->evbit);
    set_bit(REL_X,  fake_mouse_dev->relbit);
    set_bit(REL_Y,  fake_mouse_dev->relbit);

    set_bit(EV_KEY, fake_mouse_dev->evbit);
    set_bit(BTN_LEFT, fake_mouse_dev->keybit);

    /* 注册到 input 子系统 */
    err = input_register_device(fake_mouse_dev);
    if (err) {
        printk(KERN_ERR "%s: failed to register device\n", MODULE_NAME);
        input_free_device(fake_mouse_dev);
        return err;
    }

    /* 初始化并启动定时器 */
    timer_setup(&shake_timer, shake_timer_callback, 0);
    mod_timer(&shake_timer, jiffies + msecs_to_jiffies(interval));

    printk(KERN_INFO "%s: registered. Use 'evtest' to verify.\n", MODULE_NAME);
    return 0;
}

static void __exit fake_mouse_anti_lock_exit(void)
{
    del_timer_sync(&shake_timer);
    input_unregister_device(fake_mouse_dev);
    printk(KERN_INFO "%s: module unloaded\n", MODULE_NAME);
}

module_init(fake_mouse_anti_lock_init);
module_exit(fake_mouse_anti_lock_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("fyf");
MODULE_DESCRIPTION("A fake mouse anti-lock kernel module using input subsystem");
