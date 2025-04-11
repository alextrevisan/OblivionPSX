#ifndef _VECTOR3_HPP_
#define _VECTOR3_HPP_
#include "FixedPoint.h"
#include <psxgte.h>

struct Vector3
{
    Vector3(){}
    Vector3(const Vector3& other)
    {
        data[0] = other.data[0];
        data[1] = other.data[1];
        data[2] = other.data[2];
    }
    Vector3(const FixedPoint &x, const FixedPoint &y, const FixedPoint &z)
    {
        data[0] = x;
        data[1] = y;
        data[2] = z;
    }
    FixedPoint data[3];   
    FixedPoint &x = data[0];
    FixedPoint &y = data[1];
    FixedPoint &z = data[2];
    Vector3 operator=(const Vector3& other)
    {
        data[0] = other.data[0];
        data[1] = other.data[1];
        data[2] = other.data[2];
        return *this;
    }

    FixedPoint& operator[](int index)
    {
        return data[index];
    }

    Vector3 Rotated(Vector3 axis, FixedPoint angle) const
    {
        FixedPoint sinAngle = FixedPoint::Sin(angle);
        FixedPoint cosAngle = FixedPoint::Cos(angle);
        FixedPoint oneMinusCosAngle = FixedPoint::One() - cosAngle;

        FixedPoint ux = axis[0] * x;
        FixedPoint uy = axis[0] * y;
        FixedPoint uz = axis[0] * z;

        FixedPoint vx = axis[1] * x;
        FixedPoint vy = axis[1] * y;
        FixedPoint vz = axis[1] * z;

        FixedPoint wx = axis[2] * x;
        FixedPoint wy = axis[2] * y;
        FixedPoint wz = axis[2] * z;

        FixedPoint newX = axis[0] * (ux + vy + wz) * oneMinusCosAngle + x * cosAngle + (-wy + vz) * sinAngle;
        FixedPoint newY = axis[1] * (ux + vy + wz) * oneMinusCosAngle + y * cosAngle + (wx - uz) * sinAngle;
        FixedPoint newZ = axis[2] * (ux + vy + wz) * oneMinusCosAngle + z * cosAngle + (-vx + uy) * sinAngle;

        return Vector3 { newX, newY, newZ };
    }

    static const Vector3 Up;
    static const Vector3 Left;
    static const Vector3 Forward;
};  

const Vector3 Vector3::Up = { FixedPoint(0), FixedPoint(-1), FixedPoint(0) };
const Vector3 Vector3::Left = { FixedPoint(1), FixedPoint(0), FixedPoint(0) };
const Vector3 Vector3::Forward = { FixedPoint(0), FixedPoint(0), FixedPoint(1) };

#endif //_VECTOR3_HPP_