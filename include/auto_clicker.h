#ifndef AUTO_CLICKER_H
#define AUTO_CLICKER_H

#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>

namespace coreball {

/// X11 自动点击模块，使用 XTest 扩展模拟鼠标操作
class AutoClicker {
public:
    AutoClicker();
    ~AutoClicker();

    /// 初始化 X11 连接
    bool init();

    /// 在指定位置执行一次左键点击
    bool click(int x, int y);

    /// 仅移动鼠标（不点击）
    bool moveMouse(int x, int y);

    /// 仅执行点击（在当前位置）
    bool clickAtCurrentPos();

private:
    Display* display_;
    bool initialized_;
};

}  // namespace coreball

#endif  // AUTO_CLICKER_H
