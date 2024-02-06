cmake_minimum_required(VERSION 3.10)
project(RayTraycing)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_BUILD_TYPE Release)

if(CMAKE_GENERATOR MATCHES "^Visual Studio")
        add_compile_options("/source-charset:utf-8")
        set(FY_CPP_INCLUDE_DIR C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.34.31933/include)
        set(CMAKE_CONFIGURATION_TYPES Release)
else()
        set(FY_CPP_INCLUDE_DIR C:/MinGW/lib/gcc/mingw32/9.2.0/include/c++ C:/MinGW/include)
        set(CMAKE_CXX_COMPILER C:/MinGW/bin/g++.exe)
        set(CMAKE_C_COMPILER C:/MinGW/bin/gcc.exe)
endif()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src CPP_FILES)

add_executable(RayTraycing ${CPP_FILES})
