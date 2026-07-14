#!/bin/bash
# 一键启动：检查 Firefox 窗口，没有才重启

GAME_URL="https://www.arealme.com/coreball/cn/"

# 检查 Firefox X11 窗口是否已存在
if ! xprop -root _NET_CLIENT_LIST 2>/dev/null | grep -oP '0x[0-9a-f]+' | while read wid; do
    xprop -id "$wid" WM_CLASS 2>/dev/null
done | grep -qi "firefox"; then
    echo "[启动] 未找到 Firefox X11 窗口，正在启动..."
    killall -9 firefox 2>/dev/null
    killall -9 firefox-esr 2>/dev/null
    sleep 1
    GDK_BACKEND=x11 MOZ_ENABLE_WAYLAND=0 firefox-esr "$GAME_URL" &
    sleep 5
else
    echo "[启动] Firefox 已在运行"
fi

# 运行机器人
cd "$(dirname "$0")/build"
./coreball_bot
