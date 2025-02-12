#ifndef _GRAPHICS_H_
#define _GRAPHICS_H_

#include <sys/types.h>
#include <stdio.h>
#include <psxgte.h>
#include <psxgpu.h>
#include <inline_c.h>
#include <clip.h>
#include <gtemac.h>
#include <stdlib.h>

#include <Math.h>
#include <Operators.h>
#include <fast_draw_functions.h>

/// @brief Sets the DQA register
#define gte_SetDQA( r0 ) __asm__ volatile (		\
	"addu	$12,$0,%0;"						\
	"ctc2	$12, $27"					\
	:							\
	: "r"( r0 ) 			\
	: "a0" )

//-----------------------------------------------------------------------------

/// @brief Sets the DQB register
#define gte_SetDQB( r0 ) __asm__ volatile (		\
	"addu	$12,$0,%0;"				\
	"ctc2	$12, $28"					\
	:							\
	: "r"( r0 )             \
	: "$12"  )

/// @brief Sets the fog near register
void SetFogNear(long a, long h)
{
	//Error division by 0
	if(h == 0)
		return;
	int depthQ = -(((a << 2) + a) << 6);
	if(h != -1 && depthQ != 0x8000)
		return;
	gte_SetDQA(depthQ / h);
	gte_SetDQB(20971520);
}

/// @brief Sets the fog near and far registers
void SetFogNearFar(long fogNear, long fogFar, long h)
{
	short delta = fogFar-fogNear;
	if(delta >= 0x64)
	{
		int DQA,DQB;
		DQA = (
			  	(
					(
						(0-fogNear)*fogFar
					) / delta
				) << 0x8
			  ) / h;

		if(DQA < -0x8000)
			DQA = -0x8000;

		if(DQA > 0x7fff)
			DQA = 0x7fff;

		DQB = ((fogFar << 0xc) / delta) << 0xc;

//		cout << hex << "DQA = " << DQA << ", DQB = " << DQB << std::endl;

		gte_SetDQA(DQA);
		gte_SetDQB(DQB);
	}
}



namespace
{
	int fps;
	int fps_counter;
	int fps_measure;

	void vsync_cb(void)
	{
		fps_counter++;
		if (fps_counter >= 60)
		{
			fps = fps_measure;
			fps_measure = 0;
			fps_counter = 0;
		}
	}
	static constexpr RECT screen_clip = {20, 20, 280, 200};
} // namespace

struct UVMap
{
	u_char u0, v0; //top-left
	u_char u1, v1; //top-right
	u_char u2, v2; //bottom-left
	u_char u3, v3; //bottom-right
};

class GraphClass
{
	uint32_t *OrderingTable[2];
	u_char *Primitives[2];
	u_char *NextPrimitive;

	uint32_t OrderingTableCount;
	const int OrderingTableLength;
	const int PrimitivesLength;
	uint32_t DoubleBufferIndex;

	DISPENV DisplayEnvironment[2];
	DRAWENV DrawEnvironment[2];

public:
	GraphClass(int orderingTableLength, int primitivesLength)
		: OrderingTableLength(orderingTableLength), PrimitivesLength(primitivesLength)
	{
		OrderingTable[0] = (uint32_t*)malloc( sizeof(uint32_t)*orderingTableLength );
		OrderingTable[1] = (uint32_t*)malloc( sizeof(uint32_t)*orderingTableLength );

		DoubleBufferIndex = 0;
		OrderingTableCount = orderingTableLength;
		ClearOTagR(OrderingTable[0], OrderingTableCount);
		ClearOTagR(OrderingTable[1], OrderingTableCount);

		Primitives[0] = new u_char[primitivesLength];
		Primitives[1] = new u_char[primitivesLength];

		NextPrimitive = Primitives[0];

		printf("GraphClass::GraphClass: Buffers allocated.\n");

	} /* GraphClass */

	virtual ~GraphClass()
	{
	}

