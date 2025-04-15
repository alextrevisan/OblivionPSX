#ifndef __fire_h_
#define __fire_h_

#include <psxgte.h>

struct fire {

    const SVECTOR vertices[4] = {
        { -128, -1119, -52 },
        { 735, -1119, -52 },
        { -128, 32, -52 },
        { 735, 32, -52 }
    };

    const DVECTOR uvs[4] = {
        { 0, 0 },
        { 24, 0 },
        { 0, 32 },
        { 24, 32 }
    };

    const SVECTOR normals[1] = {
        { 0, 0, 4095 }
    };

    const CVECTOR colors[1] = {
        { 128, 128, 128 }
    };

    struct face4
    {
        short vertice0,uv0,normal0,color0;
        short vertice1,uv1,normal1,color1;
        short vertice2,uv2,normal2,color2;
        short vertice3,uv3,normal3,color3;
    };

    const face4 quads[1] = {
        { 0, 0, 0, 0, 1, 1, 0, 0, 2, 2, 0, 0, 3, 3, 0, 0 }
    };

    struct face3
    {
        unsigned short vertice0,uv0,normal0,color0;
        unsigned short vertice1,uv1,normal1,color1;
        unsigned short vertice2,uv2,normal2,color2;
    };

    const face3 tris[0] = {
    };

    int x = 0, y = 0, z = 0;
    TIM_IMAGE* texture = nullptr;
};

#endif //__fire_h_