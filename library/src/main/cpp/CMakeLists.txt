# 设置CMake最小版本
cmake_minimum_required(VERSION 3.4.1)

# 设置项目名称
project(leakguard)

# 设置rapidjson库路径
set(RAPIDJSON_INCLUDE_DIRS ../../../../rapidjson/include)

# 添加rapidjson头文件目录
include_directories(${RAPIDJSON_INCLUDE_DIRS})

# 获取libboundscheck源文件
file(GLOB LIB_BOUNDS_CHECK_SOURCES "../../../../libboundscheck/src/*.c")

# 添加libboundscheck源文件到编译中
include_directories(../../../../libboundscheck/include)

# 添加头文件目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

find_library(
    # Sets the name of the path variable.
    libnapi-lib
    # Specifies the name of the NDK library that
    # you want CMake to locate.
    ace_napi.z
)

# 获取所有源文件
aux_source_directory(. SRC_LIST)

# 排除napi_init.cpp，因为它是模块入口
list(REMOVE_ITEM SRC_LIST "./napi_init.cpp")

# 编译共享库，包含libboundscheck的源文件
add_library(${PROJECT_NAME} SHARED
       ${SRC_LIST}
       ${LIB_BOUNDS_CHECK_SOURCES}
       napi_init.cpp
)

# 链接库文件
target_link_libraries(${PROJECT_NAME} PUBLIC ${libnapi-lib})