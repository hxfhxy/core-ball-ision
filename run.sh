#!/bin/bash
# 一键编译+运行
cd "$(dirname "$0")/build"
make -j$(nproc) && ./coreball_bot
