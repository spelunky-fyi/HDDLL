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
    src/hddll/hddll.h src/hddll.cpp
    src/hddll/ui.h src/ui.cpp
    src/hddll/hooks.h src/hooks.cpp
    src/hddll/memory.h src/memory.cpp
    src/hddll/hd.h src/hd.cpp
    src/hddll/hd_entity.h
    src/hddll/entities.h
    src/hddll/utils.h src/utils.cpp
)
target_include_directories(hddll PUBLIC src)
target_link_libraries(hddll PUBLIC imgui minhook)
