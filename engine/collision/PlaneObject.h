#ifndef _PLANE_OBJECT_HPP_
#define _PLANE_OBJECT_HPP_
#include "FixedPoint.h"
#include "Vector3.h"
#include "MathUtils.h"

namespace PhysicsEnginePSX {
struct PlaneObject {
    Vector3 Normal;
    Vector3 Rotation;
    FixedPoint Distance;
    Vector3 Position;
    FixedPoint SizeX;
    FixedPoint SizeZ;

    FixedPoint MinX;
    FixedPoint MaxX;
    FixedPoint MinZ;
    FixedPoint MaxZ;

    PlaneObject(const Vector3& rotation, 
                FixedPoint planeX, FixedPoint planeY, FixedPoint planeZ,
                FixedPoint sizeX, FixedPoint sizeZ)
        : Rotation(rotation),
          Position({planeX, planeY, planeZ}),
          SizeX(sizeX),
          SizeZ(sizeZ) {
        auto normal = MathUtils::RotateNormal(Vector3{0, -1, 0}, rotation.x, rotation.y, rotation.z);
        Normal = normal;
        
        Distance = (MathUtils::DotProduct(normal, {planeX, planeY, planeZ}));
        auto minX = (Position[0] - (SizeX >> 1));
        auto minZ = (Position[2] - (SizeZ >> 1));
        auto minY = (Position[1]);
        auto maxY = (Position[1]);
        auto maxX = (Position[0] + (SizeX >> 1));
        auto maxZ = (Position[2] + (SizeZ >> 1));
        
        MinX = minX * FixedPoint::Cos(rotation.y) * FixedPoint::Cos(rotation.z) - minZ * FixedPoint::Cos(rotation.y) * FixedPoint::Sin(rotation.z);
        MaxX = maxX * FixedPoint::Cos(rotation.y) * FixedPoint::Cos(rotation.z) - maxZ * FixedPoint::Cos(rotation.y) * FixedPoint::Sin(rotation.z);

        MinZ = minX * FixedPoint::Sin(rotation.y)  + minZ * FixedPoint::Cos(rotation.y);        
        MaxZ = maxX * FixedPoint::Sin(rotation.y) + maxZ * FixedPoint::Cos(rotation.y);
        
    }

    Vector3 Right() const {
        return { Normal.y, -Normal.x, FixedPoint(0.0f) };
    }

    /*Vector3 Up() const {
        return MathUtils::CrossProduct(Normal, Right());
    }*/
};
}
#endif