#include "screen_capture.h"

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm>

namespace coreball {

namespace {

int ignoreXError(Display*, XErrorEvent*) { return 0; }

bool readCardinalProperty(Display* dpy, Window win, Atom prop, unsigned long& value) {
    Atom actual_type = None;
    int actual_fmt = 0;
    unsigned long count = 0, bytes_after = 0;
    unsigned char* data = nullptr;
    const int status = XGetWindowProperty(dpy, win, prop, 0, 1, False,
        XA_CARDINAL, &actual_type, &actual_fmt, &count, &bytes_after, &data);
    if (status != Success || actual_type != XA_CARDINAL || actual_fmt != 32 ||
        count != 1 || data == nullptr) {
        if (data) XFree(data);
        return false;
    }
    value = *reinterpret_cast<const unsigned long*>(data);
    XFree(data);
    return true;
}

bool matchesWindowClass(const char* window_class, const std::string& expected) {
    if (window_class == nullptr || expected.empty()) return false;
    std::string actual(window_class);
    std::string exp = expected;
    std::transform(actual.begin(), actual.end(), actual.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    std::transform(exp.begin(), exp.end(), exp.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return actual.find(exp) != std::string::npos;
}

} // anonymous namespace

// ── 内部实现 ────────────────────────────────────────────────────

struct ScreenCapture::Impl {
    Display* display = nullptr;
    Window root = 0;
    Window target_window = 0;
    std::string wm_class = "firefox";
    int screen_width = 0;
    int screen_height = 0;

    ~Impl() { close(); }

    bool open() {
        if (display != nullptr) return true;

        static const bool inited = [] {
            XInitThreads();
            XSetErrorHandler(ignoreXError);
            return true;
        }();
        (void)inited;

        display = XOpenDisplay(nullptr);
        if (display == nullptr) return false;

        root = RootWindow(display, DefaultScreen(display));
        screen_width = DisplayWidth(display, DefaultScreen(display));
        screen_height = DisplayHeight(display, DefaultScreen(display));
        return true;
    }

    void close() {
        if (display) {
            XCloseDisplay(display);
            display = nullptr;
        }
        root = 0;
        target_window = 0;
    }

    Window findTargetWindow() {
        const Atom client_list = XInternAtom(display, "_NET_CLIENT_LIST", True);
        if (client_list == None) return 0;

        const Atom cur_desktop_prop = XInternAtom(display, "_NET_CURRENT_DESKTOP", True);
        const Atom win_desktop_prop = XInternAtom(display, "_NET_WM_DESKTOP", True);
        unsigned long cur_desktop = 0;
        const bool has_desktop = cur_desktop_prop != None && win_desktop_prop != None &&
            readCardinalProperty(display, root, cur_desktop_prop, cur_desktop);

        Atom actual_type = None;
        int actual_fmt = 0;
        unsigned long count = 0, bytes_after = 0;
        unsigned char* data = nullptr;
        const int status = XGetWindowProperty(display, root, client_list, 0, 1024,
            False, XA_WINDOW, &actual_type, &actual_fmt, &count, &bytes_after, &data);
        if (status != Success || actual_type != XA_WINDOW || actual_fmt != 32 || data == nullptr) {
            if (data) XFree(data);
            return 0;
        }

        const auto* windows = reinterpret_cast<const Window*>(data);
        Window result = 0;

        for (unsigned long i = 0; i < count; ++i) {
            XClassHint hint{};
            if (XGetClassHint(display, windows[i], &hint) == 0) continue;
            const bool matches = (hint.res_class && matchesWindowClass(hint.res_class, wm_class));
            if (hint.res_name) XFree(hint.res_name);
            if (hint.res_class) XFree(hint.res_class);
            if (!matches) continue;

            unsigned long win_desktop = 0;
            if (has_desktop &&
                readCardinalProperty(display, windows[i], win_desktop_prop, win_desktop) &&
                win_desktop != cur_desktop && win_desktop != 0xFFFFFFFFUL)
                continue;

            XWindowAttributes attrs{};
            if (XGetWindowAttributes(display, windows[i], &attrs) != 0 &&
                attrs.map_state == IsViewable) {
                result = windows[i];
                break;
            }
        }

        XFree(data);
        return result;
    }

    // ROI 区域（0 表示截取整个窗口）
    int roi_x = 0, roi_y = 0, roi_w = 0, roi_h = 0;

    cv::Mat capture() {
        if (display == nullptr) return cv::Mat();

        if (target_window == 0) {
            target_window = findTargetWindow();
        }
        if (target_window == 0) return cv::Mat();

        XWindowAttributes attrs{};
        if (XGetWindowAttributes(display, target_window, &attrs) == 0 ||
            attrs.map_state != IsViewable) {
            target_window = 0;
            return cv::Mat();
        }

        int w = attrs.width;
        int h = attrs.height;
        int cx = 0, cy = 0;

        // 使用 ROI 区域
        if (roi_w > 0 && roi_h > 0) {
            cx = roi_x;
            cy = roi_y;
            w = roi_w;
            h = roi_h;
        }
        if (w <= 0 || h <= 0) return cv::Mat();

        XImage* ximage = XGetImage(display, target_window, cx, cy,
            static_cast<unsigned int>(w), static_cast<unsigned int>(h),
            AllPlanes, ZPixmap);
        if (ximage == nullptr) {
            target_window = 0;
            return cv::Mat();
        }

        // 直接包成 cv::Mat(BGRA)，用 OpenCV 转灰度
        cv::Mat bgra(h, w, CV_8UC4, ximage->data, ximage->bytes_per_line);
        cv::Mat gray;
        cv::cvtColor(bgra, gray, cv::COLOR_BGRA2GRAY);
        XDestroyImage(ximage);

        screen_width = w;
        screen_height = h;
        return gray;
    }
};

// ── 公开接口 ────────────────────────────────────────────────────

ScreenCapture::ScreenCapture()
    : screen_width_(0), screen_height_(0), initialized_(false), impl_(new Impl()) {}

ScreenCapture::~ScreenCapture() = default;

bool ScreenCapture::init() {
    if (!impl_->open()) {
        fprintf(stderr, "[ScreenCapture] 初始化失败\n");
        return false;
    }

    cv::Mat test = impl_->capture();
    if (test.empty()) {
        printf("[ScreenCapture] 未找到目标窗口，将在运行时重试\n");
    } else {
        screen_width_ = test.cols;
        screen_height_ = test.rows;
    }

    initialized_ = true;
    printf("[ScreenCapture] 初始化成功: %dx%d\n", screen_width_, screen_height_);
    return true;
}

void ScreenCapture::setCaptureRegion(int x, int y, int w, int h) {
    impl_->roi_x = x;
    impl_->roi_y = y;
    impl_->roi_w = w;
    impl_->roi_h = h;
    roi_x_ = x;
    roi_y_ = y;
}

cv::Mat ScreenCapture::captureFullScreen() {
    if (!initialized_) return cv::Mat();
    cv::Mat frame = impl_->capture();
    if (!frame.empty()) {
        screen_width_ = frame.cols;
        screen_height_ = frame.rows;
    }
    return frame;
}

cv::Mat ScreenCapture::captureRegion(int x, int y, int width, int height) {
    cv::Mat full = captureFullScreen();
    if (full.empty()) return cv::Mat();
    x = std::max(0, x); y = std::max(0, y);
    width = std::min(width, full.cols - x);
    height = std::min(height, full.rows - y);
    return full(cv::Rect(x, y, width, height)).clone();
}

}  // namespace coreball
