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

add_executable(kscaling
    src/main.cpp
    src/cli/Cli.cpp
    src/core/ApplyService.cpp
    src/core/ResolutionMath.cpp
    src/core/Settings.cpp
    src/backend/LibKScreenBackend.cpp
)

target_include_directories(kscaling PRIVATE src src/core src/backend src/cli)

target_link_libraries(kscaling
    PRIVATE
        Qt6::Core
        KF6::Screen
)

add_library(MockDisplayBackend STATIC
    src/backend/MockDisplayBackend.cpp
)
target_include_directories(MockDisplayBackend PUBLIC src/backend)
target_link_libraries(MockDisplayBackend PUBLIC Qt6::Core)

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

add_executable(SettingsTest
    tests/SettingsTest.cpp
    src/core/Settings.cpp
)
target_include_directories(SettingsTest PRIVATE src/core)
target_link_libraries(SettingsTest
    PRIVATE
        Qt6::Core
        Qt6::Test
)
add_test(NAME SettingsTest COMMAND SettingsTest)

add_executable(MockDisplayBackendTest
    tests/MockDisplayBackendTest.cpp
)
target_link_libraries(MockDisplayBackendTest
    PRIVATE
        MockDisplayBackend
        Qt6::Core
        Qt6::Test
)
add_test(NAME MockDisplayBackendTest COMMAND MockDisplayBackendTest)

add_executable(CliTest
    tests/CliTest.cpp
    src/cli/Cli.cpp
    src/core/ResolutionMath.cpp
)
target_include_directories(CliTest PRIVATE src src/core)
target_link_libraries(CliTest
    PRIVATE
        Qt6::Core
        Qt6::Test
)
add_test(NAME CliTest COMMAND CliTest)

add_executable(ListTest
    tests/ListTest.cpp
    src/cli/Cli.cpp
    src/core/ResolutionMath.cpp
)
target_include_directories(ListTest PRIVATE src src/core)
target_link_libraries(ListTest
    PRIVATE
        MockDisplayBackend
        Qt6::Core
        Qt6::Test
)
add_test(NAME ListTest COMMAND ListTest)

add_executable(ApplyOrchestratorTest
    tests/ApplyOrchestratorTest.cpp
    src/core/ApplyService.cpp
    src/core/ResolutionMath.cpp
    src/core/Settings.cpp
)
target_include_directories(ApplyOrchestratorTest PRIVATE src/core)
target_link_libraries(ApplyOrchestratorTest
    PRIVATE
        MockDisplayBackend
        Qt6::Core
        Qt6::Test
)
add_test(NAME ApplyOrchestratorTest COMMAND ApplyOrchestratorTest)
