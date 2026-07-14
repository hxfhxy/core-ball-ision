cmake_minimum_required(VERSION 3.10)
project(CoreBallAutomation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find OpenCV
find_package(OpenCV 4 REQUIRED COMPONENTS
    core imgproc highgui video videoio imgcodecs)

# Find X11
find_package(X11 REQUIRED)

# Source files
set(SOURCES
    src/main.cpp
    src/screen_capture.cpp
    src/vision_detector.cpp
    src/rotation_tracker.cpp
    src/game_controller.cpp
    src/auto_clicker.cpp
    src/visualizer.cpp
)

add_executable(coreball_bot ${SOURCES})

target_include_directories(coreball_bot PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${X11_INCLUDE_DIR}
    include
    src
)

target_link_libraries(coreball_bot
    ${OpenCV_LIBS}
    ${X11_LIBRARIES}
    ${X11_Xext_LIB}
    ${X11_Xtst_LIB}
)

target_compile_options(coreball_bot PRIVATE -Wall -Wextra -O2)
