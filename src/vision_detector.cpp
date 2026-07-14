#include "vision_detector.h"
#include <cmath>
#include <algorithm>

namespace coreball {

VisionDetector::VisionDetector() {}

float VisionDetector::calcAngle(const cv::Point2f& center, const cv::Point2f& point) {
    float dx = point.x - center.x;
    float dy = point.y - center.y;
    float angle = std::atan2(dx, -dy) * 180.0f / static_cast<float>(M_PI);
    if (angle < 0) angle += 360.0f;
    return angle * static_cast<float>(M_PI) / 180.0f;  // 转回弧度
}

VisionDetector::DetectionResult VisionDetector::detect(const cv::Mat& gray) {
    DetectionResult result;
    if (gray.empty()) return result;

    // 1. 复制图像
    cv::Mat img = gray.clone();

    // 2. 模糊 + 二值化
    cv::GaussianBlur(img, img, cv::Size(5, 5), 2);
    cv::Mat binary;
    cv::threshold(img, binary, 150, 255, cv::THRESH_BINARY);

    // 3. 形态学开运算去噪（回到简单方案）
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(binary, binary, cv::MORPH_OPEN, kernel);

    // 4. 找轮廓 → 外接矩形
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    struct RawRect { cv::Rect bbox; cv::Point2f center; };
    std::vector<RawRect> rects;
    for (auto& cnt : contours) {
        cv::Rect bbox = cv::boundingRect(cnt);
        if (bbox.area() < 100) continue;  // 过滤噪声

        // 过滤非正方形轮廓（宽高比 > 2 的不是球）
        float aspect = static_cast<float>(std::max(bbox.width, bbox.height)) /
                       static_cast<float>(std::min(bbox.width, bbox.height));
        if (aspect > 2.0f) continue;
        cv::Point2f center(bbox.x + bbox.width / 2.0f, bbox.y + bbox.height / 2.0f);
        rects.push_back({bbox, center});
    }

    if (rects.empty()) {
        static int no_rect_dbg = 0;
        if (++no_rect_dbg % 10 == 0)
            printf("[检测] 无轮廓 (contours=%zu)\n", contours.size());
        return result;
    }

    // 5. 找最大矩形 = 中心球（过滤太大或太小的）
    auto max_it = std::max_element(rects.begin(), rects.end(),
        [](const RawRect& a, const RawRect& b) { return a.bbox.area() < b.bbox.area(); });

    float radius = (max_it->bbox.width + max_it->bbox.height) / 4.0f;
    // 球半径应该在 30~120 之间，太大的是游戏结束画面
    if (radius < 30 || radius > 120) return result;

    result.ball_found = true;
    result.ball_center = max_it->center;
    result.ball_radius = radius;

    // 6. 把中心球抠掉（否则极坐标展开后360行全是白色）
    cv::Mat binary_no_ball = binary.clone();
    cv::circle(binary_no_ball, result.ball_center,
               static_cast<int>(result.ball_radius * 1.1f), cv::Scalar(0), -1);

    // 7. 极坐标展开
    int max_radius = static_cast<int>(result.ball_radius * 3.5f);
    cv::warpPolar(binary_no_ball, result.polar_img,
                  cv::Size(max_radius, 360),
                  result.ball_center,
                  max_radius,
                  cv::INTER_LINEAR | cv::WARP_POLAR_LINEAR);
    cv::Mat& polar_img = result.polar_img;

    // 8. 在极坐标图中找针（连通域聚类，合并针的厚度）
    //    OpenCV warpPolar: 0度在3点钟方向，需要+90度对齐12点钟
    struct PinCluster { int start_angle; int end_angle; int max_dist; };
    std::vector<PinCluster> clusters;
    bool in_cluster = false;
    PinCluster current = {0, 0, 0};

    for (int angle = 0; angle < 360; angle++) {
        cv::Mat row = polar_img.row(angle);
        // 找该行最外侧白色像素（针尖距离）
        int pin_dist = 0;
        for (int r = max_radius - 1; r >= 0; r--) {
            if (row.at<uchar>(0, r) > 0) {
                pin_dist = r;
                break;
            }
        }

        // 黑圈内不可能有白色像素，所以pin_dist>0即为针（整行纯黑才断开）
        bool has_pin = (pin_dist > 0);

      if (has_pin) {
    if (!in_cluster) {
        current = {angle, angle, pin_dist};
        in_cluster = true;
    } else {
        // 强制拆分：一根正常的针在极坐标上通常只有 3~6 度的宽度
        // 如果连续白色像素超过 10 度，说明至少有两根针粘连了，强制切断！
        if (angle - current.start_angle > 10) {
            clusters.push_back(current);             // 保存前一根针
            current = {angle, angle, pin_dist};      // 新起一根针
        } else {
            current.end_angle = angle;
            current.max_dist = std::max(current.max_dist, pin_dist);
        }
    }
}else {
            if (in_cluster) {
                clusters.push_back(current);
                in_cluster = false;
            }
        }
    }
    if (in_cluster) clusters.push_back(current);

    // 环绕合并：首尾聚类如果很近，合并为一根针
    if (clusters.size() >= 2) {
        auto& first = clusters.front();
        auto& last = clusters.back();
        int wrap_gap = (360 - last.end_angle) + first.start_angle;
        if (wrap_gap < 15) {  // 15度以内视为同一根针
            first.start_angle = last.start_angle - 360;  // 负角度，后面取中间值时会修正
            first.max_dist = std::max(first.max_dist, last.max_dist);
            clusters.pop_back();
        }
    }

    // 9. 每个聚类 = 一根针，取中间角度
    for (auto& cluster : clusters) {
        int mid_angle = (cluster.start_angle + cluster.end_angle) / 2;
        // +90度修正：OpenCV 0度在3点钟，游戏0度在12点钟
        int game_angle = (mid_angle + 90) % 360;
        float angle_rad = static_cast<float>(game_angle) * static_cast<float>(M_PI) / 180.0f;
        float pin_dist = static_cast<float>(cluster.max_dist);

        float tip_x = result.ball_center.x + std::sin(angle_rad) * pin_dist;
        float tip_y = result.ball_center.y - std::cos(angle_rad) * pin_dist;

        PinInfo pin;
        pin.tip = cv::Point2f(tip_x, tip_y);
        pin.distance = pin_dist;
        pin.angle = angle_rad;
        result.pins.push_back(pin);
    }

    // 按角度排序
    std::sort(result.pins.begin(), result.pins.end(),
              [](const PinInfo& a, const PinInfo& b) { return a.angle < b.angle; });

    // 调试
    static int dbg = 0;
    if (++dbg % 3 == 0) {
        printf("[检测] 球=%s(%.0f) | 针=%zu | 聚类=%zu\n",
               result.ball_found ? "✓" : "✗",
               result.ball_radius,
               result.pins.size(),
               clusters.size());
    }

    return result;
}

GamePhase VisionDetector::detectGamePhase(const cv::Mat& gray) {
    cv::Rect roi(10, 10, 50, 50);
    roi &= cv::Rect(0, 0, gray.cols, gray.rows);
    if (roi.width <= 0 || roi.height <= 0) return GamePhase::UNKNOWN;
    float brightness = static_cast<float>(cv::mean(gray(roi))[0]);
    if (brightness > 150.0f) return GamePhase::WAITING;
    return GamePhase::PLAYING;
}

bool VisionDetector::findStartButton(const cv::Mat& gray, cv::Point& button_pos) {
    cv::Mat bgr, hsv;
    cv::cvtColor(gray, bgr, cv::COLOR_GRAY2BGR);
    cv::cvtColor(bgr, hsv, cv::COLOR_BGR2HSV);
    cv::Mat mask;
    cv::inRange(hsv, cv::Scalar(35, 80, 80), cv::Scalar(85, 255, 255), mask);
    cv::Rect bottom(0, gray.rows / 2, gray.cols, gray.rows / 2);
    cv::Mat roi = mask(bottom);
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(roi, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    float best_area = 0;
    bool found = false;
    for (auto& cnt : contours) {
        float area = cv::contourArea(cnt);
        if (area < 500 || area <= best_area) continue;
        cv::Moments m = cv::moments(cnt);
        if (m.m00 == 0) continue;
        best_area = area;
        button_pos = cv::Point(static_cast<int>(m.m10 / m.m00),
                               static_cast<int>(m.m01 / m.m00 + gray.rows / 2));
        found = true;
    }
    return found;
}

}  // namespace coreball
