# 包含FetchContent模块
include(FetchContent)

# 下载并构建Google Test
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1 # 可以更改为最新版本
)

# 避免在Windows上安装Google Test
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# 创建测试可执行文件
add_executable(test_list test_list.cpp)

# 链接库和Google Test
target_link_libraries(test_list
        PRIVATE gtest
        PRIVATE gtest_main
        PRIVATE pthread
)

# 将测试添加到CTest
add_test(
        NAME test_list
        COMMAND test_list
)