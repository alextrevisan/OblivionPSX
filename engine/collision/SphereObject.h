#ifndef _SPHERE_OBJECT_HPP_
#define _SPHERE_OBJECT_HPP_

#include "FixedPoint.h"
#include "Vector3.h"
namespace PhysicsEnginePSX {
struct SphereObject {
    const FixedPoint Radius;
    Vector3 Position;
    Vector3 Velocity = { FixedPoint(0), FixedPoint(0), FixedPoint(0) };
    Vector3 Acceleration = { FixedPoint(0), FixedPoint(0), FixedPoint(0) };
    FixedPoint Mass = FixedPoint(80);

    SphereObject(const Vector3& position, FixedPoint radius)
        : Position(position), Radius(radius) {}
};
}
#endif //_SPHERE_OBJECT_HPP_