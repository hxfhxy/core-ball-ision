#include "rotation_tracker.h"
#include <cmath>

namespace coreball {

RotationTracker::RotationTracker()
    : rotation_speed_(0.0f), angular_acceleration_(0.0f), 
      initialized_(false), last_update_time_(0.0f) {}

RotationTracker::~RotationTracker() {}

void RotationTracker::init() {
    initialized_ = true;
    reset();
}

void RotationTracker::reset() {
    rotation_speed_ = 0.0f;
    angular_acceleration_ = 0.0f;
    prev_angles_.clear();
    last_update_time_ = 0.0f;
}

void RotationTracker::update(const std::vector<PinInfo>& detected_pins, float current_time) {
    if (!initialized_) return;

    std::vector<float> current_angles;
    for (const auto& pin : detected_pins) {
        current_angles.push_back(pin.angle);
    }

    if (last_update_time_ > 0.0f && !prev_angles_.empty() && !current_angles.empty()) {
        float dt = current_time - last_update_time_;
        
        // 对应他代码中的 if(dt > 0.001)
        if (dt > 0.001f) {
            float sum_diff = 0.0f;
            int count = 0;

            for (float ca : current_angles) {
                float best_diff = 1e9f;
                for (float pa : prev_angles_) {
                    float d = ca - pa;
                    // 归一化到 [-PI, PI]
                    while (d > static_cast<float>(M_PI)) d -= 2.0f * static_cast<float>(M_PI);
                    while (d < -static_cast<float>(M_PI)) d += 2.0f * static_cast<float>(M_PI);
                    
                    if (std::abs(d) < std::abs(best_diff)) {
                        best_diff = d;
                    }
                }
                if (std::abs(best_diff) < 0.5f) {
                    sum_diff += best_diff;
                    count++;
                }
            }

            if (count > 0) {
                float r = (sum_diff / count) / dt;
                
                // 核心修复：如果原始速度是 0，或者当前观测速度 r 和历史速度符号相反（发生反转）
                if (rotation_speed_ == 0.0f || (rotation_speed_ * r < 0.0f)) {
                    rotation_speed_ = r; // 抛弃历史包袱，瞬间切换到反转后的真实速度！
                } else {
                    angular_acceleration_ = std::abs(r - rotation_speed_) / dt;
                    rotation_speed_ = 0.8f * rotation_speed_ + 0.2f * r;
                }
            }
        }
    }

    prev_angles_ = current_angles;
    last_update_time_ = current_time;
}

}  // namespace coreball