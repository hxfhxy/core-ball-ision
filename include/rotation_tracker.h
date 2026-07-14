#ifndef ROTATION_TRACKER_H
#define ROTATION_TRACKER_H

#include "game_state.h"
#include <vector>

namespace coreball {

class RotationTracker {
public:
    RotationTracker();
    ~RotationTracker();

    void init();
    void reset();

    // 只需传入当前针的信息和时间
    void update(const std::vector<PinInfo>& detected_pins, float current_time);

    // 暴露速度和角加速度供外部使用
    float getRotationSpeed() const { return rotation_speed_; }
    float getAngularAcceleration() const { return angular_acceleration_; }

private:
    float rotation_speed_;
    float angular_acceleration_;
    bool initialized_;

    // 他代码中最精简的历史状态维护
    std::vector<float> prev_angles_;
    float last_update_time_;
};

}  // namespace coreball

#endif  // ROTATION_TRACKER_H