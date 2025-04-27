#include "Div64.h"

// Basic unsigned 64-bit division
// Required by the toolchain when dividing uint64_t
// Adapted from public domain sources/compiler-rt implementations
uint64_t __udivdi3(uint64_t n, uint64_t d) {
    if (d == 0) return 0; // Or trigger an error/exception
    if (d > n) return 0;
    if (d == n) return 1;

    // Find highest set bit in denominator
    unsigned int shift = 0;
    if ((d >> 32) != 0) {
        shift += 32;
        d >>= 32;
    }
    if ((d >> 16) != 0) {
        shift += 16;
        d >>= 16;
    }
     if ((d >> 8) != 0) {
        shift += 8;
        d >>= 8;
    }
     if ((d >> 4) != 0) {
        shift += 4;
        d >>= 4;
    }
     if ((d >> 2) != 0) {
        shift += 2;
        d >>= 2;
    }
     if ((d >> 1) != 0) {
        shift += 1;
    }
    // Now shift contains the position of the highest bit (0-63)

    uint64_t q = 0;
    uint64_t r = n;
    // Simplified restoring division algorithm
    while (r >= (uint64_t)d << shift) { // Use original d for comparison magnitude
        uint64_t sub = (uint64_t)d << shift;
         if (r >= sub) {
            r -= sub;
            q |= (uint64_t)1 << shift;
        }
        if (shift == 0) break;
        shift--;
    }

    return q;
}

// Basic signed 64-bit division using unsigned division
// Required by the toolchain when dividing int64_t
int64_t __divdi3(int64_t a, int64_t b) {
    int sign = 1;
    uint64_t ua = a;
    uint64_t ub = b;

    if (a < 0) {
        ua = -a;
        sign = -sign;
    }
    if (b < 0) {
        ub = -b;
        sign = -sign;
    }

    uint64_t uq = __udivdi3(ua, ub);

    // Check for overflow potential (INT64_MIN / -1)
    if (sign < 0 && uq > (uint64_t)0x8000000000000000) {
        // Overflow case, standard behavior might vary, often traps or returns INT64_MIN
        // Returning INT64_MIN for simplicity here
        return (int64_t)0x8000000000000000;
    }

    int64_t q = uq;

    if (sign < 0) {
        q = -q;
    }
    return q;
}
