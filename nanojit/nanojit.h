/* -*- Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*- */
/* vi: set ts=4 sw=4 expandtab: (add to ~/.vimrc: set modeline modelines=5) */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __nanojit_h__
#define __nanojit_h__

#include "avmplus.h"
#include "njcpudetect.h"

#ifdef FEATURE_NANOJIT

#if defined AVMPLUS_IA32
    #define NANOJIT_IA32
#elif defined AVMPLUS_ARM
    #if defined(TARGET_THUMB2) || defined(UNDER_RT)
         #define NANOJIT_THUMB2
    #else
         #define NANOJIT_ARM
    #endif
#elif defined AVMPLUS_PPC
    #define NANOJIT_PPC
#elif defined AVMPLUS_SPARC
    #define NANOJIT_SPARC
#elif defined AVMPLUS_AMD64
    #define NANOJIT_X64
#elif defined VMCFG_SH4
    #define NANOJIT_SH4
#elif defined AVMPLUS_MIPS
    #define NANOJIT_MIPS
#else
    #error "unknown nanojit architecture"
#endif

#ifdef AVMPLUS_64BIT
#define NANOJIT_64BIT
#endif

#if defined NANOJIT_64BIT
    #define IF_64BIT(...) __VA_ARGS__
    #define UNLESS_64BIT(...)
    #define CASE32(x)
    #define CASE64(x)   case x
#else
    #define IF_64BIT(...)
    #define UNLESS_64BIT(...) __VA_ARGS__
    #define CASE32(x)   case x
    #define CASE64(x)
#endif

#if defined NANOJIT_IA32 || defined NANOJIT_X64
    #define CASE86(x)   case x
#else
    #define CASE86(x)
#endif

// Embed no-op macros that let Valgrind work with the JIT.
#ifdef MOZ_VALGRIND
#  define JS_VALGRIND
#endif
#ifdef JS_VALGRIND
#  include <valgrind/valgrind.h>
#elif !defined(VALGRIND_DISCARD_TRANSLATIONS)
#  define VALGRIND_DISCARD_TRANSLATIONS(addr, szB)
#endif

namespace nanojit
{
    /**
     * -------------------------------------------
     * START AVM bridging definitions
     * -------------------------------------------
     */
    const uint32_t MAXARGS = 8;

