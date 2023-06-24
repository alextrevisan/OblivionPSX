#ifndef _OPERATORS_H_
#define _OPERATORS_H_

#include <psxgte.h>

const VECTOR operator+(const VECTOR& left, const VECTOR& right)
{
    return {left.vx + right.vx, left.vy + right.vy, left.vz + right.vz};
}

const SVECTOR operator+(const SVECTOR& left, const SVECTOR& right)
{
    return {left.vx + right.vx, left.vy + right.vy, left.vz + right.vz};
}

const VECTOR operator+(const VECTOR& first, const SVECTOR& second)
{
    return {first.vx + second.vx, first.vy + second.vy, first.vz + second.vz};
}

const VECTOR operator+(const SVECTOR& first, const VECTOR& second)
{
    return {first.vx + second.vx, first.vy + second.vy, first.vz + second.vz};
}

#endif //_OPERATORS_H_