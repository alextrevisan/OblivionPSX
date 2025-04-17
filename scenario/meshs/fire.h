#ifndef __fire_h_
#define __fire_h_

#include <psxgte.h>

struct fire {

    const SVECTOR vertices[1] = {
        { 303, -532, -52 }
    };

    VECTOR position = { 303, -532, -52 };
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

#endif //__fire_h_