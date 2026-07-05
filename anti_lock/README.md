# Fake Mouse Anti-Lock / 防锁屏工具

[English](#english) | [中文](#中文)

---

## English

A lightweight Linux tool that simulates tiny mouse movements to prevent screen lock / screensaver activation. Useful for long-running tasks like downloading, rendering, or live streaming.

### Features

- **Configurable interval**: Set movement frequency (default: 30 seconds)
- **Configurable amplitude**: Set movement distance in pixels (default: 2px)
- **Daemon mode**: Run in background, suitable for system startup
- **Two implementations**:
  - **User-space** (`fake_mouse_anti_lock.c`): Uses libevdev, portable and easy to use
  - **Kernel module** (`fake_mouse_kernel_anti_lock.c`): Runs as a Linux kernel module for deeper system integration

### Prerequisites

```bash
# Debian/Ubuntu
sudo apt install libevdev-dev gcc make

# Fedora/RHEL
sudo dnf install libevdev-devel gcc make
```

### Build

**User-space version:**
```bash
make -f Makefile.anti_lock user
```

**Kernel module (requires kernel headers):**
```bash
make -f Makefile.kernel_anti_lock
```

### Usage

```bash
# Run in foreground (every 30 seconds, move 2 pixels)
sudo ./fake_mouse_anti_lock --interval 30000 --amplitude 2

# Run as daemon (background)
sudo ./fake_mouse_anti_lock -i 30000 -a 2 -d

# Check PID
cat /var/run/fake_mouse_anti_lock.pid

# Stop daemon
sudo kill $(cat /var/run/fake_mouse_anti_lock.pid)
```

**Options:**
| Option | Description | Default |
|--------|-------------|---------|
| `-i, --interval` | Movement interval in milliseconds | 30000 (30s) |
| `-a, --amplitude` | Movement distance in pixels | 2 |
| `-d, --daemon` | Run in background | off |
| `-h, --help` | Show help | - |

**Kernel module parameters:**
```bash
sudo insmod fake_mouse_kernel_anti_lock.ko interval=30000 amplitude=2
```

### Verify

Use `evtest` to verify events are being sent:
```bash
sudo evtest
# Select "fake_mouse_anti_lock" device
# Observe EV_REL events every N seconds
```

### Systemd (Optional)

Create `/etc/systemd/system/fake-mouse-anti-lock.service`:
```ini
[Unit]
Description=Fake Mouse Anti-Lock Service
After=network.target

[Service]
Type=simple
ExecStart=/usr/local/bin/fake_mouse_anti_lock -i 30000 -a 2 -d
Restart=on-failure

[Install]
WantedBy=multi-user.target
```
```bash
sudo systemctl daemon-reload
sudo systemctl enable fake-mouse-anti-lock
sudo systemctl start fake-mouse-anti-lock
```

### License

GPL

---

## 中文

一个轻量级的 Linux 工具，通过模拟微小的鼠标移动来防止屏幕锁定或息屏。适用于长时间运行的任务（如下载、渲染、直播等）。

### 功能特点

- **可配置间隔**：设置移动频率（默认：30秒）
- **可配置幅度**：设置移动距离像素值（默认：2px）
- **守护进程模式**：后台运行，适合开机自启
- **两种实现方式**：
  - **用户态版**（`fake_mouse_anti_lock.c`）：使用 libevdev，便于移植和使用
  - **内核模块版**（`fake_mouse_kernel_anti_lock.c`）：作为 Linux 内核模块运行，系统集成度更高

### 依赖

```bash
# Debian/Ubuntu
sudo apt install libevdev-dev gcc make

# Fedora/RHEL
sudo dnf install libevdev-devel gcc make
```

### 编译

**用户态版本：**
```bash
make -f Makefile.anti_lock user
```

**内核模块（需要内核头文件）：**
```bash
make -f Makefile.kernel_anti_lock
```

### 使用方法

```bash
# 前台运行（每30秒移动2像素）
sudo ./fake_mouse_anti_lock --interval 30000 --amplitude 2

# 后台守护进程模式
sudo ./fake_mouse_anti_lock -i 30000 -a 2 -d

# 查看 PID
cat /var/run/fake_mouse_anti_lock.pid

# 停止后台进程
sudo kill $(cat /var/run/fake_mouse_anti_lock.pid)
```

**参数说明：**
| 参数 | 说明 | 默认值 |
|------|------|--------|
| `-i, --interval` | 移动间隔（毫秒） | 30000 (30秒) |
| `-a, --amplitude` | 移动幅度（像素） | 2 |
| `-d, --daemon` | 后台运行 | 关闭 |
| `-h, --help` | 显示帮助 | - |

**内核模块参数：**
```bash
sudo insmod fake_mouse_kernel_anti_lock.ko interval=30000 amplitude=2
```

### 验证

使用 `evtest` 验证事件是否正常发出：
```bash
sudo evtest
# 选择 "fake_mouse_anti_lock" 设备
# 观察每 N 秒是否有 EV_REL 事件输出
```

### Systemd 开机自启（可选）

创建 `/etc/systemd/system/fake-mouse-anti-lock.service`：
```ini
[Unit]
Description=Fake Mouse Anti-Lock Service
After=network.target

[Service]
Type=simple
ExecStart=/usr/local/bin/fake_mouse_anti_lock -i 30000 -a 2 -d
Restart=on-failure

[Install]
WantedBy=multi-user.target
```
```bash
sudo systemctl daemon-reload
sudo systemctl enable fake-mouse-anti-lock
sudo systemctl start fake-mouse-anti-lock
```

### 许可证

GPL
