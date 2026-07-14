#ifndef VISION_DETECTOR_H
#define VISION_DETECTOR_H

#include <opencv2/opencv.hpp>
#include "game_state.h"
#include <vector>

namespace coreball {

/// 视觉检测模块（阈值+轮廓分析）
/// 纯黑背景+纯白物体：灰度→二值化→形态学→轮廓→按面积分类
class VisionDetector {
public:
    VisionDetector();

    /// 一帧完成所有检测：中心球、环绕针、待发射球
    /// 比分步调用更高效（只做一次 threshold + findContours）
    struct DetectionResult {
        bool ball_found = false;
        cv::Point2f ball_center;
        float ball_radius = 0;
        std::vector<PinInfo> pins;
        bool waiting_ball_found = false;
        cv::Point2f waiting_ball_pos;
        cv::Mat polar_img;  // 极坐标展开图（调试用）
    };

    DetectionResult detect(const cv::Mat& gray);

    /// 检测游戏阶段
    GamePhase detectGamePhase(const cv::Mat& gray);

    /// 寻找绿色开始按钮（需要灰度输入，内部会转HSV）
    bool findStartButton(const cv::Mat& gray, cv::Point& button_pos);

private:
    // 面积阈值（根据窗口大小动态计算）
    float area_center_min_;   // 中心球最小面积
    float area_pin_min_;      // 针最小面积
    float area_pin_max_;      // 针最大面积
    float area_bullet_min_;   // 待发射球最小面积
    float area_bullet_max_;   // 待发射球最大面积

    void updateAreaThresholds(int frame_area);

    /// 计算点相对于球心的角度（弧度，12点方向为0，顺时针）
    float calcAngle(const cv::Point2f& center, const cv::Point2f& point);
};

}  // namespace coreball

#endif  // VISION_DETECTOR_H
