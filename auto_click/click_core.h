/*
 * click_core.h
 *
 * 鼠标点击核心模块头文件
 * 供 Python ctypes 调用
 */

#ifndef CLICK_CORE_H
#define CLICK_CORE_H

/*
 * click_core_init - 初始化 fake mouse 设备
 * 返回: handle 指针，失败返回 NULL
 */
void *click_core_init(void);

/*
 * click_core_move - 移动鼠标（相对移动）
 * @handle: 设备句柄
 * @dx: X 方向移动量
 * @dy: Y 方向移动量
 */
void click_core_move(void *handle, int dx, int dy);

/*
 * click_core_click - 在当前位置模拟一次左键单击
 * @handle: 设备句柄
 */
void click_core_click(void *handle);

/*
 * click_core_right_click - 在当前位置模拟一次右键单击
 * @handle: 设备句柄
 */
void click_core_right_click(void *handle);

/*
 * click_core_double_click - 在当前位置模拟一次双击
 * @handle: 设备句柄
 */
void click_core_double_click(void *handle);

/*
 * click_core_destroy - 销毁设备，释放资源
 * @handle: 设备句柄
 */
void click_core_destroy(void *handle);

#endif /* CLICK_CORE_H */
