#include "visualizer.h"
#include <cmath>

namespace coreball {

Visualizer::Visualizer() : initialized_(false) {}

Visualizer::~Visualizer() {
    cv::destroyAllWindows();
}

bool Visualizer::init() {
    cv::namedWindow("CoreBall Bot", cv::WINDOW_AUTOSIZE);
    cv::namedWindow("Polar View", cv::WINDOW_AUTOSIZE);
    initialized_ = true;
    return true;
}

void Visualizer::update(const cv::Mat& frame, const GameState& state,
                         GamePhase phase, const RotationTracker& tracker,
                         const cv::Mat& polar_img) {
    if (!initialized_) return;

    if (frame.channels() == 1)
        cv::cvtColor(frame, display_, cv::COLOR_GRAY2BGR);
    else
        display_ = frame.clone();

    if (state.ball_detected && state.radius > 0) {
        int r = std::min(static_cast<int>(state.radius), std::min(display_.cols, display_.rows) / 2);
        if (r > 0) {
            // 中心球（绿色圆）
            cv::circle(display_, state.center, r, cv::Scalar(0, 255, 0), 2);
            cv::circle(display_, state.center, 3, cv::Scalar(0, 0, 255), -1);

            // 针（红色圆）
            for (auto& pin : state.pins) {
                cv::circle(display_, pin.tip, 8, cv::Scalar(0, 0, 255), 2);
            }

           // 画安全角度弧线（黄色）— 直接用游戏坐标画线段
            if (state.safe_angle > 0) {
                int arc_r = static_cast<int>(r * 2.5f);
                
                float w = tracker.getRotationSpeed(); 
                float predict_time = 0.15f; 
                float target = static_cast<float>(M_PI); // 180度，正下方
                float base_sa = state.safe_angle;

                // 在游戏坐标系中：
                // 角度比 M_PI 小的在屏幕右边 (例如 170度)
                // 角度比 M_PI 大的在屏幕左边 (例如 190度)
                float angle_start = target - base_sa; // 右边界
                float angle_end = target + base_sa;   // 左边界

                // 核心物理修正：迎风面拉长护盾
                if (w > 0) { 
                    // 顺时针旋转 (w > 0)：
                    // 轮盘底部的针【从右往左】扫过枪口！
                    // 危险全在右边，所以我们要把右边界拉长（让起点的角度变得更小）
                    angle_start -= w * predict_time; 
                } else if (w < 0) {
                    // 逆时针旋转 (w < 0)：
                    // 轮盘底部的针【从左往右】扫过枪口！
                    // 危险全在左边，所以我们要把左边界拉长（让终点的角度变得更大）
                    // 注意：w 是负数，减去一个负数 = 加上一个正数
                    angle_end -= w * predict_time; 
                }

                // 画多条线段组成弧线
                int segments = 20;
                for (int i = 0; i < segments; i++) {
                    float ratio1 = static_cast<float>(i) / segments;
                    float ratio2 = static_cast<float>(i + 1) / segments;
                    float a1 = angle_start + (angle_end - angle_start) * ratio1;
                    float a2 = angle_start + (angle_end - angle_start) * ratio2;
                    
                    // 游戏坐标转屏幕坐标：x = cx + r*sin(a), y = cy - r*cos(a)[cite: 18]
                    cv::Point2f p1(state.center.x + arc_r * std::sin(a1),
                                   state.center.y - arc_r * std::cos(a1));
                    cv::Point2f p2(state.center.x + arc_r * std::sin(a2),
                                   state.center.y - arc_r * std::cos(a2));
                    cv::line(display_, p1, p2, cv::Scalar(0, 255, 255), 3);
                }

                // 画两条边界线
                cv::Point2f p_start(state.center.x + arc_r * std::sin(angle_start),
                                    state.center.y - arc_r * std::cos(angle_start));
                cv::Point2f p_end(state.center.x + arc_r * std::sin(angle_end),
                                  state.center.y - arc_r * std::cos(angle_end));
                cv::line(display_, state.center, p_start, cv::Scalar(0, 255, 255), 1);
                cv::line(display_, state.center, p_end, cv::Scalar(0, 255, 255), 1);

                // 显示角度值
                char buf[32];
                float total_angle = angle_end - angle_start;
                std::snprintf(buf, sizeof(buf), "safe:%.1f", total_angle * 180.0f / static_cast<float>(M_PI));
              cv::putText(display_, buf,
                            cv::Point(20, 30), // 原本这里是 state.center.x + arc_r + 5
                            cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 255), 2);
            }
        }
    }

    cv::imshow("CoreBall Bot", display_);

    // 显示极坐标展开图
    if (!polar_img.empty()) {
        cv::Mat polar_display;
        cv::cvtColor(polar_img, polar_display, cv::COLOR_GRAY2BGR);
        cv::imshow("Polar View", polar_display);
    }
}

int Visualizer::waitKey(int delay_ms) {
    return cv::waitKey(delay_ms);
}

}  // namespace coreball