    #if defined(_DEBUG)
        #define __NanoAssertMsgf(a, file_, line_, f, ...)  \
            if (!(a)) { \
                printf("NanoJIT Assertion Failure: " f "%s (%s:%d)\n", __VA_ARGS__, #a, file_, line_); \
                abort(); \
            }

        #define _NanoAssertMsgf(a, file_, line_, f, ...)   __NanoAssertMsgf(a, file_, line_, f, __VA_ARGS__)

        #define NanoAssertMsgf(a,f,...)   do { __NanoAssertMsgf(a, __FILE__, __LINE__, f ": ", __VA_ARGS__); } while (0)
        #define NanoAssertMsg(a,m)        do { __NanoAssertMsgf(a, __FILE__, __LINE__, "\"%s\": ", m); } while (0)
        #define NanoAssert(a)             do { __NanoAssertMsgf(a, __FILE__, __LINE__, "%s", ""); } while (0)
    #else
        #define NanoAssertMsgf(a,f,...)   do { } while (0) /* no semi */
        #define NanoAssertMsg(a,m)        do { } while (0) /* no semi */
        #define NanoAssert(a)             do { } while (0) /* no semi */
    #endif

    /*
     * Sun Studio C++ compiler has a bug
     * "sizeof expression not accepted as size of array parameter"
     * The bug number is 6688515. It is not public yet.
     * Turn off this assert for Sun Studio until this bug is fixed.
     */
    #ifdef __SUNPRO_CC
        #define NanoStaticAssert(condition)
    #else
        #define NanoStaticAssert(condition) \
            extern void nano_static_assert(int arg[(condition) ? 1 : -1])
    #endif


    /**
     * -------------------------------------------
     * END AVM bridging definitions
     * -------------------------------------------
     */
}

#ifdef AVMPLUS_VERBOSE
    #define NJ_VERBOSE 1
#endif

#if defined(NJ_VERBOSE)
    #include <stdio.h>
    #define verbose_outputf            if (_logc->lcbits & LC_Native) \
                                        Assembler::outputf
    #define verbose_only(...)        __VA_ARGS__
#else
    #define verbose_outputf
    #define verbose_only(...)
#endif /*NJ_VERBOSE*/

#ifdef _DEBUG
    #define debug_only(x)           x
#else
    #define debug_only(x)
#endif /* DEBUG */

#define isS8(i)  ( int32_t(i) == int8_t(i) )
#define isU8(i)  ( int32_t(i) == uint8_t(i) )
//### FIXME: Revisit these for types of argument and sign-correctness.
//### Should make them inlines.
#define isU12(i) ( ((i) & 0xfff) == (i) )
#define isS16(i) ( int32_t(i) == int16_t(i) )
#define isU16(i) ( int32_t(i) == uint16_t(i) )
#define isS20(i) ( (int32_t((i)<<12)>>12) == (i) )
#define isS21(i) ( (int32_t((i)<<11)>>11) == (i) )
#define isS24(i) ( (int32_t((i)<<8)>>8) == (i) )
#define isS25(i) ( (int32_t((i)<<7)>>7) == (i) )

static inline bool isS32(intptr_t i) {
    return int32_t(i) == i;
}

static inline bool isU32(uintptr_t i) {
    return uint32_t(i) == i;
}

#define alignTo(x,s)        ((((uintptr_t)(x)))&~(((uintptr_t)s)-1))
#define alignUp(x,s)        ((((uintptr_t)(x))+(((uintptr_t)s)-1))&~(((uintptr_t)s)-1))

#define NJ_MIN(x, y) ((x) < (y) ? (x) : (y))
#define NJ_MAX(x, y) ((x) > (y) ? (x) : (y))

namespace nanojit
{
// Define msbSet32(), lsbSet32(), msbSet64(), and lsbSet64() functions using
// fast find-first-bit instructions intrinsics when available.
// The fall-back implementations use iteration.
#if defined(_WIN32) && (_MSC_VER >= 1300) && (defined(_M_IX86) || defined(_M_AMD64) || defined(_M_X64))

    extern "C" unsigned char _BitScanForward(unsigned long * Index, unsigned long Mask);
    extern "C" unsigned char _BitScanReverse(unsigned long * Index, unsigned long Mask);
    # pragma intrinsic(_BitScanForward)
    # pragma intrinsic(_BitScanReverse)

    // Returns the index of the most significant bit that is set.
    static inline unsigned msbSet32(uint32_t x) {
        unsigned long idx;
        _BitScanReverse(&idx, (unsigned long)(x | 1)); // the '| 1' ensures a 0 result when x==0
        return idx;
    }

    // Returns the index of the least significant bit that is set.
    static inline unsigned lsbSet32(uint32_t x) {
        unsigned long idx;
        _BitScanForward(&idx, (unsigned long)(x | 0x80000000)); // the '| 0x80000000' ensures a 0 result when x==0
        return idx;
    }

#if defined(_M_AMD64) || defined(_M_X64)
    extern "C" unsigned char _BitScanForward64(unsigned long * Index, unsigned __int64 Mask);
    extern "C" unsigned char _BitScanReverse64(unsigned long * Index, unsigned __int64 Mask);
    # pragma intrinsic(_BitScanForward64)
    # pragma intrinsic(_BitScanReverse64)

    // Returns the index of the most significant bit that is set.
    static inline unsigned msbSet64(uint64_t x) {
        unsigned long idx;
        _BitScanReverse64(&idx, (unsigned __int64)(x | 1)); // the '| 1' ensures a 0 result when x==0
        return idx;
    }

    // Returns the index of the least significant bit that is set.
    static inline unsigned lsbSet64(uint64_t x) {
        unsigned long idx;
        _BitScanForward64(&idx, (unsigned __int64)(x | 0x8000000000000000LL)); // the '| 0x80000000' ensures a 0 result when x==0
        return idx;
    }
#else
    // Returns the index of the most significant bit that is set.
    static unsigned msbSet64(uint64_t x) {
        return (x & 0xffffffff00000000LL) ? msbSet32(uint32_t(x >> 32)) + 32 : msbSet32(uint32_t(x));
    }
    // Returns the index of the least significant bit that is set.
    static unsigned lsbSet64(uint64_t x) {
        return (x & 0x00000000ffffffffLL) ? lsbSet32(uint32_t(x)) : lsbSet32(uint32_t(x >> 32)) + 32;
    }
#endif

#elif (__GNUC__ >= 4) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)

