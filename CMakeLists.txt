cmake_minimum_required(VERSION 3.11...4.1)

project(RingBuffer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

add_subdirectory(lib)

include(FetchContent)

include(cmake/googletest.cmake)
add_subdirectory(test)

include(cmake/doxygen.cmake)
