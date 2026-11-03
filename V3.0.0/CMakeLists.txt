cmake_minimum_required(VERSION 3.21)

project(tdlas_hydrogen_ui_suite VERSION 3.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

option(TDLAS_BUILD_STARTUP "Build the startup/login template" ON)
option(TDLAS_BUILD_ADMIN "Build the administrator template" ON)
option(TDLAS_BUILD_CLIENT "Build the client template" ON)
option(TDLAS_BUILD_INTEGRATED "Build the integrated login and role application" ON)

include(CTest)

function(tdlas_configure_ui_target target_name)
    target_link_libraries(${target_name}
        PRIVATE
            Qt${QT_VERSION_MAJOR}::Widgets
    )

    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

if(TDLAS_BUILD_STARTUP)
    add_subdirectory(apps/startup)
endif()

if(TDLAS_BUILD_ADMIN)
    add_subdirectory(apps/admin)
endif()

if(TDLAS_BUILD_CLIENT)
    add_subdirectory(apps/client)
endif()

if(TDLAS_BUILD_INTEGRATED)
    add_subdirectory(apps/integrated)
endif()
