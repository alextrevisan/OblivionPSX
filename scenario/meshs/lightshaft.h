#ifndef __lightshaft_h_
#define __lightshaft_h_

#include <psxgte.h>

struct lightshaft {

    const SVECTOR vertices[4] = {
        { -725, -125, -75 },
        { 75, -125, -75 },
        { -725, 675, -75 },
        { 75, 275, -74 }
    };

    const DVECTOR uvs[4] = {
        { 0, 0 },
        { 63, 0 },
        { 0, 63 },
        { 63, 63 }
    };

    const SVECTOR normals[1] = {
        { 0, 0, -4095 }
    };

    const CVECTOR colors[1] = {
        { 51, 51, 51 }
    };

    struct face4
    {
        short vertice0,uv0,normal0,color0;
        short vertice1,uv1,normal1,color1;
        short vertice2,uv2,normal2,color2;
        short vertice3,uv3,normal3,color3;
    };

    const face4 quads[1] = {
        { 1, 1, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 3, 3, 0, 0 }
    };

    struct face3
    {
        unsigned short vertice0,uv0,normal0,color0;
        unsigned short vertice1,uv1,normal1,color1;
        unsigned short vertice2,uv2,normal2,color2;
    };

    const face3 tris[0] = {
    };

    VECTOR position = { 1520, -1187, -67 };
    SVECTOR rotation = { -81, 302, -5 };
    VECTOR aabb_min = { -725, -125, -75 };
    VECTOR aabb_max = { 75, 675, -74 };
    TIM_IMAGE* texture = nullptr;
};

#endif //__lightshaft_h_