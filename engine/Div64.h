#ifndef DIV64_H
#define DIV64_H

#include <stdint.h>

// These implement the compiler builtins for 64-bit integer division

#ifdef __cplusplus
extern "C" {
#endif

int64_t __divdi3(int64_t a, int64_t b);
uint64_t __udivdi3(uint64_t n, uint64_t d);
// You might potentially need __moddi3 and __umoddi3 as well,
// depending on whether the compiler generated code for the % operator.

#ifdef __cplusplus
}
#endif

#endif // DIV64_H
