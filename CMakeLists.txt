cmake_minimum_required(VERSION 3.19)

if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "Set default build type to Release")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build Type" FORCE)
endif()

project(Descale LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_DIR ${SOURCE_DIR}/cmake)
set(DEPENDENCY_DIR ${CMAKE_DIR}/dependency)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    message(STATUS "Set default install path ${SOURCE_DIR}/install")
    set_property(CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE "${SOURCE_DIR}/install")
endif()

include(${DEPENDENCY_DIR}/ac.cmake)
include(${DEPENDENCY_DIR}/cli11.cmake)
include(${DEPENDENCY_DIR}/hashpp.cmake)

add_executable(descale 
    ${SOURCE_DIR}/src/descale.c
    ${SOURCE_DIR}/src/descale_avx2.c
    ${SOURCE_DIR}/src/Main.cpp
    ${SOURCE_DIR}/src/Descale.cpp
    ${SOURCE_DIR}/src/Options.cpp
)

target_include_directories(descale PRIVATE
    $<BUILD_INTERFACE:${SOURCE_DIR}/include>
)

target_link_libraries(descale PRIVATE dep::hashpp dep::cli11 dep::ac AC::Util::Parallel AC::Util::Misc)

target_compile_definitions(descale PRIVATE
    $<$<BOOL:${AC_CORE_WITH_FMA}>:DESCALE_X86>
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_SIMULATE_ID MATCHES "MSVC" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT MATCHES "MSVC")
    if(AC_CORE_WITH_AVX)
        set_source_files_properties(${SOURCE_DIR}/src/descale_avx2.c PROPERTIES COMPILE_OPTIONS "$<IF:$<BOOL:${AC_CORE_WITH_FMA}>,/arch:AVX2,/arch:AVX>")
    endif()
elseif(NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    if(AC_CORE_WITH_AVX)
        set_source_files_properties(${SOURCE_DIR}/src/descale_avx2.c PROPERTIES COMPILE_OPTIONS "-mavx;$<$<BOOL:${AC_CORE_WITH_FMA}>:-mfma>")
    endif()
endif()
