cmake_minimum_required(VERSION 3.14)
project(Tax)

set(CMAKE_CXX_STANDARD 14)
list(APPEND CMAKE_PREFIX_PATH "d:/program files/mingw32/environment")

find_package(glog)
find_package(gflags)

set(CMAKE_CXX_FLAGS "-static ${CMAKE_CXX_FLAGS}")

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME}
        glog::glog
        gflags
        )
