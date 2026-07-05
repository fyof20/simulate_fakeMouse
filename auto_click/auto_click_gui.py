#!/usr/bin/env python3
"""
auto_click_gui.py

辅助点击工具 - 帮运动障碍用户自动点击屏幕上指定的区域。
当选定区域内检测到像素变化时，自动模拟鼠标点击。

依赖:
    - Pillow (截图)
    - opencv-python (帧差异检测)
    - xdotool (移动鼠标到目标位置)
    - click_core.so (模拟点击)

安装依赖:
    pip install -r requirements.txt
    make

运行:
    sudo python3 auto_click_gui.py
"""

import sys
import os
import ctypes
import threading
import time
import subprocess
from pathlib import Path

# 尝试导入 GUI 库
try:
    import tkinter as tk
    from tkinter import ttk
    from tkinter import ImageTk
except ImportError:
    print("错误: 需要 tkinter。请安装: sudo apt install python3-tk")
    sys.exit(1)

# 尝试导入图像处理库
try:
    from PIL import Image, ImageGrab
except ImportError:
    print("错误: 需要 Pillow。请安装: pip install Pillow")
    sys.exit(1)

try:
    import cv2
    import numpy as np
except ImportError:
    print("错误: 需要 opencv-python。请安装: pip install opencv-python")
    sys.exit(1)


# =============================================================================
# C 核心库加载
# =============================================================================

class ClickCore:
    """封装 click_core.so 的调用"""

    def __init__(self, lib_path="/usr/local/lib/libclick_core.so"):
        # 尝试多个可能的路径
        possible_paths = [
            lib_path,
            os.path.join(os.path.dirname(__file__), "libclick_core.so"),
            "./libclick_core.so",
        ]

        self.lib = None
        self.handle = None
        self.lib_path = None

        for path in possible_paths:
            if os.path.exists(path):
                try:
                    self.lib = ctypes.CDLL(path)
                    self.lib_path = path
                    break
                except OSError as e:
                    print(f"警告: 无法加载 {path}: {e}")

        if not self.lib:
            print("错误: 找不到 click_core.so，请先编译: make")
            sys.exit(1)

        # 设置函数签名
        self.lib.click_core_init.restype = ctypes.c_void_p
        self.lib.click_core_init.argtypes = []
        self.lib.click_core_click.restype = None
        self.lib.click_core_click.argtypes = [ctypes.c_void_p]
        self.lib.click_core_double_click.restype = None
        self.lib.click_core_double_click.argtypes = [ctypes.c_void_p]
        self.lib.click_core_destroy.restype = None
        self.lib.click_core_destroy.argtypes = [ctypes.c_void_p]

    def init(self):
        self.handle = self.lib.click_core_init()
        if not self.handle:
            print("错误: click_core 初始化失败")
            sys.exit(1)

    def click(self):
        if self.handle:
            self.lib.click_core_click(self.handle)

    def double_click(self):
        if self.handle:
            self.lib.click_core_double_click(self.handle)

    def destroy(self):
        if self.handle:
            self.lib.click_core_destroy(self.handle)
            self.handle = None


# =============================================================================
# 屏幕区域选择器
# =============================================================================

class RegionSelector:
    """全屏覆盖层，让用户拖拽选择监控区域"""

    def __init__(self, callback):
        self.callback = callback
        self.start_x = 0
        self.start_y = 0
        self.rect = None
        self.selection_done = False

    def select(self):
        """显示全屏选择界面，返回选中的区域 (x1, y1, x2, y2) 或 None"""
        self.root = tk.Tk()
        self.root.attributes('-fullscreen', True)
        self.root.attributes('-alpha', 0.3)
        self.root.attributes('-topmost', True)
        self.root.configure(bg='gray')

        self.canvas = tk.Canvas(self.root, cursor='cross', bg='gray',
                                highlightthickness=0)
        self.canvas.pack(fill='both', expand=True)

        self.canvas.bind('<Button-1>', self.on_press)
        self.canvas.bind('<B1-Motion>', self.on_drag)
        self.canvas.bind('<ButtonRelease-1>', self.on_release)

        # 按 ESC 取消
        self.root.bind('<Escape>', lambda e: self.root.quit())

        self.root.mainloop()

        if self.selection_done and self.rect:
            x1 = min(self.rect[0], self.rect[2])
            y1 = min(self.rect[1], self.rect[3])
            x2 = max(self.rect[0], self.rect[2])
            y2 = max(self.rect[1], self.rect[3])
            self.root.destroy()
            return (x1, y1, x2, y2)
        else:
            self.root.destroy()
            return None

    def on_press(self, event):
        self.start_x = event.x
        self.start_y = event.y
        if self.rect:
            self.canvas.delete(self.rect)
        self.rect = None

    def on_drag(self, event):
        if self.rect:
            self.canvas.delete(self.rect)
        self.rect = self.canvas.create_rectangle(
            self.start_x, self.start_y, event.x, event.y,
            outline='red', width=3
        )

    def on_release(self, event):
        if self.rect:
            coords = self.canvas.coords(self.rect)
            self.rect = tuple(coords)
            self.selection_done = True
            self.root.quit()


