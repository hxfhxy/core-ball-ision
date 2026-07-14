#include "game_controller.h"
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <chrono>
#include <thread>

namespace coreball {

static int g_warmup_frames = 0;
static double g_frame_capture_time = 0.0;   
static float g_last_w = 0.0f;         // 新增：记录上一帧的角速度
static double g_reversal_time = 0.0;  // 新增：记录发生反转的时间点

GameController::GameController()
    : phase_(GamePhase::UNKNOWN), current_level_(1), total_inserted_(0),
      running_(false), ball_initialized_(false),
      click_x_(0), click_y_(0),
      angle_threshold_(0.15f),
      ball_travel_time_(0.14f) {}

GameController::~GameController() {
    stop();
}

bool GameController::init() {
    if (!capture_.init()) {
        fprintf(stderr, "[Controller] 屏幕截取初始化失败\n");
        return false;
    }
    capture_.setCaptureRegion(500, 100, 750, 900);
    if (!clicker_.init()) {
        fprintf(stderr, "[Controller] 自动点击初始化失败\n");
        return false;
    }
    click_x_ = capture_.roiX() + capture_.screenWidth() / 2;
    click_y_ = capture_.roiY() + capture_.screenHeight() * 3 / 4;
    visualizer_.init();
    return true;
}

void GameController::run() {
    running_ = true;
    printf("[Controller] 开始运行，采用全屏视觉闭环...\n");
    std::this_thread::sleep_for(std::chrono::seconds(2));
    bool prev_ball_detected = false;

    while (running_) {
        g_frame_capture_time = static_cast<double>(cv::getTickCount()) / cv::getTickFrequency();
        cv::Mat frame = capture_.captureFullScreen();
        if (frame.empty()) {
            cv::waitKey(1);
            continue;
        }

        updateState(frame);

        if (prev_ball_detected && !state_.ball_detected) {
            printf("[关卡] 球消失，重置追踪器\n");
            tracker_.reset();
            ball_initialized_ = false;
        }
        prev_ball_detected = state_.ball_detected;

        if (state_.ball_detected) {
            findBestGap();

            if (state_.should_click) {
                double now = static_cast<double>(cv::getTickCount()) / cv::getTickFrequency();
                if (now - last_click_time_ > 0.3) {
                    printf("[点击] 发射！\n");
                    clicker_.click(click_x_, click_y_);
                    total_inserted_++;
                    state_.should_click = false;
                    last_click_time_ = now;
                }
            }
        }

        if (!frame.empty())
            visualizer_.update(frame, state_, phase_, tracker_, polar_img_);

        int key = visualizer_.waitKey(1);
        if (key == 'q' || key == 27) running_ = false;
    }
}

void GameController::updateState(const cv::Mat& frame) {
    auto det = detector_.detect(frame);
    polar_img_ = det.polar_img;

    state_.ball_detected = det.ball_found;
    if (!state_.ball_detected) return;

    if (state_.radius > std::min(capture_.screenWidth(), capture_.screenHeight()) / 3) {
        state_.ball_detected = false;
        return;
    }

    state_.center = det.ball_center;
    state_.radius = det.ball_radius;
    // 视觉检测到的原始针（未对齐时间的，存在滞后）
    state_.pins = det.pins; 

    if (!ball_initialized_) {
        
        tracker_.init(); // 新 Tracker 的初始化不需要参数
        ball_initialized_ = true;
        last_click_time_ = static_cast<double>(cv::getTickCount()) / cv::getTickFrequency() + 0.5;
    }

    float real_frame_time = static_cast<float>(g_frame_capture_time);
    tracker_.update(state_.pins, real_frame_time);

    state_.angular_velocity = tracker_.getRotationSpeed();
}

void GameController::findBestGap() {
    double now = static_cast<double>(cv::getTickCount()) / cv::getTickFrequency();
    float since = static_cast<float>(now - last_click_time_);
    if (since < 0.5) return; // 防抖冷却

    if (state_.pins.empty()) {
        state_.should_click = true;
        return;
    }

    float w = state_.angular_velocity;

    // ==========================================
    // 0. 反转检测与 1 秒停火期
    // ==========================================
    // 如果前后两帧速度符号相反（且都不是0），说明发生了方向反转
    if (g_last_w != 0.0f && w != 0.0f && (w * g_last_w < 0.0f)) {
        g_reversal_time = now;
        printf("[安全机制] 检测到轮盘反转，强制观察停火 1 秒...\n");
    }
    g_last_w = w; // 更新历史速度

    // 如果距离上一次反转还不到 1 秒，直接挂机，不进行任何射击判定
    if (now - g_reversal_time < 1.0) {
        return; 
    }

    // ==========================================
    // 1. 刚体体积防撞区 (修复针头相撞漏洞)
    // ==========================================
    float R = state_.radius;
    float ball_r = R * 0.1f; // 小球的物理碰撞半径
    float actual_pin_dist = state_.pins[0].distance;
    if (actual_pin_dist < R * 1.5f) actual_pin_dist = R * 3.0f;

    float safeAngle = std::asin(4*ball_r / actual_pin_dist);
    state_.safe_angle = safeAngle; // 画线用

    // ==========================================
    // 2. 时间补偿
    // ==========================================
    float image_age = static_cast<float>(now - g_frame_capture_time);
    float total_predict_time = 0.15f + image_age; 

    // ==========================================
    // 3. 终极三维时空排雷
    // ==========================================
    bool path_is_clear = true;
    float min_current_dist = 9999.0f;

    for (const auto& pin : state_.pins) {
        float current_d = angleDiff(pin.angle, static_cast<float>(M_PI));
        if (std::abs(current_d) < min_current_dist) min_current_dist = std::abs(current_d);

        float future_angle = pin.angle + w * total_predict_time;
        float future_d = angleDiff(future_angle, static_cast<float>(M_PI));

        // 铁律 1：现在黄线内有针头，坚决不开火！
        if (std::abs(current_d) < safeAngle) {
            path_is_clear = false;
            break;
        }

        // 铁律 2：未来落点有针头，坚决不开火！(彻底杜绝小球相撞)
        if (std::abs(future_d) < safeAngle) {
            path_is_clear = false;
            break;
        }

        // 铁律 3：子弹飞行期间有针头横切扫过枪口，坚决不开火！
        if (current_d * future_d < 0.0f && std::abs(current_d - future_d) < static_cast<float>(M_PI)) {
            path_is_clear = false;
            break;
        }
    }

    state_.min_dist = min_current_dist;

    // ==========================================
    // 4. 发射指令
    // ==========================================
    if (path_is_clear) {
        state_.should_click = true;
    }
}
void GameController::calculateClickTiming() {}

void GameController::performClick() {}

void GameController::handlePhaseTransition(GamePhase new_phase) {
    if (new_phase != phase_) {
        switch (new_phase) {
            case GamePhase::LEVEL_CLEAR:
                current_level_++;
                printf("\n=== 关卡通过! 进入第 %d 关 ===\n\n", current_level_);
                ball_initialized_ = false;
                tracker_.reset();
                std::this_thread::sleep_for(std::chrono::seconds(1));
                break;
            case GamePhase::GAME_OVER:
                printf("\n=== 游戏结束! 最终到达第 %d 关, 共插入 %d 根针 ===\n\n",
                       current_level_, total_inserted_);
                std::this_thread::sleep_for(std::chrono::seconds(2));
                clicker_.click(click_x_, click_y_);
                ball_initialized_ = false;
                tracker_.reset();
                current_level_ = 1;
                break;
            case GamePhase::PLAYING:
                break;
            default:
                break;
        }
        phase_ = new_phase;
    }
}

float GameController::normalizeAngle(float angle) {
    const float TWO_PI = 2.0f * static_cast<float>(M_PI);
    while (angle < 0) angle += TWO_PI;
    while (angle >= TWO_PI) angle -= TWO_PI;
    return angle;
}

float GameController::angleDiff(float a, float b) {
    float diff = a - b;
    while (diff > static_cast<float>(M_PI)) diff -= 2.0f * static_cast<float>(M_PI);
    while (diff < -static_cast<float>(M_PI)) diff += 2.0f * static_cast<float>(M_PI);
    return diff;
}

}  // namespace coreball
