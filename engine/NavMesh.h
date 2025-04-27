#include <psxgte.h>

int DotProduct2D(const VECTOR& a, const VECTOR& b) {
    return a.vx * b.vx + a.vz * b.vz;
}

VECTOR ClosestPointOnSegment(const VECTOR& A, const VECTOR& B, const VECTOR& P) {
    VECTOR AB = {B.vx - A.vx, 0, B.vz - A.vz};
    VECTOR AP = {P.vx - A.vx, 0, P.vz - A.vz};

    int abDot = DotProduct2D(AB, AB);
    if (abDot == 0) return A;

    int apDot = DotProduct2D(AP, AB);
    // t = apDot / abDot, clamped between 0 and 1
    int t = (apDot << 12) / abDot;
    if (t < 0) t = 0;
    if (t > 4096) t = 4096;

    VECTOR result = {
        A.vx + ((AB.vx * t) >> 12),
        0,
        A.vz + ((AB.vz * t) >> 12)
    };
    return result;
}

bool PointInTriangle(const VECTOR& p, const VECTOR tri[3]) {
    auto Sign = [](int32_t px, int32_t pz, int32_t ax, int32_t az, int32_t bx, int32_t bz) -> int32_t {
        return (px - bx) * (az - bz) - (ax - bx) * (pz - bz);
    };

    int32_t d1 = Sign(p.vx, p.vz, tri[0].vx, tri[0].vz, tri[1].vx, tri[1].vz);
    int32_t d2 = Sign(p.vx, p.vz, tri[1].vx, tri[1].vz, tri[2].vx, tri[2].vz);
    int32_t d3 = Sign(p.vx, p.vz, tri[2].vx, tri[2].vz, tri[0].vx, tri[0].vz);

    bool has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
    bool has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

    return !(has_neg && has_pos);
}

VECTOR ComputeNormal(const VECTOR tri[3]) {
    VECTOR v1 = {
        tri[1].vx - tri[0].vx,
        tri[1].vy - tri[0].vy,
        tri[1].vz - tri[0].vz
    };
    VECTOR v2 = {
        tri[2].vx - tri[0].vx,
        tri[2].vy - tri[0].vy,
        tri[2].vz - tri[0].vz
    };

    VECTOR normal = {
        v1.vy * v2.vz - v1.vz * v2.vy,
        v1.vz * v2.vx - v1.vx * v2.vz,
        v1.vx * v2.vy - v1.vy * v2.vx
    };
    return normal;
}

int CalculateY(const VECTOR& p, const VECTOR tri[3]) {
    VECTOR normal = ComputeNormal(tri);

    int A = normal.vx;
    int B = normal.vy;
    int C = normal.vz;

    int D = -(A * tri[0].vx + B * tri[0].vy + C * tri[0].vz);

    if (B != 0) {
        return -(A * p.vx + C * p.vz + D) / B;
    } else {
        return p.vy;
    }
}

template<typename Navmesh>
VECTOR ComputeNavmeshPosition(VECTOR& position, const Navmesh& navmesh, int pheight) {
    for (const auto& tri_index : navmesh.triangles) {
        const VECTOR tri[3] = {
            navmesh.vertices[tri_index.vertice0],
            navmesh.vertices[tri_index.vertice1],
            navmesh.vertices[tri_index.vertice2]
        };

        VECTOR p = {position.vx, 0, position.vz};
        if (PointInTriangle(p, tri)) {
            position.vy = CalculateY(position, tri) + pheight;
            return position;
        }
    }

    VECTOR closestPoint;
    int minDist = 0x7FFFFFFF;

    for (const auto& tri_index : navmesh.triangles) {
        VECTOR A = {navmesh.vertices[tri_index.vertice0].vx, navmesh.vertices[tri_index.vertice0].vy, navmesh.vertices[tri_index.vertice0].vz};
        VECTOR B = {navmesh.vertices[tri_index.vertice1].vx, navmesh.vertices[tri_index.vertice1].vy, navmesh.vertices[tri_index.vertice1].vz};
        VECTOR C = {navmesh.vertices[tri_index.vertice2].vx, navmesh.vertices[tri_index.vertice2].vy, navmesh.vertices[tri_index.vertice2].vz};

        const VECTOR tri[3] = {A, B, C};
        VECTOR AB[2] = {A, B};
        VECTOR BC[2] = {B, C};
        VECTOR CA[2] = {C, A};

        VECTOR proj = ClosestPointOnSegment(AB[0], AB[1], position);
        VECTOR diff = {proj.vx - position.vx, 0, proj.vz - position.vz};
        int distSq = DotProduct2D(diff, diff);
        if (distSq < minDist) {
            minDist = distSq;
            closestPoint = proj;
            closestPoint.vy = CalculateY(closestPoint, tri) + pheight;
        }

        proj = ClosestPointOnSegment(BC[0], BC[1], position);
        diff = {proj.vx - position.vx, 0, proj.vz - position.vz};
        distSq = DotProduct2D(diff, diff);
        if (distSq < minDist) {
            minDist = distSq;
            closestPoint = proj;
            closestPoint.vy = CalculateY(closestPoint, tri) + pheight;
        }

        proj = ClosestPointOnSegment(CA[0], CA[1], position);
        diff = {proj.vx - position.vx, 0, proj.vz - position.vz};
        distSq = DotProduct2D(diff, diff);
        if (distSq < minDist) {
            minDist = distSq;
            closestPoint = proj;
            closestPoint.vy = CalculateY(closestPoint, tri) + pheight;
        }
    }

    return closestPoint;
}
