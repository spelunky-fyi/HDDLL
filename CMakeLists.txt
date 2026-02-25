cmake_minimum_required(VERSION 3.13)
project(HDDLL)
set(CMAKE_CXX_STANDARD 20)

add_subdirectory(3rdparty)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

option(DEV "Build with development features" OFF)

if(DEV)
    add_compile_definitions(DEV)
endif()

add_library(hddll STATIC
    src/dllmain.cpp
    src/ui.h src/ui.cpp
    src/hooks.h src/hooks.cpp
    src/memory.h src/memory.cpp
    src/hd.h src/hd.cpp
    src/hd_entity.h
    src/entities.h
    src/utils.h src/utils.cpp
    src/hddll.h
)
target_include_directories(hddll PUBLIC src)
target_link_libraries(hddll PUBLIC imgui minhook)
