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

    const CVECTOR colors[21] = {
        { 73, 67, 51 },
        { 87, 76, 51 },
        { 75, 68, 51 },
        { 87, 77, 51 },
        { 104, 88, 51 },
        { 129, 106, 51 },
        { 103, 87, 51 },
        { 128, 105, 51 },
        { 154, 142, 115 },
        { 166, 154, 125 },
        { 154, 143, 115 },
        { 166, 154, 125 },
        { 70, 70, 70 },
        { 73, 73, 73 },
        { 69, 69, 69 },
        { 73, 73, 73 },
        { 156, 135, 84 },
        { 157, 135, 83 },
        { 155, 134, 84 },
        { 155, 134, 84 },
        { 51, 51, 51 }
    };

    struct face4
    {
        short vertice0,uv0,normal0,color0;
        short vertice1,uv1,normal1,color1;
        short vertice2,uv2,normal2,color2;
        short vertice3,uv3,normal3,color3;
    };

    const face4 quads[6] = {
        { 1, 1, 0, 1, 0, 0, 0, 0, 2, 2, 0, 2, 3, 3, 0, 3 },
        { 3, 1, 1, 5, 2, 0, 1, 4, 4, 2, 1, 6, 5, 3, 1, 7 },
        { 5, 1, 2, 9, 4, 0, 2, 8, 6, 2, 2, 10, 7, 3, 2, 11 },
        { 7, 3, 3, 13, 6, 2, 3, 12, 0, 0, 3, 14, 1, 1, 3, 15 },
        { 3, 5, 4, 17, 5, 4, 4, 16, 7, 6, 4, 18, 1, 7, 4, 19 },
        { 4, 9, 5, 20, 2, 8, 5, 20, 0, 10, 5, 20, 6, 11, 5, 20 }
    };

    struct face3
    {
        unsigned short vertice0,uv0,normal0,color0;
        unsigned short vertice1,uv1,normal1,color1;
        unsigned short vertice2,uv2,normal2,color2;
    };

    const face3 tris[0] = {
    };

    VECTOR position = { -278, 380, 217 };
    SVECTOR rotation = { 0, 0, 0 };
    VECTOR aabb_min = { 249, -844, -76 };
    VECTOR aabb_max = { 300, -661, 8 };
    TIM_IMAGE* texture = nullptr;
};

#endif //__torch_h_