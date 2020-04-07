cmake_minimum_required(VERSION 3.14)
project(Tax)

set(CMAKE_CXX_STANDARD 14)

find_package(glog)
find_package(gflags)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME}
        glog
        gflags
        )
