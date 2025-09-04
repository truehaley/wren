#ifndef wren_math_h
#define wren_math_h

#include "wren.h"
#include <math.h>
#include <stdint.h>

#ifdef WREN_FLOAT32
    // 64 bit is the default, but we can use 32 bit single precision floats as an alternative
    #if WREN_NAN_TAGGING
        #error Nan Tagging may not be enabled when using 32 bit floats
    #endif

    // A union to let us reinterpret a float as raw bits and back.
    typedef union
    {
        uint32_t bits32;
        float num;
    } WrenFloatBits;

    #define WREN_FLOAT_QNAN_POS_MIN_BITS (UINT64_C(0x7FC00000))
    #define WREN_FLOAT_QNAN_POS_MAX_BITS (UINT64_C(0x7FFFFFFF))

    #define WREN_NUMBER_NAN (wrenNumberFromBits(WREN_FLOAT_QNAN_POS_MIN_BITS))

    static inline WrenNum wrenNumberFromBits(uint32_t bits)
    {
        WrenFloatBits data;
        data.bits32 = bits;
        return data.num;
    }

    static inline uint32_t wrenNumberToBits(WrenNum num)
    {
        WrenFloatBits data;
        data.num = num;
        return data.bits32;
    }
#else  // Default of 64 bit
    // Default version uses 64 bit doubles as the number representation
    // A union to let us reinterpret a double as raw bits and back.
    typedef union
    {
        uint64_t bits64;
        uint32_t bits32[2];
        double num;
    } WrenDoubleBits;

    #define WREN_DOUBLE_QNAN_POS_MIN_BITS (UINT64_C(0x7FF8000000000000))
    #define WREN_DOUBLE_QNAN_POS_MAX_BITS (UINT64_C(0x7FFFFFFFFFFFFFFF))

    #define WREN_NUMBER_NAN (wrenNumberFromBits(WREN_DOUBLE_QNAN_POS_MIN_BITS))

    static inline double wrenNumberFromBits(uint64_t bits)
    {
        WrenDoubleBits data;
        data.bits64 = bits;
        return data.num;
    }

    static inline uint64_t wrenNumberToBits(double num)
    {
        WrenDoubleBits data;
        data.num = num;
        return data.bits64;
    }
#endif // 32/64 bit
#endif // wren_math_h
