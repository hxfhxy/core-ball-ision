/// @file main.cpp
/// @brief Core Ball (见缝插针) 自动闯关机器人
///
/// 基于视觉 SLAM 的自动闯关系统：
///   - 屏幕截取: X11 共享内存高速截屏
///   - 视觉检测: OpenCV HoughCircles + 边缘检测识别球体和针
///   - 旋转追踪: Lucas-Kanade 光流(Visual Odometry) + 卡尔曼滤波(Kalman Filter)
///   - 决策控制: 间隙检测 + 时机计算
///   - 自动点击: XTest 扩展模拟鼠标
///   - 可视化:   OpenCV 窗口实时显示检测结果和状态
///
/// 使用方法:
///   1. 编译: mkdir build && cd build && cmake .. && make
///   2. 在浏览器中打开 https://www.arealme.com/coreball/cn/
///   3. 运行: ./coreball_bot
///   4. 切换到游戏窗口，机器人将自动识别并闯关
///   5. 按 q 或 ESC 退出
///
/// @author CoreBall Automation
/// @date 2026

#include "game_controller.h"
#include <csignal>
#include <cstdio>

static coreball::GameController* g_controller = nullptr;

/// 信号处理：Ctrl+C 优雅退出
void signalHandler(int signum) {
    (void)signum;
    printf("\n[Main] 收到退出信号，正在停止...\n");
    if (g_controller) {
        g_controller->stop();
    }
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    printf("========================================\n");
    printf("  Core Ball 自动闯关机器人\n");
    printf("  基于视觉SLAM的自动闯关系统\n");
    printf("========================================\n\n");

    // 注册信号处理
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // 创建并初始化控制器
    coreball::GameController controller;
    g_controller = &controller;

    if (!controller.init()) {
        fprintf(stderr, "[Main] 初始化失败，请检查:\n");
        fprintf(stderr, "  1. 是否在图形桌面环境下运行\n");
        fprintf(stderr, "  2. 是否安装了必要的库 (libx11-dev, libxtst-dev)\n");
        fprintf(stderr, "  3. OpenCV 是否正确安装\n");
        return 1;
    }

    printf("[Main] 初始化成功，开始自动闯关...\n");
    printf("[Main] 提示: 请先将游戏窗口切换到前台\n");
    printf("[Main] 按 q 或 ESC 退出\n\n");

    // 运行主循环
    controller.run();

    printf("[Main] 程序正常退出\n");
    return 0;
}
