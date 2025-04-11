#ifndef _MATH_UTILS_H_
#define _MATH_UTILS_H_
#include "FixedPoint.h"
#include "Vector3.h"

namespace PhysicsEnginePSX {

/* Get the IR0 register from the GTE */
#define gte_getir0( )			\
	({ long r0;					\
	__asm__ volatile (			\
	"mfc2	%0, $8;"			\
	: "=r"( r0 )				\
	:							\
	);							\
	r0; })
/* Read the IR1,IR2,IR3 registers into a VECTOR structure. */
#define gte_stir123( r0 ) __asm__ volatile (	\
	"swc2	$9, 0( %0 );"						\
	"swc2	$10, 4( %0 );"						\
	"swc2	$11, 8( %0 );"						\
	:											\
	: "r"( r0 )									\
	: "memory" )

/* Read the MAC1,MAC2,MAC3 registers into a VECTOR structure. */
#define gte_stmac123( r0 ) __asm__ volatile (	\
	"swc2	$25, 0( %0 );"						\
	"swc2	$26, 4( %0 );"						\
	"swc2	$27, 8( %0 );"						\
	:											\
	: "r"( r0 )									\
	: "memory" )
class MathUtils {
public:
    static void RotateVector(Vector3 &vector, Vector3 &axis, FixedPoint angle) {
        FixedPoint sinAngle = FixedPoint::Sin(angle);
        FixedPoint cosAngle = FixedPoint::Cos(angle);
        FixedPoint oneMinusCosAngle = FixedPoint::One() - cosAngle;

        FixedPoint x = vector.x;
        FixedPoint y = vector[1];
        FixedPoint z = vector.z;

        FixedPoint u = axis[0];
        FixedPoint v = axis[1];
        FixedPoint w = axis[2];

        FixedPoint ux = u * x;
        FixedPoint uy = u * y;
        FixedPoint uz = u * z;

        FixedPoint vx = v * x;
        FixedPoint vy = v * y;
        FixedPoint vz = v * z;

        FixedPoint wx = w * x;
        FixedPoint wy = w * y;
        FixedPoint wz = w * z;

        FixedPoint newX = u * (ux + vy + wz) * oneMinusCosAngle + x * cosAngle + (-wy + vz) * sinAngle;
        FixedPoint newY = v * (ux + vy + wz) * oneMinusCosAngle + y * cosAngle + (wx - uz) * sinAngle;
        FixedPoint newZ = w * (ux + vy + wz) * oneMinusCosAngle + z * cosAngle + (-vx + uy) * sinAngle;

        vector.x = newX;
        vector[1] = newY;
        vector.z = newZ;
    }

    static Vector3 Normalize(const Vector3& vector) {
        VECTOR v {vector.x.AsFixedPoint(), vector.y.AsFixedPoint(), vector.z.AsFixedPoint()};
        SVECTOR output;
        VectorNormalS(&v, &output);
        return { output.vx, output.vy, output.vz };
        //FixedPoint dot = vector.x * vector.x + vector.y * vector.y + vector.z * vector.z;
        //FixedPoint length = FixedPoint::Sqrt(dot);
        //return { vector.x / length, vector.y / length, vector.z / length };
    }

    static Vector3 Subtract(const Vector3& vector, const Vector3& vector2) {
        return { vector.x - vector2.x, vector.y - vector2.y, vector.z - vector2.z };
    }

