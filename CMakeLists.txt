# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 KScaling contributors

cmake_minimum_required(VERSION 3.16)

project(kscaling LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Test)
find_package(KF6Screen REQUIRED)

add_executable(kscaling src/main.cpp)

target_link_libraries(kscaling
    PRIVATE
        Qt6::Core
        KF6::Screen
)

enable_testing()
include(CTest)

add_executable(ResolutionMathTest
    tests/ResolutionMathTest.cpp
    src/core/ResolutionMath.cpp
)
target_include_directories(ResolutionMathTest PRIVATE src/core)
target_link_libraries(ResolutionMathTest
    PRIVATE
        Qt6::Core
        Qt6::Test
)
add_test(NAME ResolutionMathTest COMMAND ResolutionMathTest)
