#include "auto_clicker.h"
#include <cstdio>

namespace coreball {

AutoClicker::AutoClicker()
    : display_(nullptr), initialized_(false) {}

AutoClicker::~AutoClicker() {
    if (display_) {
        XCloseDisplay(display_);
    }
}

bool AutoClicker::init() {
    display_ = XOpenDisplay(nullptr);
    if (!display_) {
        fprintf(stderr, "[AutoClicker] 无法打开 X11 显示\n");
        return false;
    }
    initialized_ = true;
    return true;
}

bool AutoClicker::click(int x, int y) {
    if (!initialized_) return false;

    // 移动鼠标到目标位置
    XTestFakeMotionEvent(display_, -1, x, y, CurrentTime);
    XFlush(display_);

    // 按下左键
    XTestFakeButtonEvent(display_, 1, True, CurrentTime);
    XFlush(display_);

    // 释放左键
    XTestFakeButtonEvent(display_, 1, False, CurrentTime);
    XFlush(display_);

    return true;
}

bool AutoClicker::moveMouse(int x, int y) {
    if (!initialized_) return false;
    XTestFakeMotionEvent(display_, -1, x, y, CurrentTime);
    XFlush(display_);
    return true;
}

bool AutoClicker::clickAtCurrentPos() {
    if (!initialized_) return false;

    XTestFakeButtonEvent(display_, 1, True, CurrentTime);
    XFlush(display_);
    XTestFakeButtonEvent(display_, 1, False, CurrentTime);
    XFlush(display_);

    return true;
}

}  // namespace coreball
