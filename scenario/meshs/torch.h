#ifndef __torch_h_
#define __torch_h_

#include <psxgte.h>

struct torch {

    const SVECTOR vertices[8] = {
        { 274, -828, -76 },
        { 274, -661, -5 },
        { 249, -836, -51 },
        { 267, -663, 1 },
        { 274, -844, -27 },
        { 274, -665, 8 },
        { 300, -836, -51 },
        { 282, -663, 1 }
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

    const CVECTOR colors[7] = {
        { 77, 77, 77 },
        { 92, 77, 77 },
        { 95, 77, 77 },
        { 143, 101, 77 },
        { 146, 103, 77 },
        { 149, 104, 77 },
        { 151, 106, 77 }
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
        { 7, 3, 3, 1, 6, 2, 3, 0, 0, 0, 3, 0, 1, 1, 3, 2 },
        { 3, 5, 4, 4, 5, 4, 4, 3, 7, 6, 4, 5, 1, 7, 4, 6 },
        { 4, 9, 5, 0, 2, 8, 5, 0, 0, 10, 5, 0, 6, 11, 5, 0 }
    };

    struct face3
    {
        unsigned short vertice0,uv0,normal0,color0;
        unsigned short vertice1,uv1,normal1,color1;
        unsigned short vertice2,uv2,normal2,color2;
    };

    const face3 tris[0] = {
    };

    VECTOR position = { 28, 380, 0 };
    SVECTOR rotation = { 0, 0, 0 };
    TIM_IMAGE* texture = nullptr;
};

#endif //__torch_h_