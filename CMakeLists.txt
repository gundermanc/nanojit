cmake_minimum_required(VERSION 2.8)

project(nanojit)
include_directories(nanojit . lirasm/VMPI.nj hack hack/vprof)

add_definitions(-DFEATURE_NANOJIT -DVMCFG_NANOJIT)

include ("cmake_macros.cmake")
define_nanojit_platform()

if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
    add_definitions( -DDEBUG -D_DEBUG -DNJ_VERBOSE)
endif ()

set(NANOJIT_CXX_FLAGS "${CMAKE_CXX_FLAGS}" CACHE STRING "Nanojit cxx flags")

# Travis CI Compatible C++11 Support
# CXX_STANDARD proeprty doesn't work on Travis CI because
# Travis has horribly out of date CMAKE *grumble grumble*
include (CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG ("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG ("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if (COMPILER_SUPPORTS_CXX11)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif (COMPILER_SUPPORTS_CXX0X)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else ()
    message (STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif ()

add_library(nanojit
    nanojit/Allocator.cpp
    nanojit/Assembler.cpp
    nanojit/CodeAlloc.cpp
    nanojit/Containers.cpp
    nanojit/Fragmento.cpp
    nanojit/LIR.cpp
    nanojit/NativeARM.cpp
    nanojit/NativeMIPS.cpp
    nanojit/NativePPC.cpp
    nanojit/NativeSH4.cpp
#    nanojit/NativeSparc.cpp
    nanojit/NativeThumb2.cpp
    nanojit/NativeX64.cpp
    nanojit/Nativei386.cpp
    nanojit/RegAlloc.cpp
    nanojit/njconfig.cpp
    AVMPI/float4Support.cpp
)

add_library(njutil
    lirasm/VMPI.nj/VMPI.cpp
    lirasm/VMPI.nj/avmplus.cpp
    lirasm/VMPI.nj/common.cpp
)

add_executable(lirasm_cli
    lirasm/lirasm/lirasm.cpp
)
target_link_libraries(lirasm_cli nanojit njutil)

add_executable(demo
    main.cpp
)
target_link_libraries(demo nanojit njutil)
