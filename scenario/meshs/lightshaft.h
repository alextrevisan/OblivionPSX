#ifndef __lightshaft_h_
#define __lightshaft_h_

#include <psxgte.h>

struct lightshaft {

    const SVECTOR vertices[8] = {
        { 1398, -1198, -26 },
        { 1398, -798, -26 },
        { 998, -798, -26 },
        { 998, -1198, -26 },
        { 1398, -1198, -176 },
        { 1398, -798, -176 },
        { 998, -798, -176 },
        { 998, -1198, -176 }
    };

    const DVECTOR uvs[4] = {
        { 109, 0 },
        { 109, 63 },
        { 45, 63 },
        { 45, 0 }
    };

    const SVECTOR normals[1] = {
        { 0, 4095, 0 }
    };

    const CVECTOR colors[1] = {
        { 60, 60, 60 }
    };

    struct face4
    {
        short vertice0,uv0,normal0,color0;
        short vertice1,uv1,normal1,color1;
        short vertice2,uv2,normal2,color2;
        short vertice3,uv3,normal3,color3;
    };

    const face4 quads[0] = {
    };

    struct face3
    {
        unsigned short vertice0,uv0,normal0,color0;
        unsigned short vertice1,uv1,normal1,color1;
        unsigned short vertice2,uv2,normal2,color2;
    };

    const face3 tris[4] = {
        { 0, 1, 0, 0, 1, 0, 0, 0, 2, 2, 0, 0 },
        { 2, 3, 0, 0, 3, 2, 0, 0, 0, 0, 0, 0 },
        { 4, 1, 0, 0, 5, 0, 0, 0, 6, 2, 0, 0 },
        { 6, 3, 0, 0, 7, 2, 0, 0, 4, 0, 0, 0 },
    };

    int x = 0, y = 0, z = 0;
    TIM_IMAGE* texture = nullptr;
};

#endif //__lightshaft_h_