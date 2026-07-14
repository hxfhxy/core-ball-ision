#ifndef SCREEN_CAPTURE_H
#define SCREEN_CAPTURE_H

#include <opencv2/opencv.hpp>
#include <memory>
#include <string>

namespace coreball {

/// X11 窗口截取模块
class ScreenCapture {
public:
    ScreenCapture();
    ~ScreenCapture();

    bool init();
    void setCaptureRegion(int x, int y, int w, int h);
    cv::Mat captureFullScreen();
    cv::Mat captureRegion(int x, int y, int width, int height);
    int screenWidth() const { return screen_width_; }
    int screenHeight() const { return screen_height_; }
    int roiX() const { return roi_x_; }
    int roiY() const { return roi_y_; }

private:
    int screen_width_;
    int screen_height_;
    int roi_x_ = 0;
    int roi_y_ = 0;
    bool initialized_;

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace coreball

#endif  // SCREEN_CAPTURE_H