	void SetResolution(short width, short heigth)
	{
		Width = width;
		Height = heigth;
		ResetGraph(0);

		SetDefDispEnv(&DisplayEnvironment[0], 0, heigth, width, heigth);
		SetDefDrawEnv(&DrawEnvironment[0], 0, 0, width, heigth);

		setRGB0(&DrawEnvironment[0], 0, 0, 0);
		DrawEnvironment[0].isbg = 1;
		DrawEnvironment[0].dtd = 1;

		SetDefDispEnv(&DisplayEnvironment[1], 0, 0, width, heigth);
		SetDefDrawEnv(&DrawEnvironment[1], 0, heigth, width, heigth);

		setRGB0(&DrawEnvironment[1], 0, 0, 0);
		DrawEnvironment[1].isbg = 1;
		DrawEnvironment[1].dtd = 1;

		PutDispEnv(&DisplayEnvironment[0]);
		PutDrawEnv(&DrawEnvironment[0]);

		// Initialize the GTE
		InitGeom();

		// Set GTE offset (recommended method  of centering)
		gte_SetGeomOffset(width >> 1, heigth >> 1);

		// Set screen depth (basically FOV control, W/2 works best)
		gte_SetGeomScreen(width >> 1);

		// Set light ambient color and light color matrix
		//original day 191,182,127
		gte_SetBackColor(127, 102, 19);
		gte_SetFarColor(0, 0, 0);
		SetFogNearFar(250, 600, Width);

		VSyncCallback(vsync_cb);

	} /* SetRes */

	template <typename T, typename U>
	struct is_same
	{
		static constexpr bool value = false;
	};

	template <typename T>
	struct is_same<T, T> //specialization
	{
		static constexpr bool value = true;
	};

	inline void DrawSpriteRect(const TIM_IMAGE &tim, const RECT &rect)
	{
		DrawSpriteRect(tim, rect, {0, 0}, {128, 128, 128});
	}

	inline void DrawSpriteRect(const TIM_IMAGE &tim, const RECT &rect, const DVECTOR &uv)
	{
		DrawSpriteRect(tim, rect, uv, {128, 128, 128});
	}

	inline void DrawSpriteRect(const TIM_IMAGE &tim, const RECT &rect, const DVECTOR &uv, const CVECTOR &color)
	{
		SPRT *sprt = (SPRT *)GetNextPri();

		setSprt(sprt);
		setXY0(sprt, rect.x, rect.y);
		setWH(sprt, rect.w, rect.h);
		setRGB0(sprt, 128, 128, 128);
		setUV0(sprt, uv.vx, uv.vy);
		setClut(sprt, tim.crect->x, tim.crect->y);

		//addPrim(GetOt() + (p >> 2), polygon);
		addPrim(GetOt() + 1, sprt);

		IncPri(sizeof(SPRT));

		DR_TPAGE *tpri = (DR_TPAGE *)GetNextPri();
		auto tpage = getTPage(tim.mode, 0, tim.prect->x, tim.prect->y);
		setDrawTPage(tpri, 1, 0, tpage);
		addPrim(GetOt() + 1, tpri);
		IncPri(sizeof(DR_TPAGE));
	}

	inline void DrawSprite(const TIM_IMAGE &tim, const SVECTOR &pos)
	{
		SPRT *sprt = (SPRT *)GetNextPri();

		setSprt(sprt);
		setXY0(sprt, pos.vx, pos.vy);
		setWH(sprt, 256, 192);
		setRGB0(sprt, 128, 128, 128);
		setUV0(sprt, 0, 0);
		setClut(sprt, tim.crect->x, tim.crect->y);

		//addPrim(GetOt() + (p >> 2), polygon);
		addPrim(GetOt() + (OrderingTableLength - 1), sprt);

		IncPri(sizeof(SPRT));

		DR_TPAGE *tpri = (DR_TPAGE *)GetNextPri();
		auto tpage = getTPage(tim.mode, 0, tim.prect->x, tim.prect->y);
		setDrawTPage(tpri, 0, 0, tpage);
		addPrim(GetOt() + (OrderingTableLength - 1), tpri);
		IncPri(sizeof(DR_TPAGE));
	}

