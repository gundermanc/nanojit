macro(define_nanojit_platform)
    add_definitions(-DFEATURE_NANOJIT -DVMCFG_NANOJIT)
    if (NOT ANDROID)
        if (CMAKE_SIZEOF_VOID_P EQUAL 8)
            add_definitions (-DAVMPLUS_AMD64 -DNANOJIT_64BIT -DVMCFG_SSE2)
            message("64 bit NanoJIT")
        else ()
            add_definitions (-DAVMPLUS_IA32 -DVMCFG_SSE2)
            message("32 bit NanoJIT")
        endif ()

        if (WIN32)
            add_definitions (-DAVMPLUS_WIN32)
            message("Windows NanoJIT")
        else ()
            add_definitions (-DAVMPLUS_UNIX)
            message("Unix NanoJIT")
        endif ()
    else ()
        add_definitions (-DAVMPLUS_ARM -DAVMPLUS_UNIX)
    endif ()
endmacro()