    // Returns the index of the most significant bit that is set.
    static inline unsigned msbSet32(uint32_t x) {
        return 31 - __builtin_clz(x | 1);
    }

    // Returns the index of the least significant bit that is set.
    static inline unsigned lsbSet32(uint32_t x) {
        return __builtin_ctz(x | 0x80000000);
    }

    // Returns the index of the most significant bit that is set.
    static inline unsigned msbSet64(uint64_t x) {
        return 63 - __builtin_clzll(x | 1);
    }

    // Returns the index of the least significant bit that is set.
    static inline unsigned lsbSet64(uint64_t x) {
        return __builtin_ctzll(x | 0x8000000000000000LL);
    }

#else

    // Slow fall-back: return most significant bit set by searching iteratively.
    static unsigned msbSet32(uint32_t x) {
        for (int i = 31; i >= 0; i--)
            if ((1 << i) & x)
                return i;
        return 0;
    }

    // Slow fall-back: return least significant bit set by searching iteratively.
    static unsigned lsbSet32(uint32_t x) {
        for (int i = 0; i < 32; i++)
            if ((1 << i) & x)
                return i;
        return 31;
    }

    // Slow fall-back: return most significant bit set by searching iteratively.
    static unsigned msbSet64(uint64_t x) {
        for (int i = 63; i >= 0; i--)
            if ((1LL << i) & x)
                return i;
        return 0;
    }

    // Slow fall-back: return least significant bit set by searching iteratively.
    static unsigned lsbSet64(uint64_t x) {
        for (int i = 0; i < 64; i++)
            if ((1LL << i) & x)
                return i;
        return 63;
    }

#endif // select compiler
} // namespace nanojit

// -------------------------------------------------------------------
// START debug-logging definitions
// -------------------------------------------------------------------

/* Debug printing stuff.  All Nanojit and jstracer debug printing
   should be routed through LogControl::printf.  Don't use
   ad-hoc calls to printf, fprintf(stderr, ...) etc.

   Similarly, don't use ad-hoc getenvs etc to decide whether or not to
   print debug output.  Instead consult the relevant control bit in
   LogControl::lcbits in the LogControl object you are supplied with.
*/

# if defined(__GNUC__)
# define PRINTF_CHECK(x, y) __attribute__((format(__printf__, x, y)))
# else
# define PRINTF_CHECK(x, y)
# endif

namespace nanojit {

    // LogControl, a class for controlling and routing debug output

    enum LC_Bits {
        /* Output control bits for Nanojit code.  Only use bits 15
           and below, so that callers can use bits 16 and above for
           themselves. */
        // TODO: add entries for the writer pipeline
        LC_FragProfile      = 1<<8, // collect per-frag usage counts
        LC_Liveness         = 1<<7, // show LIR liveness analysis
        LC_ReadLIR          = 1<<6, // as read from LirBuffer
        LC_AfterSF          = 1<<5, // after StackFilter
        LC_AfterDCE         = 1<<4, // after dead code elimination
        LC_Bytes            = 1<<3, // byte values of native instruction
        LC_Native           = 1<<2, // final native code
        LC_RegAlloc         = 1<<1, // stuff to do with reg alloc
        LC_Activation       = 1<<0  // enable printActivationState
    };

    class LogControl
    {
    public:
        // All Nanojit and jstracer printing should be routed through
        // this function.
        virtual ~LogControl() {}
        #ifdef NJ_VERBOSE
        virtual void printf( const char* format, ... ) PRINTF_CHECK(2,3);
        #endif

        // An OR of LC_Bits values, indicating what should be output
        uint32_t lcbits;
    };
}

// -------------------------------------------------------------------
// END debug-logging definitions
// -------------------------------------------------------------------


#include "njconfig.h"
#include "Allocator.h"
#include "Containers.h"
#include "Native.h"
#include "CodeAlloc.h"
#include "LIR.h"
#include "RegAlloc.h"
#include "Fragmento.h"
#include "Assembler.h"

#endif // FEATURE_NANOJIT
#endif // __nanojit_h__