	template <typename Object, Object object>
	inline void DrawBillboard(const VECTOR &verts)
	{
		// Load the 3D coordinate of the sprite to GTE
		gte_ldv0_f(verts);

		// Rotation, Translation and Perspective Single
		gte_rtps();

		int p;
		// Store depth
		gte_stsz(&p);

		// Don't sort sprite if depth is zero
		// (or divide by zero will happen later)
		if (p > 0)
		{
			SVECTOR spos;
			// Store result to position vector
			gte_stsxy2(&spos);

			// Calculate sprite size, the divide operation might be a
			// performance killer but it's likely faster than performing
			// a lookat operation between sprite and camera, which some
			// billboard sprite implementations use.
			const int szx = ((object.texture->prect->w * Width) / p);
			const int szy = ((object.texture->prect->h * Height) / p);

			// Prepare polygon primitive
			POLY_FT4 *polygon = (POLY_FT4 *)GetNextPri();
			setPolyFT4(polygon);

			// Set polygon coordinates

			setXY4(polygon,
				   spos.vx - szx, spos.vy - szy,
				   spos.vx + szx, spos.vy - szy,
				   spos.vx - szx, spos.vy + szy,
				   spos.vx + szx, spos.vy + szy);
			/*
			spos.vx-sz, spos.vy-sz,
						spos.vx+sz, spos.vy-sz,
						spos.vx-sz, spos.vy+sz,
						spos.vx+sz, spos.vy+sz*/
			// Set color
			setRGB0(polygon, 128, 128, 128);
			
			if(object.texture->mode&0x8)
			{
				// Set tpage
				polygon->tpage = getTPage(object.texture->mode, 0, object.texture->prect->x, object.texture->prect->y);

				// Set CLUT
				setClut(polygon, object.texture->crect->x, object.texture->crect->y);
			}

			// Set texture coordinates
			const auto uv = object.uvs;
			setUV4(polygon, uv[3].vx, uv[3].vy, uv[2].vx, uv[2].vy,
				   uv[0].vx, uv[0].vy, uv[1].vx, uv[1].vy);

			/* Sort primitive to the ordering table */
			addPrim(GetOt(), polygon);
			/* Advance to make another primitive */
			IncPri(sizeof(POLY_FT4));
		}
	}

