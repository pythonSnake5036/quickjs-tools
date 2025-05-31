cmake_minimum_required(VERSION 3.25)
project(quickjs_tools)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(QJS_BUILD_LIBC ON)

add_subdirectory(extern)

find_package(Boost
    CONFIG
    1.83.0
    REQUIRED
    COMPONENTS program_options
)

add_executable(quickjs_interrupt_explorer src/interrupt_explorer.cpp src/utilities.cpp)
target_link_libraries(quickjs_interrupt_explorer PRIVATE qjs Boost::program_options)
