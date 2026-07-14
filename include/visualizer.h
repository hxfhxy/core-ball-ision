#ifndef VISUALIZER_H
#define VISUALIZER_H

#include <opencv2/opencv.hpp>
#include "game_state.h"
#include "rotation_tracker.h"

namespace coreball {

/// 可视化模块：显示检测结果和状态
class Visualizer {
public:
    Visualizer();
    ~Visualizer();

    bool init();

    void update(const cv::Mat& frame, const GameState& state,
                GamePhase phase, const RotationTracker& tracker,
                const cv::Mat& polar_img = cv::Mat());

    int waitKey(int delay_ms);

private:
    cv::Mat display_;
    bool initialized_;

    void drawDetections(const GameState& state);
    void drawStatusPanel(const GameState& state, GamePhase phase);
    static const char* phaseToString(GamePhase phase);
};

}  // namespace coreball

#endif  // VISUALIZER_H