	template <typename Object, bool BackfaceCulling = true>
	inline void DrawObject3D(const Object &object, int x = 0, int y = 0, int z = 0)
	{
		constexpr auto sizeq = sizeof(object.quads) / sizeof(typename Object::face4);
		using PolygonTypeQ = POLY_FT4;
		if constexpr (sizeq > 0)
		{
			
			for (unsigned int i = 0; i < sizeq; ++i)
			{
				const auto vertex0 = object.vertices[object.quads[i].vertice0];
				const auto vertex1 = object.vertices[object.quads[i].vertice3];
				const auto vertex2 = object.vertices[object.quads[i].vertice1];
				const auto vertex3 = object.vertices[object.quads[i].vertice2];
				const auto normal = object.normals[object.quads[i].normal0];

				gte_ldv3_f(vertex0, vertex1, vertex2);
				gte_rtpt();
				gte_nclip();
				int p;

				gte_stopz(p);
				if constexpr (BackfaceCulling)
				{
					if (p <= 0)
						continue;
				}
				else
				{
					if (p == 0)
						continue;
				}

				if constexpr (is_same<PolygonTypeQ, POLY_F4>::value)
					gte_avsz4();
				else if constexpr (is_same<PolygonTypeQ, POLY_FT4>::value)
					gte_avsz4();

				gte_stotz_m(p);
				const auto p2 = p >> 2;
				if ((p2 <= 0) || p2 >= OrderingTableLength)
					continue;

				PolygonTypeQ *polygon = (PolygonTypeQ *)GetNextPri();
				if constexpr (is_same<PolygonTypeQ, POLY_F4>::value)
				{
					setPolyF4(polygon);
					gte_stsxy3_f4(polygon);
				}
				if constexpr (is_same<PolygonTypeQ, POLY_FT4>::value)
				{
					setPolyFT4(polygon);
					gte_stsxy3_ft4(polygon);
				}
				else
				{
					gte_stsxy3(&polygon->x0, &polygon->x1, &polygon->x2);
				}

				if constexpr (is_same<PolygonTypeQ, POLY_F4>::value || is_same<PolygonTypeQ, POLY_FT4>::value)
				{
					gte_ldv0_f(vertex3);
					gte_rtps();
					gte_stsxy(&polygon->x3);
				}

				setRGB0(polygon, 127,127,127);

				gte_ldrgb(&polygon->r0);
				gte_ldv0_f(normal);
				gte_ncs();
				gte_strgb(&polygon->r0);

				if constexpr (is_same<PolygonTypeQ, POLY_FT3>::value || is_same<PolygonTypeQ, POLY_FT4>::value)
				{
					if(object.texture->mode&0x8)
					{
						// Set tpage
						polygon->tpage = getTPage(object.texture->mode, 0, object.texture->prect->x, object.texture->prect->y);

						// Set CLUT
						setClut(polygon, object.texture->crect->x, object.texture->crect->y);
					}
					//setUV3
					polygon->u0 = object.uvs[object.quads[i].uv0].vx;
					polygon->v0 = object.uvs[object.quads[i].uv0].vy;

					polygon->u1 = object.uvs[object.quads[i].uv3].vx;
					polygon->v1 = object.uvs[object.quads[i].uv3].vy;

					polygon->u2 = object.uvs[object.quads[i].uv1].vx;
					polygon->v2 = object.uvs[object.quads[i].uv1].vy;
					if constexpr (is_same<PolygonTypeQ, POLY_FT4>::value)
					{
						polygon->u3 = object.uvs[object.quads[i].uv2].vx;
						polygon->v3 = object.uvs[object.quads[i].uv2].vy;
					}
				}

				addPrim(GetOt() + (p >> 2), polygon);
				IncPri(sizeof(PolygonTypeQ));
			}
		}

		constexpr auto sizet = sizeof(object.tris) / sizeof(typename Object::face3);
		if constexpr (sizet > 0)
		{
			using PolygonTypeT = POLY_FT3;
			for (unsigned int i = 0; i < sizet; ++i)
			{
				const auto vertex0 = object.vertices[object.tris[i].vertice0];
				const auto vertex1 = object.vertices[object.tris[i].vertice2];
				const auto vertex2 = object.vertices[object.tris[i].vertice1];
				const auto normal = object.normals[object.tris[i].normal0];
				//uint32_t tab[sizeof(A)]= {A::value...};

				gte_ldv3_f(vertex0, vertex1, vertex2);
				gte_rtpt();
				gte_nclip();
				int p;
				gte_stopz(&p);
				if constexpr (BackfaceCulling)
				{
					if (p <= 0)
						continue;
				}
				else
				{
					if (p == 0)
						continue;
				}

				gte_avsz3();
				gte_stotz_m(p);

				const auto p2 = p >> 2;
				if ((p2 <= 0) || p2 >= OrderingTableLength)
					continue;

				PolygonTypeT *polygon = (PolygonTypeT *)GetNextPri();
				if constexpr (is_same<PolygonTypeT, POLY_F3>::value)
				{
					setPolyF3(polygon);
					gte_stsxy3_f3(polygon);
				}
				if constexpr (is_same<PolygonTypeT, POLY_FT3>::value)
				{
					setPolyFT3(polygon);
					gte_stsxy3_ft3(polygon);
				}
				else
				{
					gte_stsxy3(&polygon->x0, &polygon->x1, &polygon->x2);
				}
				setRGB0(polygon, 127,127,127);
				gte_ldrgb(&polygon->r0);
				gte_ldv0_f(normal);
				gte_ncs();
				gte_strgb(&polygon->r0);

				if constexpr (is_same<PolygonTypeT, POLY_FT3>::value || is_same<PolygonTypeT, POLY_FT4>::value)
				{
					
					if(object.texture->mode&0x8)
					{
						// Set tpage
						polygon->tpage = getTPage(object.texture->mode, 0, object.texture->prect->x, object.texture->prect->y);

						// Set CLUT
						setClut(polygon, object.texture->crect->x, object.texture->crect->y);
					}
					//setUV3
					polygon->u0 = object.uvs[object.tris[i].uv0].vx;
					polygon->v0 = object.uvs[object.tris[i].uv0].vy;

					polygon->u1 = object.uvs[object.tris[i].uv2].vx;
					polygon->v1 = object.uvs[object.tris[i].uv2].vy;

					polygon->u2 = object.uvs[object.tris[i].uv1].vx;
					polygon->v2 = object.uvs[object.tris[i].uv1].vy;
				}

				addPrim(GetOt() + p2, polygon);
				IncPri(sizeof(PolygonTypeT));
			}
		}
		//PopMatrix();
	}
	
