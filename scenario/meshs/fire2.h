#ifndef __fire2_h_
#define __fire2_h_

#include <psxgte.h>

struct fire2 {

    const SVECTOR vertices[1] = {
        { -4, -532, 165 }
    };

    VECTOR position = { -4, -532, 165 };
    const DVECTOR uvs[4] = {
        { 24, 0 },
        { 0, 0 },
        { 0, 32 },
        { 24, 32 }
    };

    const SVECTOR normals[1] = {
        { 0, 0, 4095 }
    };

    const CVECTOR colors[1] = {
        { 128, 128, 128 }
    };

    const uint8_t max_frame_index = 7;
    const uint8_t width = 24;
};

#endif //__fire2_h_