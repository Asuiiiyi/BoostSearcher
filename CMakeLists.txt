cmake_minimum_required(VERSION 3.10)  # 指定 CMake 的最低版本

# 设置项目名称和版本
project(MyProject VERSION 1.0)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 添加头文件目录
include_directories(
    ${CMAKE_SOURCE_DIR}/cppjieba/include/cppjieba
    ${CMAKE_SOURCE_DIR}/include
)

# 为 parser 添加源文件
set(PARSER_SOURCES
    ${CMAKE_SOURCE_DIR}/src/parser.cc
)

# 为 httpserver 添加源文件
set(HTTPSERVER_SOURCES
    ${CMAKE_SOURCE_DIR}/src/http_server.cc
)

# 定义 parser 库文件
set(LIBS_PARSER
    -lboost_system
    -lboost_filesystem
)

# 定义 httpserver 库文件
set(LIBS_HTTPSERVER
    -ljsoncpp 
    -lhiredis
    ${CMAKE_SOURCE_DIR}/redis-plus-plus/build/libredis++.a 
    -lpthread
)

# 设置可执行文件输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# 添加可执行文件
add_executable(Parser ${PARSER_SOURCES})
add_executable(HttpServer ${HTTPSERVER_SOURCES})

# 链接库文件
target_link_libraries(Parser ${LIBS_PARSER})
target_link_libraries(HttpServer ${LIBS_HTTPSERVER})