	template <typename Object, Object object, bool BackfaceCulling = true, typename PolygonTypeQ = POLY_FT4>
	inline void DrawObject3D(const SVECTOR& position)
	{
		constexpr auto sizeq = sizeof(object.quads) / sizeof(typename Object::face4);
		if constexpr (sizeq > 0)
		{
			//typedef POLY_F4 PolygonTypeQ;
			for (unsigned int i = 0; i < sizeq; ++i)
			{
				const auto vertex0 = position + object.vertices[object.quads[i].vertice0];
				const auto vertex1 = position + object.vertices[object.quads[i].vertice3];
				const auto vertex2 = position + object.vertices[object.quads[i].vertice1];
				const auto vertex3 = position + object.vertices[object.quads[i].vertice2];
				const auto normal = object.normals[object.quads[i].normal0];

				gte_ldv3_f(vertex0, vertex1, vertex2);

				gte_rtpt();
				gte_nclip();

				int p;
				gte_stopz_m(p);
				if constexpr (BackfaceCulling)
				{
					if (p <= 0)
						continue;
				}
				else
				{
					if (p == 0)
						continue;
				}

				

				PolygonTypeQ *polygon = (PolygonTypeQ *)GetNextPri();
				if constexpr (is_same<PolygonTypeQ, POLY_F4>::value)
				{
					setPolyF4(polygon);
					gte_stsxy3_f4(polygon);
				}
				else if constexpr (is_same<PolygonTypeQ, POLY_FT4>::value)
				{
					setPolyFT4(polygon);
					gte_stsxy3_ft4(polygon);
				}
				else
				{
					gte_stsxy3(&polygon->x0, &polygon->x1, &polygon->x2);
				}

                if (tri_clip<screen_clip>(
                            (DVECTOR *)&polygon->x0, (DVECTOR *)&polygon->x1,
                            (DVECTOR *)&polygon->x2))
                    continue;

				if constexpr (is_same<PolygonTypeQ, POLY_F4>::value)
					gte_avsz4();
				else if constexpr (is_same<PolygonTypeQ, POLY_FT4>::value)
					gte_avsz4();

				gte_stotz_m(p);
				const auto p2 = p >> 2;
				if ((p2 <= 0) || p2 >= OrderingTableLength)
					continue;

				if constexpr (is_same<PolygonTypeQ, POLY_F4>::value || is_same<PolygonTypeQ, POLY_FT4>::value)
				{
					gte_ldv0_f(vertex3);
					gte_rtps();
					gte_stsxy(&polygon->x3);
				}

				setRGB0(polygon, 127,127,127);
				gte_ldrgb(&polygon->r0);
				gte_ldv0_f(normal);
				gte_ncs();
				gte_strgb(&polygon->r0);

				if constexpr (is_same<PolygonTypeQ, POLY_FT3>::value || is_same<PolygonTypeQ, POLY_FT4>::value)
				{
					if(object.texture->mode&0x8)
					{
						// Set tpage
						polygon->tpage = getTPage(object.texture->mode, 0, object.texture->prect->x, object.texture->prect->y);

						// Set CLUT
						setClut(polygon, object.texture->crect->x, object.texture->crect->y);
					}
					//setUV3
					polygon->u0 = object.uvs[object.quads[i].uv0].vx;
					polygon->v0 = object.uvs[object.quads[i].uv0].vy;

					polygon->u1 = object.uvs[object.quads[i].uv3].vx;
					polygon->v1 = object.uvs[object.quads[i].uv3].vy;

					polygon->u2 = object.uvs[object.quads[i].uv1].vx;
					polygon->v2 = object.uvs[object.quads[i].uv1].vy;
					if constexpr (is_same<PolygonTypeQ, POLY_FT4>::value)
					{
						polygon->u3 = object.uvs[object.quads[i].uv2].vx;
						polygon->v3 = object.uvs[object.quads[i].uv2].vy;
					}
				}

				addPrim(GetOt() + p2, polygon);
				IncPri(sizeof(PolygonTypeQ));
			}
		}

		constexpr auto sizet = sizeof(object.tris) / sizeof(typename Object::face3);
		if constexpr (sizet > 0)
		{
			using PolygonTypeT = POLY_FT3;
			for (int i = 0; i < sizet; ++i)
			{
				const auto vertex0 = object.vertices[object.tris[i].vertice0];
				const auto vertex1 = object.vertices[object.tris[i].vertice2];
				const auto vertex2 = object.vertices[object.tris[i].vertice1];
				const auto normal = object.normals[object.tris[i].normal0];
				//uint32_t tab[sizeof(A)]= {A::value...};

				gte_ldv3_f(vertex0, vertex1, vertex2);
				gte_rtpt();
				gte_nclip();
				int p;
				gte_stopz(&p);

				if constexpr (BackfaceCulling)
				{
					if (p <= 0)
						continue;
				}
				else
				{
					if (p == 0)
						continue;
				}

				

				PolygonTypeT *polygon = (PolygonTypeT *)GetNextPri();
				if constexpr (is_same<PolygonTypeT, POLY_F3>::value)
				{
					setPolyF3(polygon);
					gte_stsxy3_f3(polygon);
				}
				if constexpr (is_same<PolygonTypeT, POLY_FT3>::value)
				{
					setPolyFT3(polygon);
					gte_stsxy3_ft3(polygon);
				}
				else
				{
					gte_stsxy3(&polygon->x0, &polygon->x1, &polygon->x2);
				}

                // Test if tri is off-screen, discard if so
                if (tri_clip<screen_clip>(
                            (DVECTOR *)&polygon->x0, (DVECTOR *)&polygon->x1,
                            (DVECTOR *)&polygon->x2))
                    continue;

				gte_avsz3();

				gte_stotz_m(p);

				const auto p2 = p>>2;
				if (p2 <= 0 || p2 >= OrderingTableLength)
					continue;

				setRGB0(polygon, 0,0,0);
				gte_ldrgb(&polygon->r0);
				gte_ldv0_f(normal);
				gte_ncs();
				gte_strgb(&polygon->r0);

				if constexpr (is_same<PolygonTypeT, POLY_FT3>::value)
				{
					if(object.texture->mode&0x8)
					{
						// Set tpage
						polygon->tpage = getTPage(object.texture->mode, 0, object.texture->prect->x, object.texture->prect->y);

						// Set CLUT
						setClut(polygon, object.texture->crect->x, object.texture->crect->y);
					}
					//setUV3
					polygon->u0 = object.uvs[object.tris[i].uv0].vx;
					polygon->v0 = object.uvs[object.tris[i].uv0].vy;

					polygon->u1 = object.uvs[object.tris[i].uv2].vx;
					polygon->v1 = object.uvs[object.tris[i].uv2].vy;

					polygon->u2 = object.uvs[object.tris[i].uv1].vx;
					polygon->v2 = object.uvs[object.tris[i].uv1].vy;
				}

				addPrim(GetOt() + (p >> 2), polygon);
				IncPri(sizeof(PolygonTypeT));
			}
		}
	}

