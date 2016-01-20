cmake_minimum_required(VERSION 2.8)

project(nanojit)
include_directories(nanojit . lirasm/VMPI.nj hack hack/vprof)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFEATURE_NANOJIT -DVMCFG_NANOJIT")

include ("cmake_macros.cmake")
define_nanojit_platform()

if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG -D_DEBUG -DNJ_VERBOSE")
endif ()

set(NANOJIT_CXX_FLAGS "${CMAKE_CXX_FLAGS}" CACHE STRING "Nanojit cxx flags")

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

add_executable(lirasm
    lirasm/lirasm/lirasm.cpp
)
target_link_libraries(lirasm nanojit njutil)

add_executable(demo
    main.cpp
)
target_link_libraries(demo nanojit njutil)
