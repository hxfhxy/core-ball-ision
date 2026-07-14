#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <opencv2/opencv.hpp>
#include <vector>

namespace coreball {

/// 单根针的信息（极坐标表示）
struct PinInfo {
    float angle;     // 角度（弧度，相对于12点钟方向，顺时针为正）
    float distance;  // 距球心的距离（像素）
    cv::Point2f tip; // 针尖端在屏幕坐标系中的位置
};

/// 游戏状态快照
struct GameState {
    cv::Point2f center;              // 球心屏幕坐标
    float radius;                    // 球体半径（像素）
    float current_angle;             // 当前旋转角度（弧度）
    float angular_velocity;          // 旋转角速度（rad/s）
    std::vector<PinInfo> pins;       // 已有针的信息
    int remaining_pins;              // 剩余待插针数
    bool is_playing;                 // 是否处于游戏中
    float target_gap_center;         // 目标间隙中心角度（弧度）
    float target_gap_size;           // 目标间隙大小（弧度）
    bool should_click;               // 是否应该点击
    bool ball_detected;              // 是否检测到球体
    float safe_angle;                // 安全角度（弧度）
    float min_dist;                  // 到最近针的距离（弧度）

    GameState()
        : center(0, 0), radius(0), current_angle(0), angular_velocity(0),
          remaining_pins(0), is_playing(false), target_gap_center(0),
          target_gap_size(0), should_click(false), ball_detected(false),
          safe_angle(0), min_dist(0) {}
};

/// 游戏阶段枚举
enum class GamePhase {
    UNKNOWN,       // 未知状态
    WAITING,       // 等待开始/关卡间
    PLAYING,       // 游戏进行中
    LEVEL_CLEAR,   // 关卡通过
    GAME_OVER      // 游戏结束
};

}  // namespace coreball

#endif  // GAME_STATE_H