	template <typename PolygonType, bool BackfaceCulling = true>
	inline void Draw(const SVECTOR (&values)[], const SVECTOR &normal, const TIM_IMAGE *texture = nullptr, const CVECTOR rgb = {127,127,127})
	{
		/* Load the first 3 vertices of a quad to the GTE */
		gte_ldv3_f(values[0], values[1], values[2]);

		/* Rotation, Translation and Perspective Triple */
		gte_rtpt();


		/*checks if the "Overflow" bit of the GTE status is set.*/
		int p;
		gte_stflg_m(p);
		
		if (p & 0x80000000)
			return;

		/* Compute normal clip for backface culling */
		gte_nclip();

		/* Get result*/
		gte_stopz_m(p);

		/* Skip this face if backfaced */
		if constexpr (BackfaceCulling)
		{
			if ( p <= 0 )
				return;
		}
		else
		{
			if (p == 0)
				return;
		}		

		

		PolygonType *polygon = (PolygonType *)GetNextPri();
		/* Initialize the primitive */
		if constexpr (is_same<PolygonType, POLY_F4>::value)
		{
			setPolyF4(polygon);
			/* Set the projected vertices to the primitive */
			gte_stsxy3_f4(polygon);
		}
		else if constexpr (is_same<PolygonType, POLY_F3>::value)
		{
			setPolyF3(polygon);
			/* Set the projected vertices to the primitive */
			gte_stsxy3_f3(polygon);
		}
		else if constexpr (is_same<PolygonType, POLY_FT3>::value)
		{
			setPolyFT3(polygon);
			/* Set the projected vertices to the primitive */
			gte_stsxy3_ft3(polygon);
		}
		else if constexpr (is_same<PolygonType, POLY_FT4>::value)
		{
			setPolyFT4(polygon);
			/* Set the projected vertices to the primitive */
			gte_stsxy3_ft4(polygon);
		}
		else 
		{
			gte_stsxy3(&polygon->x0, &polygon->x1, &polygon->x2);
		}
			
		// Test if tri is off-screen, discard if so
		if (tri_clip<screen_clip>(
					 (DVECTOR *)&polygon->x0, (DVECTOR *)&polygon->x1,
					 (DVECTOR *)&polygon->x2))
			return;
		
		/* Calculate average Z for depth sorting */
		if constexpr (is_same<PolygonType, POLY_F4>::value)
			gte_avsz4();
		else if constexpr (is_same<PolygonType, POLY_FT3>::value)
			gte_avsz3();
		else if constexpr (is_same<PolygonType, POLY_FT4>::value)
			gte_avsz4();

		gte_stotz_m(p);

		/* Skip if clipping off */
		/* (the shift right operator is to scale the depth precision) */
		const auto p2 = p>>2;
		if (p2 <= 0 || p2 >= OrderingTableLength)
			return;

		if constexpr (is_same<PolygonType, POLY_F4>::value || is_same<PolygonType, POLY_FT4>::value || is_same<PolygonType, POLY_GT4>::value)
		{
			/* Compute the last vertex and set the result */
			gte_ldv0_f(values[3]);
			gte_rtps();
			gte_stsxy(&polygon->x3);
		}

		/* Load primitive color even though gte_ncs() doesn't use it. */
		/* This is so the GTE will output a color result with the */
		/* correct primitive code. */
		CVECTOR out;
		gte_DpqColor(&rgb, p, &out);
		setRGB0(polygon, out.r,out.g,out.b);
		gte_ldrgb(&polygon->r0);

		/* Load the face normal */
		gte_ldv0_f(normal);
		
		/* Normal Color Single */
		gte_ncs();

		gte_strgb(&polygon->r0);

		if constexpr (is_same<PolygonType, POLY_FT3>::value || is_same<PolygonType, POLY_FT4>::value)
		{
			// Set tpage
			if(texture->mode&0x8)
			{
				polygon->tpage = getTPage(texture->mode, 0, texture->prect->x, texture->prect->y);
				// Set CLUT
				setClut(polygon, texture->crect->x, texture->crect->y);
			}

			// Set texture coordinates
			constexpr auto U_out = 0;
			constexpr auto V_out = 0;
			constexpr auto W_out = 128;
			constexpr auto H_out = 128;
			if constexpr (is_same<PolygonType, POLY_FT3>::value)
				setUV3(polygon, U_out + W_out, V_out, U_out + W_out, V_out + H_out, U_out, V_out);
			else
				setUVWH(polygon, 0, 0, 16, 16);
		}

		/* Sort primitive to the ordering table */
		addPrim(GetOt() + p2, polygon);
		//setaddr( p, getaddr( ot ) ), setaddr( ot, p )

		/* Advance to make another primitive */
		//NextPrimitive+=sizeof(PolygonType);
		IncPri(sizeof(PolygonType));
	}

