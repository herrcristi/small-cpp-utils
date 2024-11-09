cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
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
# CASE 1 download google test 
#
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

file(GLOB TEST_SOURCES "tests/*.cpp")
add_executable(small-cpp-utils-tests ${TEST_SOURCES})
target_link_libraries(small-cpp-utils-tests GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(small-cpp-utils-tests)


#
# CASE 2 # use it from external folder
# https://stackoverflow.com/questions/9689183/cmake-googletest
#
# include(ExternalProject)

# # variables to help keep track of gtest paths
# set(GTEST_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/gtest")
# set(GTEST_LOCATION "${GTEST_PREFIX}/src/GTestExternal-build")
# set(GTEST_INCLUDES "${GTEST_PREFIX}/src/GTestExternal/googletest/include")
# set(GMOCK_INCLUDES "${GTEST_PREFIX}/src/GTestExternal/googlemock/include")

# # external project download and build (no install for gtest)
# ExternalProject_Add(GTestExternal
#     URL ${CMAKE_CURRENT_SOURCE_DIR}/../googletest
#     PREFIX "${GTEST_PREFIX}"

#     # cmake arguments
#     CMAKE_ARGS -Dgtest_force_shared_crt=ON

#     # Disable install step
#     INSTALL_COMMAND ""

#     # Wrap download, configure and build steps in a script to log output
#     LOG_DOWNLOAD ON
#     LOG_CONFIGURE ON
#     LOG_BUILD ON
#     )


# # variables defining the import location properties for the generated gtest and
# # gtestmain libraries
# if (MSVC)
#     set(GTEST_IMPORTED_LOCATION
#         IMPORTED_LOCATION_DEBUG           "${GTEST_LOCATION}/Debug/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX}"
#         IMPORTED_LOCATION_RELEASE         "${GTEST_LOCATION}/Release/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX}"
#         )
#     set(GTESTMAIN_IMPORTED_LOCATION
#         IMPORTED_LOCATION_DEBUG           "${GTEST_LOCATION}/Debug/${CMAKE_STATIC_LIBRARY_PREFIX}gtest_main${CMAKE_STATIC_LIBRARY_SUFFIX}"
#         IMPORTED_LOCATION_RELEASE         "${GTEST_LOCATION}/Release/${CMAKE_STATIC_LIBRARY_PREFIX}gtest_main${CMAKE_STATIC_LIBRARY_SUFFIX}"
#         )
# else()
#     set(GTEST_IMPORTED_LOCATION
#         IMPORTED_LOCATION                 "${GTEST_LOCATION}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX}")
#     set(GTESTMAIN_IMPORTED_LOCATION
#         IMPORTED_LOCATION                 "${GTEST_LOCATION}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}gtest_main${CMAKE_STATIC_LIBRARY_SUFFIX}")
# endif()

# # the gtest include directory exists only after it is build, but it is used/needed
# # for the set_target_properties call below, so make it to avoid an error
# file(MAKE_DIRECTORY ${GTEST_INCLUDES})

# # define imported library GTest
# add_library(GTest IMPORTED STATIC GLOBAL)
# set_target_properties(GTest PROPERTIES
#     INTERFACE_INCLUDE_DIRECTORIES     "${GTEST_INCLUDES}"
#     IMPORTED_LINK_INTERFACE_LIBRARIES "${CMAKE_THREAD_LIBS_INIT}"
#     ${GTEST_IMPORTED_LOCATION}
#     )

# # define imported library GTestMain
# add_library(GTestMain IMPORTED STATIC GLOBAL)
# set_target_properties(GTestMain PROPERTIES
#     IMPORTED_LINK_INTERFACE_LIBRARIES GTest
#     ${GTESTMAIN_IMPORTED_LOCATION}
#     )

# # make GTest depend on GTestExternal
# add_dependencies(GTest GTestExternal)

# #
# # Test executable
# #
# enable_testing()

# file(GLOB TEST_SOURCES "tests/*.cc")
# include_directories(${GTEST_INCLUDES})
# include_directories(${GMOCK_INCLUDES})

# add_executable(small-cpp-utils-tests ${TEST_SOURCES})
# # set_target_properties(small-cpp-utils-tests PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
# target_link_libraries(small-cpp-utils-tests GTest)
# target_link_libraries(small-cpp-utils-tests GTestMain)

# include(GoogleTest)
# gtest_discover_tests(small-cpp-utils-tests)