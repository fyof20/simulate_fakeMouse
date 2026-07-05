/*
 * fake_mouse_anti_lock.c
 *
 * 防锁屏工具 - 模拟微小鼠标移动防止系统息屏/锁屏
 *
 * 编译: gcc -o fake_mouse_anti_lock fake_mouse_anti_lock.c -levdev -lpthread
 *
 * 用法:
 *   sudo ./fake_mouse_anti_lock --interval 30000 --amplitude 2 --daemon
 *   sudo ./fake_mouse_anti_lock -i 30000 -a 2 -d
 *
 * 参数说明:
 *   -i, --interval   移动间隔(毫秒)，默认 30000 (30秒)
 *   -a, --amplitude  移动幅度(像素)，默认 2
 *   -d, --daemon     后台运行
 *   -h, --help       显示帮助
 *
 * 停止后台进程:
 *   sudo kill $(cat /var/run/fake_mouse_anti_lock.pid)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define PID_FILE "/var/run/fake_mouse_anti_lock.pid"

static volatile sig_atomic_t running = 1;
static int interval_ms = 30000;
static int amplitude = 2;

void signal_handler(int sig)
{
    (void)sig;
    running = 0;
}

void write_pid_file(void)
{
    FILE *fp = fopen(PID_FILE, "w");
    if (fp) {
        fprintf(fp, "%d\n", getpid());
        fclose(fp);
    }
}

void remove_pid_file(void)
{
    unlink(PID_FILE);
}

void daemonize(void)
{
    pid_t pid;

    /* 第一次 fork */
    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        /* 父进程退出 */
        exit(EXIT_SUCCESS);
    }

    /* 子进程成为会话领导者 */
    if (setsid() < 0) {
        perror("setsid");
        exit(EXIT_FAILURE);
    }

    /* 第二次 fork，防止获取新的控制终端 */
    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    /* 关闭标准输入/输出/错误，重定向到 /dev/null */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);
    open("/dev/null", O_RDWR);

    /* 写入 PID 文件 */
    write_pid_file();
}

void print_help(const char *prog)
{
    printf("用法: %s [选项]\n", prog);
    printf("防锁屏工具 - 模拟微小鼠标移动防止系统息屏/锁屏\n\n");
    printf("选项:\n");
    printf("  -i, --interval  MS   移动间隔(毫秒)，默认 30000 (30秒)\n");
    printf("  -a, --amplitude PX   移动幅度(像素)，默认 2\n");
    printf("  -d, --daemon         后台运行\n");
    printf("  -h, --help           显示此帮助\n");
    printf("\n示例:\n");
    printf("  %s -i 30000 -a 2 -d     # 每30秒移动2像素，后台运行\n", prog);
    printf("  %s -i 60000 -a 1       # 每分钟移动1像素，前台运行\n", prog);
}

int main(int argc, char *argv[])
{
    struct libevdev *dev;
    struct libevdev_uinput *uidev = NULL;
    int daemon_mode = 0;

    /* 命令行参数解析 */
    static struct option long_options[] = {
        {"interval",  required_argument, 0, 'i'},
        {"amplitude", required_argument, 0, 'a'},
        {"daemon",    no_argument,       0, 'd'},
        {"help",      no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "i:a:dh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'i':
                interval_ms = atoi(optarg);
                break;
            case 'a':
                amplitude = atoi(optarg);
                break;
            case 'd':
                daemon_mode = 1;
                break;
            case 'h':
                print_help(argv[0]);
                return 0;
            default:
                print_help(argv[0]);
                return 1;
        }
    }

    /* 参数校验 */
    if (interval_ms <= 0) {
        fprintf(stderr, "错误: 间隔必须大于 0\n");
        return 1;
    }
    if (amplitude <= 0) {
        fprintf(stderr, "错误: 幅度必须大于 0\n");
        return 1;
    }

    /* 信号处理 */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Daemon 模式 */
    if (daemon_mode) {
        daemonize();
    }

    /* 创建 fake mouse 设备 */
    dev = libevdev_new();
    libevdev_set_name(dev, "fake_mouse_anti_lock");
    libevdev_enable_event_type(dev, EV_REL);
    libevdev_enable_event_code(dev, EV_REL, REL_X, NULL);
    libevdev_enable_event_code(dev, EV_REL, REL_Y, NULL);
    libevdev_enable_event_type(dev, EV_SYN);
    libevdev_enable_event_code(dev, EV_SYN, SYN_REPORT, NULL);

    int err = libevdev_uinput_create_from_device(dev, LIBEVDEV_UINPUT_OPEN_MANAGED, &uidev);
    if (err != 0) {
        fprintf(stderr, "错误: 无法创建 uinput 设备: %s\n", strerror(-err));
        libevdev_free(dev);
        return 1;
    }

    if (!daemon_mode) {
        printf("防锁屏运行中 (Ctrl+C 停止)\n");
        printf("间隔: %d ms, 幅度: %d px\n", interval_ms, amplitude);
        printf("PID: %d\n", getpid());
    }

    /* 主循环 - 定时发送微小移动 */
    while (running) {
        /* 发送 X 方向移动 */
        libevdev_uinput_write_event(uidev, EV_REL, REL_X, amplitude);
        libevdev_uinput_write_event(uidev, EV_SYN, SYN_REPORT, 0);

        /* 等待下一次触发 */
        usleep(interval_ms * 1000);
    }

    /* 清理 */
    libevdev_uinput_destroy(uidev);
    libevdev_free(dev);

    if (daemon_mode) {
        remove_pid_file();
    }

    if (!daemon_mode) {
        printf("已停止.\n");
    }

    return 0;
}
