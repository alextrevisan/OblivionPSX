#ifndef _AABOX_HPP_
#define _AABOX_HPP_
#include "FixedPoint.h"
#include "Vector3.h"

struct AABOX
{
	Vector3 Position; // Position of the cube's center
	Vector3 Size;     // Dimensions of the cube (width, height, depth)

	AABOX(Vector3 position, Vector3 size)
	{
		Position = position;
		Size = size;
	}
};

#endif