    static FixedPoint Magnitude(const Vector3& vector) {
        return FixedPoint::Sqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z);
    }

    static FixedPoint DotProduct(const Vector3& v1, const Vector3& v2) {
        return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
    }

    static Vector3 RotateNormal(float angleX, float angleY, float angleZ) {
        Vector3 normal = Vector3::Up.Rotated(Vector3::Left, -angleX)
                        .Rotated(Vector3::Up, angleY)
                        .Rotated(Vector3::Forward, -angleZ);

        return { FixedPoint(normal.x), FixedPoint(normal.y), FixedPoint(normal.z) };
    }


    //rotate using GTE
    static Vector3 RotateNormal(const Vector3& normal, FixedPoint angleX, FixedPoint angleY, FixedPoint angleZ) {
        MATRIX mtx;
        SVECTOR normal0 {normal.x.AsFixedPoint(), normal.y.AsFixedPoint(), normal.z.AsFixedPoint()};
        SVECTOR trot {angleX.AsFixedPoint()>>5, angleY.AsFixedPoint()>>5, angleZ.AsFixedPoint()>>5};
        RotMatrix(&trot, &mtx);
        gte_SetRotMatrix(&mtx);
        gte_SetTransMatrix(&mtx);
        gte_ldv0(&normal0);
        gte_rtv0();
        VECTOR out;
        gte_stir123(&out);
        SVECTOR outNormalized;
        VectorNormalS(&out, &outNormalized);
        return {
            FixedPoint::FromFixedPoint(outNormalized.vx), 
            FixedPoint::FromFixedPoint(outNormalized.vy), 
            FixedPoint::FromFixedPoint(outNormalized.vz)
            };
        /*
        const FixedPoint angle180 = FixedPoint::FromFixedPoint(65536);
        const FixedPoint pi = FixedPoint::PI();
        const FixedPoint piOver180 = pi / angle180;
        const FixedPoint radiansX = angleX * pi / angle180;
        const FixedPoint radiansY = angleY * pi / angle180;
        const FixedPoint radiansZ = angleZ * pi / angle180;

        const FixedPoint cosX = FixedPoint::Cos(radiansX);
        const FixedPoint sinX = FixedPoint::Sin(radiansX);
        const FixedPoint cosY = FixedPoint::Cos(radiansY);
        const FixedPoint sinY = FixedPoint::Sin(radiansY);
        const FixedPoint cosZ = FixedPoint::Cos(radiansZ);
        const FixedPoint sinZ = FixedPoint::Sin(radiansZ);

        // Apply rotation in X-axis
        const FixedPoint newY = normal.y * cosX - normal.z * sinX; // y = -1
        const FixedPoint newZ = normal.y * sinX + normal.z * cosX; // z = 0

        // Apply rotation in Y-axis
        const FixedPoint newX = normal.x * cosY + newZ * sinY; // x = 0
        const FixedPoint newZ2 = -normal.x * sinY + newZ * cosY; 

        // Apply rotation in Z-axis
        auto normalX = newX * cosZ - newY * sinZ;
        auto normalY = newX * sinZ + newY * cosZ;
        auto normalZ = newZ2;

        return {normalX, normalY, normalZ};*/

    }

    static Vector3 RotateNormalv1(Vector3& normal, FixedPoint angleX, FixedPoint angleY, FixedPoint angleZ) {
        FixedPoint angle180 = FixedPoint(180);
        FixedPoint pi = FixedPoint::PI();
        FixedPoint radiansX = angleX * pi / angle180;
        FixedPoint radiansY = angleY * pi / angle180;
        FixedPoint radiansZ = angleZ * pi / angle180;

        FixedPoint cosX = FixedPoint::Cos(radiansX);
        FixedPoint sinX = FixedPoint::Sin(radiansX);
        FixedPoint cosY = FixedPoint::Cos(radiansY);
        FixedPoint sinY = FixedPoint::Sin(radiansY);
        FixedPoint cosZ = FixedPoint::Cos(radiansZ);
        FixedPoint sinZ = FixedPoint::Sin(radiansZ);

        // Apply rotation in X-axis
        FixedPoint newX = normal[0];
        FixedPoint newY = normal[1] * cosX - normal[2] * sinX;
        FixedPoint newZ = normal[1] * sinX + normal[2] * cosX;

        // Apply rotation in Y-axis
        normal[0] = newX * cosY + newZ * sinY;
        normal[1] = newY;
        normal[2] = -newX * sinY + newZ * cosY;

        // Apply rotation in Z-axis
        newX = normal[0] * cosZ - normal[1] * sinZ;
        newY = normal[0] * sinZ + normal[1] * cosZ;

        normal[0] = newX;
        normal[1] = newY;

        return normal;
    }

    static Vector3 RotateNormalv0(Vector3& normal, FixedPoint angleX, FixedPoint angleY, FixedPoint angleZ) {
        // Convert angles from degrees to radians
        FixedPoint angle180 = FixedPoint(180.0f);
        FixedPoint pi = FixedPoint::PI();
        FixedPoint radiansX = angleX * pi / angle180;
        FixedPoint radiansY = angleY * pi / angle180;
        FixedPoint radiansZ = angleZ * pi / angle180;

        // Apply rotation around X-axis
        FixedPoint newX = normal[0];
        FixedPoint newY = FixedPoint(normal[1] * FixedPoint::Cos(radiansX) - normal[2] * FixedPoint::Sin(radiansX));
        FixedPoint newZ = FixedPoint(normal[1] * FixedPoint::Sin(radiansX) + normal[2] * FixedPoint::Cos(radiansX));

        // Apply rotation around Y-axis
        normal[0] = FixedPoint(newX * FixedPoint::Cos(radiansY) + newZ * FixedPoint::Sin(radiansY));
        normal[1] = FixedPoint(newY);
        normal[2] = FixedPoint(-newX * FixedPoint::Sin(radiansY) + newZ * FixedPoint::Cos(radiansY));

        // Apply rotation around Z-axis
        newX = FixedPoint(normal[0] * FixedPoint::Cos(radiansZ) - normal[1] * FixedPoint::Sin(radiansZ));
        newY = FixedPoint(normal[0] * FixedPoint::Sin(radiansZ) + normal[1] * FixedPoint::Cos(radiansZ));

        normal[0] = newX;
        normal[1] = newY;
        // normal[2] does not change in the rotation around the Z-axis

        return normal;
    }
};

} // namespace PhysicsEnginePSX
#endif //_MATH_UTILS_H_