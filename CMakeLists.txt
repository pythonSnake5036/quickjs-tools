cmake_minimum_required(VERSION 3.31)
project(quickjs_tools)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(QJS_BUILD_LIBC ON)

add_subdirectory(extern)

add_executable(quickjs_interrupt_explorer interrupt_explorer.cpp)
target_link_libraries(quickjs_interrupt_explorer PRIVATE qjs)
