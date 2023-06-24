#ifndef __lightshaft_h_
#define __lightshaft_h_

#include <psxgte.h>

struct lightshaft {

	const SVECTOR vertices[8] = {
		{ 800, -640, 50 },
		{ 504, -640, 50 },
		{ 504, -344, 50 },
		{ 800, -344, 50 },
		{ 800, -640, -50 },
		{ 504, -640, -50 },
		{ 504, -344, -50 },
		{ 800, -344, -50 },
	};

	const DVECTOR uvs[4] = {
		{ 63, 0 },
		{ 0, 0 },
		{ 0, 63 },
		{ 63, 63 },
	};

	const SVECTOR normals[1] = {
		{ 0, 0, -4095 },
	};

	const CVECTOR colors[1] = {
		{ 255, 255, 255 },
	};

	struct face4
	{
		short vertice0,uv0,normal0,color0;
		short vertice1,uv1,normal1,color1;
		short vertice2,uv2,normal2,color2;
		short vertice3,uv3,normal3,color3;
	};

	const face4 quads[2] = {
		{ 0, 0, 0, 0, 1, 1, 0, 0, 2, 2, 0, 0, 3, 3, 0, 0 },
		{ 4, 0, 0, 0, 5, 1, 0, 0, 6, 2, 0, 0, 7, 3, 0, 0 },
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

#endif //__lightshaft_h_
