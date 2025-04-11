#include "MathUtils.h" // Include your MathUtils functions here
#include "SphereObject.h"
#include "PlaneObject.h"
#include "AABOX.h"

namespace PhysicsEnginePSX {

class CollisionSolver {
public:
    static void ResolveSpherePlaneCollision(SphereObject& sphere, const PlaneObject& plane) {
        FixedPoint distance = MathUtils::DotProduct(sphere.Position, plane.Normal) - plane.Distance;
        FixedPoint sphereToPlaneDistance = distance.Abs();
        if (sphereToPlaneDistance <= sphere.Radius) {
            sphere.Position.x += plane.Normal.x * (sphere.Radius - sphereToPlaneDistance);
            sphere.Position.y += plane.Normal.y * (sphere.Radius - sphereToPlaneDistance);
            sphere.Position.z += plane.Normal.z * (sphere.Radius - sphereToPlaneDistance);
        }
    }

    static void ResolveSpherePlaneCollisionV2(SphereObject& sphere, const PlaneObject& plane) {
        FixedPoint distance = MathUtils::DotProduct(sphere.Position, plane.Normal) - plane.Distance;
        FixedPoint sphereToPlaneDistance = distance.Abs();

        if (sphereToPlaneDistance <= sphere.Radius) {
            FixedPoint velocityAlongNormal = MathUtils::DotProduct(sphere.Velocity, plane.Normal);

            if (velocityAlongNormal < 0) {
                FixedPoint requiredForce = (velocityAlongNormal * sphere.Mass) / FixedPoint::Sqrt(sphere.Radius * sphere.Radius - sphereToPlaneDistance * sphereToPlaneDistance);

                Vector3 acceleration;
                
                acceleration.x = requiredForce * plane.Normal.x / sphere.Mass;
                acceleration.y = requiredForce * plane.Normal.y / sphere.Mass;
                acceleration.z = requiredForce * plane.Normal.z / sphere.Mass;

                for (int i = 0; i < 3; ++i) {
                    sphere.Acceleration[i] += acceleration[i];
                }

                FixedPoint velocityAdjustment = requiredForce / sphere.Mass;
                
                sphere.Velocity.x -= velocityAdjustment * plane.Normal.x;
                sphere.Velocity.y -= velocityAdjustment * plane.Normal.y;
                sphere.Velocity.z -= velocityAdjustment * plane.Normal.z;

                FixedPoint penetrationDepth = sphere.Radius - sphereToPlaneDistance;
                
                sphere.Position.x += plane.Normal.x * penetrationDepth;
                sphere.Position.y += plane.Normal.y * penetrationDepth;
                sphere.Position.z += plane.Normal.z * penetrationDepth;
            }
        }
    }

    static void ResolveSpherePlaneCollision(SphereObject& sphere, const PlaneObject& plane, FixedPoint gravityMagnitude) {
        FixedPoint distance = MathUtils::DotProduct(sphere.Position, plane.Normal) - plane.Distance;
        FixedPoint sphereToPlaneDistance = distance.Abs();

        sphere.Acceleration[1] += gravityMagnitude;

        if (sphereToPlaneDistance <= sphere.Radius) {
            FixedPoint requiredForce = FixedPoint::Zero();
            FixedPoint velocityAlongNormal = MathUtils::DotProduct(sphere.Velocity, plane.Normal);

            if (velocityAlongNormal < 0) {
                requiredForce = (velocityAlongNormal * sphere.Mass) / FixedPoint::Sqrt(sphere.Radius * sphere.Radius - sphereToPlaneDistance * sphereToPlaneDistance);

                Vector3 acceleration;
                
                acceleration.x = requiredForce * plane.Normal.x / sphere.Mass;
                acceleration.y = requiredForce * plane.Normal.y / sphere.Mass;
                acceleration.z = requiredForce * plane.Normal.z / sphere.Mass;


                sphere.Acceleration.x += acceleration.x;
                sphere.Acceleration.y += acceleration.y;
                sphere.Acceleration.z += acceleration.z;

                FixedPoint penetrationDepth = sphere.Radius - sphereToPlaneDistance;
                
                sphere.Position.x += plane.Normal.x * penetrationDepth;
                sphere.Position.y += plane.Normal.y * penetrationDepth;
                sphere.Position.z += plane.Normal.z * penetrationDepth;
            }
        } else {
            for (int i = 0; i < 3; ++i) {
                sphere.Velocity[i] += sphere.Acceleration[i];
                sphere.Position[i] += sphere.Velocity[i];
            }
        }
    }

    struct CollisionInfo {
        bool IsColliding;
        FixedPoint PenetrationDepth;
        Vector3 Normal;

        CollisionInfo() : IsColliding(false), PenetrationDepth(0), Normal{0, 0, 0} {}
    };

