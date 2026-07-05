/*
 * click_core.c
 *
 * 鼠标点击核心模块 - libevdev 实现
 * 提供 fake mouse 设备的创建和点击模拟
 *
 * 编译: gcc -shared -fPIC -o libclick_core.so click_core.c -levdev -lpthread
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>

#include "click_core.h"

struct click_core_handle {
    struct libevdev *dev;
    struct libevdev_uinput *uidev;
};

/*
 * click_core_init - 初始化 fake mouse 设备
 * 返回: handle 指针，失败返回 NULL
 */
void *click_core_init(void)
{
    struct click_core_handle *h;
    int err;

    h = calloc(1, sizeof(struct click_core_handle));
    if (!h)
        return NULL;

    /* 创建虚拟鼠标设备 */
    h->dev = libevdev_new();
    if (!h->dev) {
        free(h);
        return NULL;
    }

    libevdev_set_name(h->dev, "auto_click_core");

    /* 支持相对移动 */
    libevdev_enable_event_type(h->dev, EV_REL);
    libevdev_enable_event_code(h->dev, EV_REL, REL_X, NULL);
    libevdev_enable_event_code(h->dev, EV_REL, REL_Y, NULL);

    /* 支持按键 */
    libevdev_enable_event_type(h->dev, EV_KEY);
    libevdev_enable_event_code(h->dev, EV_KEY, BTN_LEFT, NULL);
    libevdev_enable_event_code(h->dev, EV_KEY, BTN_RIGHT, NULL);
    libevdev_enable_event_code(h->dev, EV_KEY, BTN_MIDDLE, NULL);

    /* 支持同步事件 */
    libevdev_enable_event_type(h->dev, EV_SYN);
    libevdev_enable_event_code(h->dev, EV_SYN, SYN_REPORT, NULL);

    /* 创建 uinput 设备 */
    err = libevdev_uinput_create_from_device(h->dev,
                                              LIBEVDEV_UINPUT_OPEN_MANAGED,
                                              &h->uidev);
    if (err != 0) {
        fprintf(stderr, "click_core: failed to create uinput device: %s\n",
                strerror(-err));
        libevdev_free(h->dev);
        free(h);
        return NULL;
    }

    return (void *)h;
}

/*
 * click_core_move - 移动鼠标（相对移动）
 * @handle: 设备句柄
 * @dx: X 方向移动量
 * @dy: Y 方向移动量
 */
void click_core_move(void *handle, int dx, int dy)
{
    struct click_core_handle *h = (struct click_core_handle *)handle;
    if (!h || !h->uidev)
        return;

    libevdev_uinput_write_event(h->uidev, EV_REL, REL_X, dx);
    libevdev_uinput_write_event(h->uidev, EV_REL, REL_Y, dy);
    libevdev_uinput_write_event(h->uidev, EV_SYN, SYN_REPORT, 0);
}

/*
 * click_core_click - 在当前位置模拟一次左键单击
 * @handle: 设备句柄
 */
void click_core_click(void *handle)
{
    struct click_core_handle *h = (struct click_core_handle *)handle;
    if (!h || !h->uidev)
        return;

    /* 按下 */
    libevdev_uinput_write_event(h->uidev, EV_KEY, BTN_LEFT, 1);
    libevdev_uinput_write_event(h->uidev, EV_SYN, SYN_REPORT, 0);

    /* 释放 */
    libevdev_uinput_write_event(h->uidev, EV_KEY, BTN_LEFT, 0);
    libevdev_uinput_write_event(h->uidev, EV_SYN, SYN_REPORT, 0);
}

/*
 * click_core_right_click - 在当前位置模拟一次右键单击
 * @handle: 设备句柄
 */
void click_core_right_click(void *handle)
{
    struct click_core_handle *h = (struct click_core_handle *)handle;
    if (!h || !h->uidev)
        return;

    libevdev_uinput_write_event(h->uidev, EV_KEY, BTN_RIGHT, 1);
    libevdev_uinput_write_event(h->uidev, EV_SYN, SYN_REPORT, 0);

    libevdev_uinput_write_event(h->uidev, EV_KEY, BTN_RIGHT, 0);
    libevdev_uinput_write_event(h->uidev, EV_SYN, SYN_REPORT, 0);
}

/*
 * click_core_double_click - 在当前位置模拟一次双击
 * @handle: 设备句柄
 */
void click_core_double_click(void *handle)
{
    struct click_core_handle *h = (struct click_core_handle *)handle;
    if (!h || !h->uidev)
        return;

    /* 第一次点击 */
    libevdev_uinput_write_event(h->uidev, EV_KEY, BTN_LEFT, 1);
    libevdev_uinput_write_event(h->uidev, EV_SYN, SYN_REPORT, 0);
    libevdev_uinput_write_event(h->uidev, EV_KEY, BTN_LEFT, 0);
    libevdev_uinput_write_event(h->uidev, EV_SYN, SYN_REPORT, 0);

    /* 短暂间隔 */
    usleep(50000);  /* 50ms */

    /* 第二次点击 */
    libevdev_uinput_write_event(h->uidev, EV_KEY, BTN_LEFT, 1);
    libevdev_uinput_write_event(h->uidev, EV_SYN, SYN_REPORT, 0);
    libevdev_uinput_write_event(h->uidev, EV_KEY, BTN_LEFT, 0);
    libevdev_uinput_write_event(h->uidev, EV_SYN, SYN_REPORT, 0);
}

/*
 * click_core_destroy - 销毁设备，释放资源
 * @handle: 设备句柄
 */
void click_core_destroy(void *handle)
{
    struct click_core_handle *h = (struct click_core_handle *)handle;
    if (!h)
        return;

    if (h->uidev)
        libevdev_uinput_destroy(h->uidev);
    if (h->dev)
        libevdev_free(h->dev);

    free(h);
}
