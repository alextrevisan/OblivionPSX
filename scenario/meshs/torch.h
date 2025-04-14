#ifndef __torch_h_
#define __torch_h_

#include <psxgte.h>

struct torch {

    const SVECTOR vertices[8] = {
        { 303, -448, -76 },
        { 303, -281, -5 },
        { 278, -456, -51 },
        { 296, -283, 1 },
        { 303, -464, -27 },
        { 303, -285, 8 },
        { 329, -456, -51 },
        { 311, -283, 1 }
    };

    const DVECTOR uvs[12] = {
        { 24, 111 },
        { 27, 246 },
        { 32, 111 },
        { 29, 246 },
        { 34, 191 },
        { 26, 191 },
        { 34, 183 },
        { 26, 183 },
        { 42, 95 },
        { 30, 95 },
        { 42, 83 },
        { 30, 83 }
    };

    const SVECTOR normals[6] = {
        { -2888, 1131, -2673 },
        { -2887, -573, 2846 },
        { 2888, -573, 2845 },
        { 2887, 1131, -2673 },
        { 0, 3912, 1208 },
        { 0, -3912, -1208 }
    };

    const CVECTOR colors[2] = {
        { 77, 77, 77 },
        { 153, 153, 77 }
    };

    struct face4
    {
        short vertice0,uv0,normal0,color0;
        short vertice1,uv1,normal1,color1;
        short vertice2,uv2,normal2,color2;
        short vertice3,uv3,normal3,color3;
    };

    const face4 quads[6] = {
        { 1, 1, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 3, 3, 0, 0 },
        { 3, 1, 1, 0, 2, 0, 1, 0, 4, 2, 1, 0, 5, 3, 1, 0 },
        { 5, 1, 2, 0, 4, 0, 2, 0, 6, 2, 2, 0, 7, 3, 2, 0 },
        { 7, 3, 3, 0, 6, 2, 3, 0, 0, 0, 3, 0, 1, 1, 3, 0 },
        { 3, 5, 4, 0, 5, 4, 4, 0, 7, 6, 4, 0, 1, 7, 4, 0 },
        { 4, 9, 5, 1, 2, 8, 5, 1, 0, 10, 5, 1, 6, 11, 5, 1 }
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

#endif //__torch_h_