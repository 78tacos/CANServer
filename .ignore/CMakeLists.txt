cmake_minimum_required(VERSION 3.10)
project(project-name)

enable_testing()

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_executable(server_test tests/server_test.cpp)
target_link_libraries(server_test ${GTEST_LIBRARIES} pthread)

add_test(NAME server_test COMMAND server_test)