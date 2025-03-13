cmake_minimum_required(VERSION 3.6)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "build type")
set(CMAKE_SKIP_RPATH TRUE)

project(
        workflow
        VERSION 0.11.8
        LANGUAGES C CXX
)

include(GNUInstallDirs)

# 查找 src 目录下所有头文件所在的目录
file(GLOB_RECURSE HEADER_DIRS RELATIVE ${CMAKE_SOURCE_DIR} src/*.h)

# 提取头文件所在的路径，去重
set(ALL_INCLUDE_DIRS "")
foreach (header ${HEADER_DIRS})
    get_filename_component(HEADER_PATH ${header} DIRECTORY)
    list(APPEND ALL_INCLUDE_DIRS ${HEADER_PATH})
endforeach ()
list(REMOVE_DUPLICATES ALL_INCLUDE_DIRS)

# 添加这些目录到 include_directories
include_directories(${ALL_INCLUDE_DIRS})

file(
        GLOB_RECURSE SOURCES
        "src/*.cpp"
        "src/*.c"
        "src/*.h"
)

add_executable(main ${SOURCES}  src/main.cpp
        src/kernel/RBTree.cpp
        src/kernel/RBTree.h
        src/time/Timer.cpp
        src/time/Timer.h
        src/time/TimerId.h
        src/time/TimerQueue.cpp
        src/time/TimerQueue.h
        src/time/Timestamp.cpp
        src/time/Timestamp.h
        src/kernel/Callbacks.h
        src/kernel/Poller.h)

add_subdirectory(tests)