#ifndef GAME_CONTROLLER_H
#define GAME_CONTROLLER_H

#include "game_state.h"
#include "screen_capture.h"
#include "vision_detector.h"
#include "rotation_tracker.h"
#include "auto_clicker.h"
#include "visualizer.h"
#include <atomic>

namespace coreball {

/// 游戏控制器：协调各模块实现自动闯关
class GameController {
public:
    GameController();
    ~GameController();

    /// 初始化所有模块
    bool init();

    /// 主循环
    void run();

    /// 停止主循环
    void stop() { running_ = false; }

private:
    // 子模块
    ScreenCapture capture_;
    VisionDetector detector_;
    RotationTracker tracker_;
    AutoClicker clicker_;
    Visualizer visualizer_;

    // 游戏状态
    GameState state_;
    GamePhase phase_;
    int current_level_;
    int total_inserted_;
    cv::Mat polar_img_;  // 极坐标展开图（调试用）

    // 控制
    std::atomic<bool> running_;
    bool ball_initialized_;

    // 配置参数
    int click_x_;        // 点击位置 X（屏幕中央）
    int click_y_;        // 点击位置 Y（屏幕下方）
    float angle_threshold_;  // 角度匹配阈值（弧度）
    float ball_travel_time_; // 球飞行到中心的时间（秒）
    double last_click_time_ = 0.0;  // 上次点击时间

    /// 更新游戏状态
    void updateState(const cv::Mat& frame);

    /// 寻找最大间隙
    void findBestGap();

    /// 计算是否应该点击
    void calculateClickTiming();

    /// 执行点击
    void performClick();

    /// 处理关卡切换
    void handlePhaseTransition(GamePhase new_phase);

    /// 归一化角度到 [0, 2π)
    static float normalizeAngle(float angle);

    /// 计算两个角度之间的最短差值
    static float angleDiff(float a, float b);
};

}  // namespace coreball

#endif  // GAME_CONTROLLER_H