	void IncPri(int bytes)
	{
		NextPrimitive += bytes;

	} /* IncPri */

	void SetPri(u_char *ptr)
	{
		NextPrimitive = ptr;

	} /* SetPri */

	inline u_char *GetNextPri(void) const
	{
		return (NextPrimitive);

	} /* GetNextPri */

	inline uint32_t *GetOt(void)
	{
		return (OrderingTable[DoubleBufferIndex]);

	} /* GetOt */

	const int FPS() const
	{
		return fps;
	}

	template <bool UseVSync = true>
	void Display(void)
	{
		fps_measure++;
		//printf("%d\n", fps);
		if constexpr (UseVSync)
		{
			VSync(0);
			DrawSync(0);
		}
		SetDispMask(1);

		DoubleBufferIndex = !DoubleBufferIndex;

		PutDispEnv(&DisplayEnvironment[DoubleBufferIndex]);
		PutDrawEnv(&DrawEnvironment[DoubleBufferIndex]);

		DrawOTag(OrderingTable[!DoubleBufferIndex] + (OrderingTableCount - 1));

		ClearOTagR(OrderingTable[DoubleBufferIndex], OrderingTableCount);
		NextPrimitive = Primitives[DoubleBufferIndex];

	} /* Display */

	short Width;
	short Height;

}; /* GraphClass */

#endif //_GRAPHICS_H_