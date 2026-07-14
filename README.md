# Core Ball (见缝插针) 自动闯关机器人

基于 **视觉 SLAM** 的自动闯关系统，使用 C++ / OpenCV / X11 实现。

## 环境要求

- Linux 图形桌面环境 (X11)
- OpenCV 4.4+
- GCC 支持 C++17
- libx11-dev, libxtst-dev

## 编译

```bash
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

## 使用方法

1. 在浏览器中打开游戏: https://www.arealme.com/coreball/cn/
2. 运行机器人:
   ```bash
   cd build
   ./coreball_bot
   ```
3. 切换到游戏窗口，机器人将自动识别并闯关
4. 按 `q` 或 `ESC` 退出

## 系统架构

```
┌──────────────┐    ┌───────────────┐    ┌──────────────┐    ┌──────────────┐
│ ScreenCapture │───▶│ VisionDetector │───▶│ RotationTracker│───▶│ GameController│
│  (X11 SHM)   │    │  (OpenCV)     │    │ (SLAM/Kalman) │    │  (Decision)  │
└──────────────┘    └───────────────┘    └──────────────┘    └──────────────┘
                                                                   │
                                                            ┌──────────────┐
                                                            │  AutoClicker │
                                                            │  (XTest)     │
                                                            └──────────────┘
```

## 模块说明

| 模块 | 文件 | 功能 |
|------|------|------|
| 屏幕截取 | `screen_capture.h/cpp` | X11 共享内存高速截屏 (~30-60 FPS) |
| 视觉检测 | `vision_detector.h/cpp` | HoughCircles 检测球体，边缘检测识别针 |
| 旋转追踪 | `rotation_tracker.h/cpp` | 光流法(Visual Odometry) + 卡尔曼滤波 |
| 游戏控制 | `game_controller.h/cpp` | 间隙检测、时机计算、决策控制 |
| 自动点击 | `auto_clicker.h/cpp` | XTest 扩展模拟鼠标点击 |
| 可视化 | `visualizer.h/cpp` | OpenCV 窗口实时显示检测结果 |

## SLAM 知识点

1. **视觉里程计 (Visual Odometry)**: 使用 Lucas-Kanade 稀疏光流追踪球体表面特征点，计算帧间旋转角度
2. **卡尔曼滤波 (Kalman Filter)**: 状态向量 [angle, angular_velocity]，融合光流观测与匀速旋转运动模型
3. **特征提取**: goodFeaturesToTrack 在球体表面环形区域提取角点特征
4. **运动模型**: 匀速旋转模型作为卡尔曼滤波的状态转移方程

## 可视化内容

- 球体检测结果（绿色圆）
- 已有针位置（红色线段 + 角度标注）
- 目标间隙（黄色高亮弧）
- 卡尔曼滤波预测角度（蓝色线）
- 点击指示器（红色闪烁 "SHOOT!"）
- 状态面板：角度、角速度、针数、间隙大小
- 实时曲线图：角度和角速度历史
