cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

project(small-cpp-utils)

include_directories(include)


include_directories(./include/)
file(GLOB SOURCES "*.cpp")
add_executable(small-cpp-utils ${SOURCES})

target_link_libraries(small-cpp-utils pthread rt)

# find_package(pthread)
# find_package(rt)
