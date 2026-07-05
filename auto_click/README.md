# Auto Click Assistant / 辅助点击工具

[English](#english) | [中文](#中文)

---

## English

An accessibility tool that automatically clicks a designated screen region when pixel changes are detected. Designed to assist users with motor impairments, or for automation testing scenarios.

### Features

- **Visual region selector**: Click and drag to select any screen area to monitor
- **Pixel change detection**: Uses OpenCV frame differencing to detect visual changes
- **Configurable threshold**: Adjust sensitivity from 0 (very sensitive) to 100 (stable)
- **Configurable interval**: Set detection frequency in milliseconds
- **Click types**: Support single-click and double-click
- **C-based core**: `click_core.c` uses libevdev for precise mouse event simulation

### Prerequisites

```bash
# System packages
sudo apt install xdotool python3-tk

# Python packages
pip install -r requirements.txt
```

### Build

```bash
make
```

### Usage

```bash
# Must run with sudo (xdotool + libevdev require privileges)
sudo python3 auto_click_gui.py
```

**Workflow:**
1. Click "选择监控区域" (Select Monitoring Region) to enter fullscreen selection mode
2. Click and drag to draw a rectangle around the target area
3. Adjust threshold (0-100) and detection interval (ms) in settings
4. Click "开始" (Start) to begin monitoring
5. When pixel changes in the selected region exceed the threshold, a click is triggered at the region's center

**Settings:**
| Setting | Description | Default |
|---------|-------------|---------|
| 触发阈值 (Threshold) | Pixel difference sensitivity (0-100) | 30 |
| 检测间隔 (Interval) | Detection frequency in ms | 500ms |
| 点击类型 (Click Type) | Single click or double click | 单击 (Single) |

### Architecture

```
auto_click_gui.py          # Python GUI (Tkinter)
    ├── RegionSelector      # Screen region selection overlay
    ├── Image detection      # OpenCV frame differencing
    └── ClickCore           # ctypes wrapper for click_core.so
        └── click_core.c    # libevdev mouse simulation (C)
```

### Limitations

- **X11 only**: `xdotool` does not support Wayland. This tool runs on X11 desktops.
- **Virtual machine**: Mouse movement via `xdotool` may not propagate to the host system's cursor in VM environments.

### License

GPL

---

## 中文

一款无障碍辅助工具，当检测到指定屏幕区域的像素变化时，自动在该区域中心位置模拟鼠标点击。旨在帮助运动障碍用户操作电脑，也适用于自动化测试场景。

### 功能特点

- **可视化选区**：点击拖拽选择屏幕上任意区域进行监控
- **像素变化检测**：使用 OpenCV 帧差法检测画面变化
- **可调阈值**：灵敏度可在 0（极敏感）到 100（极稳定）之间调节
- **可调间隔**：设置检测频率（毫秒）
- **点击类型**：支持单击和双击
- **C 语言核心**：`click_core.c` 使用 libevdev 精确模拟鼠标事件

### 依赖

```bash
# 系统依赖
sudo apt install xdotool python3-tk

# Python 依赖
pip install -r requirements.txt
```

### 编译

```bash
make
```

### 使用方法

```bash
# 必须使用 sudo 运行（xdotool + libevdev 需要权限）
sudo python3 auto_click_gui.py
```

**操作流程：**
1. 点击"选择监控区域"按钮进入全屏选区模式
2. 在屏幕上点击并拖拽，框选目标区域
3. 在设置中调节阈值（0-100）和检测间隔（毫秒）
4. 点击"开始"按钮启动监控
5. 当选定区域内的像素变化超过阈值时，自动在区域中心触发一次点击

**参数说明：**
| 设置 | 说明 | 默认值 |
|------|------|--------|
| 触发阈值 | 像素差异灵敏度 (0-100) | 30 |
| 检测间隔 | 检测频率（毫秒） | 500ms |
| 点击类型 | 单击或双击 | 单击 |

### 系统架构

```
auto_click_gui.py          # Python GUI (Tkinter)
    ├── RegionSelector      # 屏幕区域选择覆盖层
    ├── Image detection      # OpenCV 帧差检测
    └── ClickCore           # click_core.so 的 ctypes 封装
        └── click_core.c    # libevdev 鼠标模拟 (C 语言)
```

### 限制

- **仅支持 X11**：`xdotool` 不支持 Wayland。本工具仅在 X11 桌面环境下运行。
- **虚拟机环境**：`xdotool` 移动的鼠标坐标可能无法传到虚拟机宿主机系统的光标。