# =============================================================================
# 主应用
# =============================================================================

class AutoClickApp:
    """辅助点击工具主界面"""

    def __init__(self):
        self.running = False
        self.monitor_thread = None
        self.region = None  # (x1, y1, x2, y2)
        self.trigger_count = 0
        self.last_click_time = 0

        # 初始化 C 核心
        self.click_core = ClickCore()
        self.click_core.init()

        # 创建主窗口
        self.root = tk.Tk()
        self.root.title("辅助点击工具")
        self.root.geometry("500x400")
        self.root.resizable(False, False)

        self._build_ui()

    def _build_ui(self):
        """构建界面"""
        main_frame = ttk.Frame(self.root, padding="20")
        main_frame.pack(fill='both', expand=True)

        # --- 标题 ---
        title_label = ttk.Label(main_frame, text="辅助点击工具",
                                font=('Arial', 16, 'bold'))
        title_label.pack(pady=(0, 10))

        desc_label = ttk.Label(main_frame,
                               text="当选定区域内的画面发生变化时，"
                               "自动点击该区域中心",
                               foreground='gray')
        desc_label.pack(pady=(0, 20))

        # --- 选区按钮 ---
        select_frame = ttk.Frame(main_frame)
        select_frame.pack(fill='x', pady=5)

        self.select_btn = ttk.Button(select_frame, text="选择监控区域",
                                      command=self.on_select_region)
        self.select_btn.pack(side='left')

        self.region_label = ttk.Label(select_frame, text="未选择区域",
                                       foreground='gray')
        self.region_label.pack(side='left', padx=10)

        # --- 预览区域 ---
        self.preview_label = ttk.Label(main_frame, text="选定区域预览:")
        self.preview_label.pack(anchor='w', pady=(10, 5))

        self.preview_frame = ttk.Frame(main_frame, borderwidth=2,
                                        relief='solid', width=300, height=150)
        self.preview_frame.pack(pady=5)
        self.preview_frame.pack_propagate(False)

        self.preview_canvas = tk.Canvas(self.preview_frame, width=300,
                                         height=150, bg='#f0f0f0')
        self.preview_canvas.pack(fill='both', expand=True)

        # --- 参数设置 ---
        param_frame = ttk.LabelFrame(main_frame, text="设置", padding="10")
        param_frame.pack(fill='x', pady=10)

        # 阈值
        ttk.Label(param_frame, text="触发阈值 (0-100):").grid(row=0, column=0,
                                                                  sticky='w')
        self.threshold_var = tk.IntVar(value=30)
        threshold_spin = ttk.Spinbox(param_frame, from_=0, to=100,
                                      textvariable=self.threshold_var,
                                      width=10)
        threshold_spin.grid(row=0, column=1, padx=5, sticky='w')

        # 检测间隔
        ttk.Label(param_frame, text="检测间隔 (毫秒):").grid(row=1, column=0,
                                                                  sticky='w', pady=(5, 0))
        self.interval_var = tk.IntVar(value=500)
        interval_spin = ttk.Spinbox(param_frame, from_=100, to=10000,
                                     increment=100,
                                     textvariable=self.interval_var, width=10)
        interval_spin.grid(row=1, column=1, padx=5, pady=(5, 0), sticky='w')

        # 点击类型
        ttk.Label(param_frame, text="点击类型:").grid(row=2, column=0,
                                                        sticky='w', pady=(5, 0))
        self.click_type_var = tk.StringVar(value="单击")
        click_type_combo = ttk.Combobox(param_frame,
                                          values=["单击", "双击"],
                                          textvariable=self.click_type_var,
                                          state='readonly', width=8)
        click_type_combo.grid(row=2, column=1, padx=5, pady=(5, 0), sticky='w')

        # --- 触发计数 ---
        self.count_label = ttk.Label(main_frame, text="触发次数: 0",
                                      font=('Arial', 12))
        self.count_label.pack(pady=10)

        # --- 控制按钮 ---
        control_frame = ttk.Frame(main_frame)
        control_frame.pack(pady=10)

        self.start_btn = ttk.Button(control_frame, text="开始",
                                     command=self.on_start,
                                     width=12)
        self.start_btn.pack(side='left', padx=5)

        stop_btn = ttk.Button(control_frame, text="停止",
                              command=self.on_stop, width=12)
        stop_btn.pack(side='left', padx=5)

    def on_select_region(self):
        """选择监控区域"""
        # 隐藏主窗口
        self.root.withdraw()

        # 等待一下让窗口消失
        self.root.update()
        time.sleep(0.3)

        # 弹出全屏选择器
        selector = RegionSelector(None)
        region = selector.select()

        # 显示主窗口
        self.root.deiconify()
        self.root.update()

        if region:
            self.region = region
            x1, y1, x2, y2 = region
            self.region_label.config(
                text=f"已选择: ({x1},{y1}) - ({x2},{y2})",
                foreground='green'
            )
            # 更新预览
            self._update_preview(region)
        else:
            self.region_label.config(text="未选择区域", foreground='gray')

    def _update_preview(self, region):
        """更新区域预览"""
        x1, y1, x2, y2 = region
        width = x2 - x1
        height = y2 - y1

        if width <= 0 or height <= 0:
            return

        try:
            # 截取区域
            img = ImageGrab.grab(bbox=(x1, y1, x2, y2))

            # 缩放以适应预览框
            preview_w, preview_h = 296, 146
            scale = min(preview_w / width, preview_h / height)
            new_w = int(width * scale)
            new_h = int(height * scale)

            img = img.resize((new_w, new_h), Image.LANCZOS)

            # 转换为 Tkinter 可用的格式
            self.preview_photo = ImageTk.PhotoImage(img)

            # 在 canvas 上显示
            self.preview_canvas.delete('all')
            self.preview_canvas.create_image(
                (296 - new_w) // 2, (146 - new_h) // 2,
                anchor='nw', image=self.preview_photo
            )
        except Exception as e:
            print(f"预览更新失败: {e}")

    def on_start(self):
        """开始监控"""
        if not self.region:
            self.region_label.config(text="请先选择监控区域！",
                                      foreground='red')
            return

        if self.running:
            return

        self.running = True
        self.start_btn.config(state='disabled')
        self.select_btn.config(state='disabled')
        self.trigger_count = 0
        self.count_label.config(text="触发次数: 0")

        # 启动监控线程
        self.monitor_thread = threading.Thread(target=self._monitor_loop,
                                                daemon=True)
        self.monitor_thread.start()

    def on_stop(self):
        """停止监控"""
        self.running = False
        self.start_btn.config(state='normal')
        self.select_btn.config(state='normal')

    def _monitor_loop(self):
        """监控主循环"""
        x1, y1, x2, y2 = self.region
        center_x = (x1 + x2) // 2
        center_y = (y1 + y2) // 2
        width = x2 - x1
        height = y2 - y1

        threshold = self.threshold_var.get()
        interval = self.interval_var.get() / 1000.0  # 转为秒
        click_type = self.click_type_var.get()

        # 用于存储上一帧
        prev_frame = None

        while self.running:
            try:
                # 截取区域
                img = ImageGrab.grab(bbox=(x1, y1, x2, y2))
                curr_frame = cv2.cvtColor(
                    np.array(img), cv2.COLOR_RGB2BGR
                )
                curr_gray = cv2.cvtColor(curr_frame, cv2.COLOR_BGR2GRAY)

                if prev_frame is not None:
                    # 计算差异
                    diff = cv2.absdiff(prev_frame, curr_gray)
                    diff_pixels = np.sum(diff > threshold)
                    total_pixels = width * height
                    change_ratio = diff_pixels / total_pixels

                    # 触发阈值：变化像素超过 1%
                    if change_ratio > 0.01:
                        self._trigger_click(center_x, center_y, click_type)

                prev_frame = curr_gray

            except Exception as e:
                print(f"监控异常: {e}")

            time.sleep(interval)

    def _trigger_click(self, x, y, click_type):
        """触发点击"""
        current_time = time.time()

        # 防连击：距离上次点击至少 200ms
        if current_time - self.last_click_time < 0.2:
            return

        self.last_click_time = current_time
        self.trigger_count += 1

        # 在主线程更新计数
        self.root.after(0, lambda: self.count_label.config(
            text=f"触发次数: {self.trigger_count}"
        ))

        try:
            # 用 xdotool 移动鼠标到目标位置
            subprocess.run(['xdotool', 'mousemove', str(x), str(y)],
                           check=True, capture_output=True)

            # 用 click_core 模拟点击
            if click_type == "双击":
                self.click_core.double_click()
            else:
                self.click_core.click()

        except subprocess.CalledProcessError as e:
            print(f"xdotool 执行失败: {e}")
        except Exception as e:
            print(f"点击失败: {e}")

    def run(self):
        """运行应用"""
        try:
            self.root.mainloop()
        finally:
            self.running = False
            self.click_core.destroy()


# =============================================================================
# 入口
# =============================================================================

if __name__ == '__main__':
    # 检查 xdotool
    try:
        subprocess.run(['xdotool', '--version'],
                       check=True, capture_output=True)
    except (subprocess.CalledProcessError, FileNotFoundError):
        print("错误: 需要 xdotool。请安装: sudo apt install xdotool")
        sys.exit(1)

    # 检查权限（xdotool 需要 X11）
    print("注意: 需要在 X11 环境下运行（不支持 Wayland）")
    print("如果遇到权限问题，尝试: xhost +SI:localuser:root")
    print()

    app = AutoClickApp()
    app.run()
