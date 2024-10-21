cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

project(small-cpp-utils)

#
# default executable with examples
#
include_directories(include)

file(GLOB SOURCES "*.cpp")
add_executable(small-cpp-utils ${SOURCES})
target_link_libraries(small-cpp-utils pthread rt)


# find_package(pthread)
# find_package(rt)


#
# test executable
#
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/5376968f6948923e2411081fd9372e71a59d8e77.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

file(GLOB TEST_SOURCES "tests/*.cc")
add_executable(small-cpp-utils-tests ${TEST_SOURCES})
target_link_libraries(small-cpp-utils-tests GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(small-cpp-utils-tests)