    static void ResolveSphereCollisionsBounce(SphereObject& sphere, FixedPoint gravityMagnitude, const PlaneObject &plane) {
        FixedPoint restitution(0.5);
        FixedPoint stoppingFactor(1);
        
            CollisionInfo collision = CheckSpherePlaneCollision(sphere, plane);

            if (collision.IsColliding) {
                FixedPoint velocityAlongNormal = MathUtils::DotProduct(sphere.Velocity, collision.Normal);

                if (velocityAlongNormal < 0) {
                    FixedPoint impulse = (FixedPoint::One() + restitution) * velocityAlongNormal * sphere.Mass * stoppingFactor;

                    for (int i = 0; i < 3; ++i) {
                        sphere.Velocity[i] -= impulse * collision.Normal[i] / sphere.Mass;
                    }

                    for (int i = 0; i < 3; ++i) {
                        sphere.Position[i] += collision.Normal[i] * collision.PenetrationDepth;
                    }
                }
            }
        

        sphere.Acceleration[1] += gravityMagnitude;

        for (int i = 0; i < 3; ++i) {
            sphere.Velocity[i] += sphere.Acceleration[i];
            sphere.Position[i] += sphere.Velocity[i];
        }
    }

    static bool ResolveSpherePlaneCollisions(SphereObject& sphere, const PlaneObject& plane) {
        bool colliding = false;
        bool onGround = false;
        
        CollisionInfo collision = CheckSpherePlaneCollision(sphere, plane);

        if (collision.IsColliding) {
            onGround = onGround || IsPlaneInclinedLessThan45Degrees(plane);
            colliding = true;

            for (int i = 0; i < 3; ++i) {
                sphere.Position[i] += collision.Normal[i] * collision.PenetrationDepth;
            }
        }
        
        if (colliding && onGround)
		{
			sphere.Acceleration[1] = FixedPoint::Zero();
			sphere.Velocity[0] = FixedPoint::Zero();
			sphere.Velocity[1] = FixedPoint::Zero();
			sphere.Velocity[2] = FixedPoint::Zero();
		}
        return onGround;
    }

    static bool ResolveSphereCubeCollisions(SphereObject& sphere, const AABOX& cube) {
        bool colliding = false;
        bool onGround = false;
        
            CollisionInfo collision = CheckSphereCubeCollision(sphere, cube);

            if (collision.IsColliding) {
                colliding = true;

                for (int i = 0; i < 3; ++i) {
                    sphere.Position[i] += collision.Normal[i] * -collision.PenetrationDepth;
                }

                onGround = onGround || IsNormalMoreThan45Degrees(collision.Normal);

                if (onGround)
                {
                    sphere.Acceleration[1] = FixedPoint::Zero();
                    sphere.Velocity[0] = FixedPoint::Zero();
                    sphere.Velocity[1] = FixedPoint::Zero();
                    sphere.Velocity[2] = FixedPoint::Zero();
                }
            }
        
        return colliding;
    }

private:
    static CollisionInfo CheckSpherePlaneCollision(SphereObject& sphere, const PlaneObject& plane) {
        CollisionInfo collisionInfo;

        if (sphere.Position[2] < plane.MinZ) return collisionInfo;
        if (sphere.Position[2] > plane.MaxZ) return collisionInfo;
        if (sphere.Position[0] < plane.MinX) return collisionInfo;
        if (sphere.Position[0] > plane.MaxX) return collisionInfo;

        FixedPoint distance = MathUtils::DotProduct(sphere.Position, plane.Normal) - plane.Distance;
        FixedPoint sphereToPlaneDistance = distance.Abs();

        if (sphereToPlaneDistance <= sphere.Radius) {
            collisionInfo.IsColliding = true;
            collisionInfo.PenetrationDepth = sphere.Radius - sphereToPlaneDistance;
            collisionInfo.Normal = plane.Normal;
        }

        return collisionInfo;
    }

    static CollisionInfo CheckSphereCubeCollision(SphereObject& sphere, const AABOX& cube) {
        CollisionInfo collisionInfo;

        Vector3 closestPoint{
            FixedPoint::Clamp(sphere.Position[0], -cube.Size.x >> 1, cube.Size.x >> 1),
            FixedPoint::Clamp(sphere.Position[1], -cube.Size.y >> 1, cube.Size.y >> 1),
            FixedPoint::Clamp(sphere.Position[2], -cube.Size.z >> 1, cube.Size.z >> 1)
        };

        FixedPoint distance = MathUtils::Magnitude(MathUtils::Subtract(closestPoint, sphere.Position));

        if (distance <= sphere.Radius) {
            collisionInfo.IsColliding = true;
            collisionInfo.PenetrationDepth = sphere.Radius - distance;
            collisionInfo.Normal = MathUtils::Normalize(MathUtils::Subtract(closestPoint, sphere.Position));
        }

        return collisionInfo;
    }

    static bool IsPlaneInclinedLessThan45Degrees(const PlaneObject& groundPlane) {
        return IsNormalMoreThan45Degrees(groundPlane.Rotation);
    }

    static bool IsNormalMoreThan45Degrees(const Vector3& normal) {
        return normal.x.Abs().AsFixedPoint() < 16384 && normal.y.Abs().AsFixedPoint() < 16384 && normal.z.Abs().AsFixedPoint() < 16384;
        static const FixedPoint angleThreshold = FixedPoint::FromFixedPoint(16384);//131072 == 360° || 16384 == 45°
        const FixedPoint dotProduct = MathUtils::DotProduct(normal, {0, -FixedPoint::One(), 0});
        const FixedPoint angle = FixedPoint::Acos(dotProduct);
        return angle < angleThreshold;
    }
};

} // namespace PhysicsEnginePSX
