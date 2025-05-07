#ifndef _GRAPHICSV2_H_
#define _GRAPHICSV2_H_

#include <sys/types.h>
#include <stdio.h>
#include <psxgte.h>
#include <psxgpu.h>
#include <inline_c.h>
#include <clip.h>
#include <gtemac.h>
#include <stdlib.h>
#include <string.h>
#include "fast_draw_functions.h"
#include "Util.h"
#include "clip.h"
#include <cstdint>
//#include "ui.h"
//int32_t* _scratchData = reinterpret_cast<int32_t*>(0x1F800000);
//#define getScratchAddr(offset)  ((int32_t *)(_scratchData+(offset)*4))
// OT and Packet Buffer sizes
// Screen resolution
#define SCREEN_XRES 320
#define SCREEN_YRES 240

// Screen center position
#define CENTERX SCREEN_XRES >> 1
#define CENTERY SCREEN_YRES >> 1

#define OT_LEN 1024
#define PACKET_LEN 65536*2
uint8_t pad_buff[2][34];
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
} // namespace

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

#define gte_SetDQA( r0 ) __asm__ volatile (		\
	"addu	$12,$0,%0;"						\
	"ctc2	$12, $27"					\
	:							\
	: "r"( r0 ) 			\
	: "a0" )

//-----------------------------------------------------------------------------

#define gte_SetDQB( r0 ) __asm__ volatile (		\
	"addu	$12,$0,%0;"				\
	"ctc2	$12, $28"					\
	:							\
	: "r"( r0 )             \
	: "$12"  )

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

void SetFogNearFar0(long fogNear, long fogFar, long h)
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

void SetFogNearFar(long a, long b, long h)
{
	int diff; // ecx
	int dqa; // eax

	diff = b - a;
	if (b - a >= 100)
	{
		dqa = -256 * (a * b / diff) / h;
		if (dqa < -32768) dqa = -32768;
		if (dqa >  32767) dqa = 32767;
		gte_SetDQA(dqa);
		gte_SetDQB(((b << 12) / diff) << 12);
	}
}

constexpr RECT screen_clip{0, 0, SCREEN_XRES, SCREEN_YRES};

typedef struct
{
    DISPENV disp;        // Display environment
    DRAWENV draw;        // Drawing environment
    u_long _orderingTable[OT_LEN];    // Ordering table
    uint8_t _packetBuffer[PACKET_LEN]; // Packet buffer
} DB;



inline SVECTOR midpoint(const SVECTOR& p1, const SVECTOR& p2) {
    return {(p1.vx + p2.vx)>>1, (p1.vy + p2.vy)>>1, (p1.vz + p2.vz)>>1};
}
inline DVECTOR midpoint_uv(const DVECTOR& uv1, const DVECTOR& uv2) {
    return {(uv1.vx + uv2.vx) >> 1, (uv1.vy + uv2.vy) >> 1};
}
inline CVECTOR midpoint_color(const CVECTOR& color1, const CVECTOR& color2) {
    return {(color1.r + color2.r) >> 1, (color1.g + color2.g) >> 1, (color1.b + color2.b) >> 1};
}
uint16_t primCount = 0;
class Graphics
{
    // Double buffer variables
    DB* db;
    int db_active = 0;
    uint8_t *db_nextpri;

    DISPENV *disp;
    DRAWENV *draw;
    uint32_t *_orderingTable;
    uint8_t *_packetBuffer;
    enum BlendMode
    {
        Blend = 0,
        Add = 1,
        Sub = 2,
        Mul = 3
    };
    static constexpr int k[3] = { 50, 25, 15 };
public:
    #define SCREEN_OFFSET_X 0
    #define SCREEN_OFFSET_Y 0

    void draw_rectangle(int16_t x, int16_t y, int16_t width, int16_t height, CVECTOR color) {
        // Configure filled rectangle primitive
        TILE *tile = (TILE*)db_nextpri;
        
        setTile(tile);
        setRGB0(tile, color.r, color.g, color.b);
        
        // Convert coordinates to PS1 screen space
        tile->x0 = SCREEN_OFFSET_X + x;
        tile->y0 = SCREEN_OFFSET_Y + y;
        tile->w = width;
        tile->h = height;
        
        // Add to primitive buffer
        db_nextpri += sizeof(TILE);
        
        // Add to ordering table
        addPrim(_orderingTable, tile);
    }

    void draw_tile(TIM_IMAGE* tim, int16_t x, int16_t y, int16_t w, int16_t h) {
        // Configure image primitive with SPRITE
        TILE *sprt = (TILE *)db_nextpri;

		setTile(sprt);
		setXY0(sprt, x, y);
		setWH(sprt, w, h);
		setRGB0(sprt, 128, 128, 128);
		//setUV0(sprt, 0, 0);
		//setClut(sprt, tim->crect->x, tim->crect->y);
		addPrim(_orderingTable, sprt);
		db_nextpri += sizeof(SPRT);

		DR_TPAGE *tpri = (DR_TPAGE *)db_nextpri;
		auto tpage = getTPage(tim->mode, 0, tim->prect->x, tim->prect->y);
		setDrawTPage(tpri, 1, 0, tpage);
		addPrim(_orderingTable, tpri);
		db_nextpri += sizeof(DR_TPAGE);

        //const RECT texRect = {tim->prect->x, tim->prect->y, tim->prect->w>>3, tim->prect->h>>3};
        //DR_TWIN* ptwin = (DR_TWIN*)db_nextpri;
        //setTexWindow(ptwin, &texRect);
        //addPrim(_orderingTable, ptwin);
        //db_nextpri = (uint8_t*)(ptwin + 1);
    }

    void draw_sprite(TIM_IMAGE* tim, int16_t x, int16_t y, int16_t w, int16_t h, const DVECTOR& uv0, const DVECTOR& uv1) {
        // Configure image primitive with SPRITE
        SPRT *sprt = (SPRT *)db_nextpri;

		setSprt(sprt);
		setXY0(sprt, x, y);
		setWH(sprt, w, h);
		setRGB0(sprt, 128, 128, 128);
		//setUV0(sprt, uv0.x, uv0.y);
		setClut(sprt, tim->crect->x, tim->crect->y);
		addPrim(_orderingTable, sprt);
		db_nextpri += sizeof(SPRT);

		DR_TPAGE *tpri = (DR_TPAGE *)db_nextpri;
		auto tpage = getTPage(tim->mode, 0, tim->prect->x, tim->prect->y);
		setDrawTPage(tpri, 1, 0, tpage);
		addPrim(_orderingTable, tpri);
		db_nextpri += sizeof(DR_TPAGE);

        const RECT texRect = {0, 0, tim->prect->w >> 3, tim->prect->h >> 3};  // Use full texture size
        DR_TWIN* ptwin = (DR_TWIN*)db_nextpri;
        setTexWindow(ptwin, &texRect);
        addPrim(_orderingTable, ptwin);
        db_nextpri = (uint8_t*)(ptwin + 1);
    }

    void draw_image(TIM_IMAGE* tim, int16_t x, int16_t y, int16_t w, int16_t h, const DVECTOR& uv0, const DVECTOR& uv1) {
        // Configure image primitive with POLY_FT4
        POLY_FT4 *quad = (POLY_FT4 *)db_nextpri;

        setPolyFT4(quad);
        setXY4(quad, x, y, x + w, y, x, y + h, x + w, y + h);
        setRGB0(quad, 128, 128, 128);
        setUV4(quad, uv0.vx, uv0.vy, uv1.vx, uv0.vy, uv0.vx, uv1.vy, uv1.vx, uv1.vy);
        printf("CRECT: %d, %d, %d, %d, %d\n", tim->mode & 0x3, tim->prect->x, tim->prect->y, tim->prect->w, tim->prect->h);
        setClut(quad, tim->crect->x, tim->crect->y);
        quad->tpage = getTPage(tim->mode, 0, tim->prect->x, tim->prect->y);
        addPrim(_orderingTable, quad);
        db_nextpri += sizeof(POLY_FT4);

        /*
        const RECT texRect = {tim->prect->x, tim->prect->y, tim->prect->w >> 3, tim->prect->h >> 1};
        DR_TWIN* ptwin = (DR_TWIN*)db_nextpri;
        setTexWindow(ptwin, &texRect);
        addPrim(_orderingTable, ptwin);
        db_nextpri = (uint8_t*)(ptwin + 1);
        */
    }
    
    TIM_IMAGE font;
    void draw_text(const char* text, int16_t x, int16_t y, CVECTOR color) {
        // Draw each character
        int16_t cursor_x = x;
        while (*text) {
            SPRT *sprt = (SPRT *)db_nextpri;

            setSprt(sprt);
            setXY0(sprt, cursor_x, y);
            setWH(sprt, 8, 8);
            setRGB0(sprt, color.r, color.g, color.b);
            setShadeTex(sprt, 1);
            setSemiTrans(sprt, 1);
            uint8_t c = *text;
            sprt->u0 = (c % 16) * 8;
            sprt->v0 = (c / 16) * 8;

            setClut(sprt, font.crect->x, font.crect->y);
            addPrim(_orderingTable, sprt);
            db_nextpri += sizeof(SPRT);
            // Move cursor to next character position
            cursor_x += 8;
            text++;
        }

        DR_TPAGE *tpri = (DR_TPAGE *)db_nextpri;
        auto tpage = getTPage(font.mode, 0, font.prect->x, font.prect->y);
        //what are these params of setDrawTPage
        setDrawTPage(tpri, 0, 0, tpage);
        addPrim(_orderingTable, tpri);
        db_nextpri += sizeof(DR_TPAGE);
        //db_nextpri = (uint8_t*)FntSort(_orderingTable, db_nextpri, x, y, text);
    }

	inline void DrawBillboard(const SVECTOR &center, TIM_IMAGE *texture, const DVECTOR (&uvs)[], int uv_index)
	{

        int p;
		// Load the 3D coordinate of the sprite to GTE
		gte_ldv0_f(center);

		// Rotation, Translation and Perspective Single
		gte_rtps();

        int sz;
        gte_stsz(&sz);
        // Store depth
        gte_stszotz(&p);
        //printf("sz: %d | p: %d\n", sz, p);
        
		// Don't sort sprite if depth is zero
		// (or divide by zero will happen later)
		if (sz > 0)
		{
			SVECTOR spos;
			// Store result to position vector
			gte_stsxy2(&spos);

			// Calculate sprite size, the divide operation might be a
			// performance killer but it's likely faster than performing
			// a lookat operation between sprite and camera, which some
			// billboard sprite implementations use.
			const int szx = ((uvs[0].vx - uvs[1].vx) * SCREEN_XRES * 2) / sz;
			const int szy = ((uvs[0].vy - uvs[2].vy) * SCREEN_YRES * 2) / sz;

			// Prepare polygon primitive
			POLY_FT4 *polygon = (POLY_FT4 *)db_nextpri;
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
			
			if(texture->mode&0x8)
			{
				// Set tpage
				polygon->tpage = getTPage(texture->mode, 0, texture->prect->x, texture->prect->y);

				// Set CLUT
				setClut(polygon, texture->crect->x, texture->crect->y);
			}
            int width = (uvs[0].vx - uvs[1].vx) * uv_index;
			// Set texture coordinates
			setUV4(polygon, uvs[3].vx + width, uvs[3].vy, uvs[2].vx + width, uvs[2].vy,
				   uvs[0].vx + width, uvs[0].vy, uvs[1].vx + width, uvs[1].vy);

			
			addPrim(_orderingTable + p, polygon);
			/* Advance to make another primitive */
			db_nextpri += sizeof(POLY_FT4);
		}
	}


    template <typename GeometryType, bool BackfaceCulling = true, bool TriangleClip = true, bool ComputeNormal = true, typename VectorType = const SVECTOR (&)[]>
    void Draw(VectorType values, const SVECTOR &normal, TIM_IMAGE *texture = nullptr, const DVECTOR (&uvs)[] = {}, const CVECTOR (&color)[] = {}, bool tiling = false, uint8_t level = 0, int z = 0, bool semitransparent = false)
    {
        
        // Load the first 3 vertices of a quad to the GTE
        gte_ldv3_f(
            values[0],
            values[1],
            values[2]);
        // Rotation, Translation and Perspective Triple
        gte_rtpt();
        int p;
        if constexpr (true)
        {
            // Compute normal clip for backface culling
            gte_nclip();

            // Get result
            gte_stopz_m(p);

            // Skip this face if backfaced
            if (p <= 0)
                return;
        }

        

        // Calculate average Z for depth sorting
        gte_avsz3();
        gte_stotz_m(p);
        

        // Skip if clipping off
        // (the shift right operator is to scale the depth precision)
        //(p) is to match the DPQ fog precision (p<<2) so the polygons are not drawn after the fog
        if (((p) <= 0) || (((p) + z) >= OT_LEN))
            return;


        if (level < 1 && p <= k[level] && is_same<GeometryType, POLY_GT3>::value)
        {
            //printf("P: %d\n", p);
            // Calcula os pontos médios das posições
            const SVECTOR m1 = midpoint(values[0], values[1]);
            const SVECTOR m2 = midpoint(values[1], values[2]);
            const SVECTOR m3 = midpoint(values[2], values[0]);

            // Calcula os pontos médios dos UVs
            const DVECTOR uv_m1 = midpoint_uv(uvs[0], uvs[1]);
            const DVECTOR uv_m2 = midpoint_uv(uvs[1], uvs[2]);
            const DVECTOR uv_m3 = midpoint_uv(uvs[2], uvs[0]);

            const CVECTOR color_m1 = midpoint_color(color[0], color[1]);
            const CVECTOR color_m2 = midpoint_color(color[1], color[2]);
            const CVECTOR color_m3 = midpoint_color(color[2], color[0]);

            // Define os novos triângulos com os UVs correspondentes
            const SVECTOR triangle1[3] = {values[0], m1, m3};
            const DVECTOR triangle1_uvs[3] = {uvs[0], uv_m1, uv_m3};
            const CVECTOR triangle1_color[3] = {color[0], color_m1, color_m3};

            const SVECTOR triangle2[3] = {m1, values[1], m2};
            const DVECTOR triangle2_uvs[3] = {uv_m1, uvs[1], uv_m2};
            const CVECTOR triangle2_color[3] = {color_m1, color[1], color_m2};

            /*
            const SVECTOR triangle3[3] = {m3, m2, values[2]};
            const DVECTOR triangle3_uvs[3] = {uv_m3, uv_m2, uvs[2]};

            const SVECTOR triangle4[3] = {m1, m2, m3};
            const DVECTOR triangle4_uvs[3] = {uv_m1, uv_m2, uv_m3};
            */

            const SVECTOR quad[4] = {m3, m1, values[2], m2};
            const DVECTOR quad_uvs[4] = {uv_m3, uv_m1, uvs[2], uv_m2};
            const CVECTOR quad_color[4] = {color_m3, color_m1, color[2], color_m2};
            
            //FILL cracks in the subdivision
            const SVECTOR fill1[3] = {m1, values[0], values[1]};
            const DVECTOR fill1_uv[3] = {uv_m1, uvs[0], uvs[1]};
            const CVECTOR fill1_color[3] = {color_m1, color[0], color[1]};

            const SVECTOR fill2[3] = {m2, values[1], values[2]};
            const DVECTOR fill2_uv[3] = {uv_m2, uvs[1], uvs[2]};
            const CVECTOR fill2_color[3] = {color_m2, color[1], color[2]};

            const SVECTOR fill3[3] = {m3, values[2], values[0]};
            const DVECTOR fill3_uv[3] = {uv_m3, uvs[2], uvs[0]};
            const CVECTOR fill3_color[3] = {color_m3, color[2], color[0]};
            
            Draw<POLY_GT3>(triangle1, normal, texture, triangle1_uvs, triangle1_color, tiling, level + 1);
            Draw<POLY_GT3>(triangle2, normal, texture, triangle2_uvs, triangle2_color, tiling, level + 1);
            //Draw<POLY_FT3>(triangle3, normal, texture, triangle3_uvs, color, tiling, level + 1);
            //Draw<POLY_FT3>(triangle4, normal, texture, triangle4_uvs, color, tiling, level + 1);

            Draw<POLY_GT4>(quad, normal, texture, quad_uvs, quad_color, tiling, level + 1);
            Draw<POLY_GT3>(fill1, normal, texture, fill1_uv, fill1_color, tiling, 255);
            Draw<POLY_GT3>(fill2, normal, texture, fill2_uv, fill2_color, tiling, 255);
            Draw<POLY_GT3>(fill3, normal, texture, fill3_uv, fill3_color, tiling, 255);

            return;
        }

        if (primCount >= PACKET_LEN)
        {
            return;
        }
        primCount += 2;
        //const RECT texRect = {0, 0, tiling ? texture->prect->w>>3, texture->prect->h>>3};
        //DR_TWIN* ptwin = (DR_TWIN*)db_nextpri;
        //setTexWindow(ptwin, &texRect);
        //db_nextpri = (uint8_t*)(ptwin + 1);
        
        GeometryType *pol4 = (GeometryType *)db_nextpri;

        if constexpr (is_same<GeometryType, POLY_F4>::value)
        {
            // Initialize a quad primitive
            setPolyF4(pol4);
            // setSemiTrans(pol4, 1);
            //  Set the projected vertices to the primitive
            gte_stsxy3_f4(pol4);
        }

        if constexpr (is_same<GeometryType, POLY_GT3>::value)
        {
            // Initialize a quad primitive
            setPolyGT3(pol4);
            // setSemiTrans(pol4, 1);
            //  Set the projected vertices to the primitive
            gte_stsxy3_gt3(pol4);
        }

        if constexpr (is_same<GeometryType, POLY_GT4>::value)
        {
            // Initialize a quad primitive
            setPolyGT4(pol4);
            // setSemiTrans(pol4, 1);
            //  Set the projected vertices to the primitive
            gte_stsxy3_gt4(pol4);
        }

        if constexpr (is_same<GeometryType, POLY_G4>::value)
        {
            // Initialize a quad primitive
            setPolyG4(pol4);
            // setSemiTrans(pol4, 1);

            // Set the projected vertices to the primitive
            gte_stsxy3_g4(pol4);
        }

        if constexpr (is_same<GeometryType, POLY_G3>::value)
        {
            // Initialize a quad primitive
            setPolyG3(pol4);
            // setSemiTrans(pol4, 1);

            // Set the projected vertices to the primitive
            gte_stsxy3_g3(pol4);
        }

        if constexpr (is_same<GeometryType, POLY_F3>::value)
        {
            // Initialize a quad primitive
            setPolyF3(pol4);
            // setSemiTrans(pol4, 1);
            //  Set the projected vertices to the primitive
            gte_stsxy3_f3(pol4);
        }
        if constexpr (is_same<GeometryType, POLY_FT3>::value)
        {
            // Initialize a quad primitive
            setPolyFT3(pol4);
            // setSemiTrans(pol4, 1);

            // Set the projected vertices to the primitive
            gte_stsxy3_ft3(pol4);
        }
        if constexpr (is_same<GeometryType, POLY_FT4>::value)
        {
            // Initialize a quad primitive
            setPolyFT4(pol4);
            // setSemiTrans(pol4, 1);

            // Set the projected vertices to the primitive
            gte_stsxy3_ft4(pol4);
        }

        if (semitransparent)
        {
            setSemiTrans(pol4, 1);
        }

        if constexpr (is_same<GeometryType, POLY_F4>::value ||
                      is_same<GeometryType, POLY_FT4>::value ||
                      is_same<GeometryType, POLY_GT4>::value ||
                      is_same<GeometryType, POLY_G4>::value)
        {
            // Compute the last vertex and set the result
            gte_ldv0_f(values[3]);
            gte_rtps();
            gte_stsxy(&pol4->x3);
        }

        int dist = p<<2;
        
        if constexpr (is_same<GeometryType, POLY_F3>::value || is_same<GeometryType, POLY_FT3>::value)
        {
            //CVECTOR out;
            //gte_DpqColor(&color[0], dist, &out);
            setRGB0(pol4, color[0].r, color[0].g, color[0].b);
        }

        if constexpr (is_same<GeometryType, POLY_G3>::value || is_same<GeometryType, POLY_GT3>::value)
        {
            CVECTOR out[3];
            gte_DpqColor3(&color[0], &color[1], &color[2], dist, &out[0], &out[1], &out[2]);
            setRGB0(pol4, out[0].r, out[0].g, out[0].b);
            setRGB1(pol4, out[1].r, out[1].g, out[1].b);
            setRGB2(pol4, out[2].r, out[2].g, out[2].b);
        }
        if constexpr (is_same<GeometryType, POLY_G4>::value || is_same<GeometryType, POLY_GT4>::value)
        {
            CVECTOR out[4];
            gte_DpqColor3(&color[0], &color[1], &color[2], dist, &out[0], &out[1], &out[2]);
            setRGB0(pol4, out[0].r, out[0].g, out[0].b);
            setRGB1(pol4, out[1].r, out[1].g, out[1].b);
            setRGB2(pol4, out[2].r, out[2].g, out[2].b);

            gte_DpqColor(&color[3], dist, &out[3]);
            setRGB3(pol4, out[3].r, out[3].g, out[3].b);
        }
        

        if constexpr (is_same<GeometryType, POLY_FT3>::value ||
                      is_same<GeometryType, POLY_FT4>::value ||
                      is_same<GeometryType, POLY_GT4>::value ||
                      is_same<GeometryType, POLY_GT3>::value)
        {
            // Set tpage
            //int tim_uoffs = (texture->prect->x%64)<<(2-(texture->mode&0x3));
            //int tim_voffs = (texture->prect->y&0xff);
            pol4->tpage = getTPage(texture->mode, semitransparent ? 3 : 0, texture->prect->x, texture->prect->y);
            
            if (texture->mode & 0x8)
            {
                // Set CLUT
                setClut(pol4, texture->crect->x, texture->crect->y);
            }
            
            pol4->u0 = uvs[0].vx ;
            pol4->v0 = uvs[0].vy ;

            pol4->u1 = uvs[1].vx ;
            pol4->v1 = uvs[1].vy ;

            pol4->u2 = uvs[2].vx ;
            pol4->v2 = uvs[2].vy ;
            if constexpr (is_same<GeometryType, POLY_FT4>::value || is_same<GeometryType, POLY_GT4>::value)
            {
                pol4->u3 = uvs[3].vx ;
                pol4->v3 = uvs[3].vy ;
            }
        }
        // Sort primitive to the ordering table
        
        addPrim(_orderingTable + p + z, pol4);
        //addPrim(_orderingTable + p + z, ptwin);

        // Advance to make another primitive
        pol4++;
        db_nextpri = (uint8_t *)pol4;
        
    }
    void display()
    {
        const RECT texRect = {0, 0, 0, 0};
        DR_TWIN* ptwin = (DR_TWIN*)db_nextpri;
        setTexWindow(ptwin, &texRect);
        db_nextpri = (uint8_t*)(ptwin + 1);
        addPrim(_orderingTable, ptwin);

        fps_measure++;
        // Wait for GPU to finish drawing and vertical retrace
        //DrawSync( 0 );
        //VSync( 0 );

        // Swap buffers
        db_active ^= 1;
        db_nextpri = db[db_active]._packetBuffer;

        // Clear the OT of the next frame
        ClearOTagR(db[db_active]._orderingTable, OT_LEN);
        primCount = 0;
        // Apply display/drawing environments
        PutDrawEnv(&db[db_active].draw);
        PutDispEnv(&db[db_active].disp);

        // Enable display
        SetDispMask(1);

        // Start drawing the OT of the last buffer
        DrawOTag(db[1 - db_active]._orderingTable + (OT_LEN - 1));
        _orderingTable = &db[db_active]._orderingTable[0];
    }
    void init()
    {
        db = new DB[2];
        // Reset the GPU, also installs a VSync event handler
        ResetGraph(0);

        // Set display and draw environment areas
        // (display and draw areas must be separate, otherwise hello flicker)
        SetDefDispEnv(&db[0].disp, 0, SCREEN_YRES, SCREEN_XRES, SCREEN_YRES);
        SetDefDrawEnv(&db[0].draw, 0, 0, SCREEN_XRES, SCREEN_YRES);

        // Enable draw area clear and dither processing
        setRGB0(&db[0].draw, 0, 0, 0);
        db[0].draw.isbg = 1;
        db[0].draw.dtd = 1;

        // Define the second set of display/draw environments
        SetDefDispEnv(&db[1].disp, 0, 0, SCREEN_XRES, SCREEN_YRES);
        SetDefDrawEnv(&db[1].draw, 0, SCREEN_YRES, SCREEN_XRES, SCREEN_YRES);

        setRGB0(&db[1].draw, 0, 0, 0);
        db[1].draw.isbg = 1;
        db[1].draw.dtd = 1;

        // Apply the drawing environment of the first double buffer
        PutDrawEnv(&db[0].draw);

        // Clear both ordering tables to make sure they are clean at the start
        ClearOTagR(db[0]._orderingTable, OT_LEN);
        ClearOTagR(db[1]._orderingTable, OT_LEN);

        // Set primitive pointer address
        db_nextpri = db[0]._packetBuffer;

        // Set clip region
        // setRECT( &screen_clip, 0, 0, SCREEN_XRES, SCREEN_YRES );

        // Initialize the GTE
        InitGeom();

        // Set GTE offset (recommended method  of centering)
        gte_SetGeomOffset(CENTERX, CENTERY);

        // Set screen depth (basically FOV control, W/2 works best)
        gte_SetGeomScreen(CENTERX);

        // Set light ambient color and light color matrix
        gte_SetBackColor(0, 0, 0);
        gte_SetFarColor(0, 0, 0);
        SetFogNearFar(10000, 16000, CENTERX);
        //gte_SetColorMatrix(&color_mtx);

        VSyncCallback(vsync_cb);
        // Init BIOS pad driver and set pad buffers (buffers are updated
        // automatically on every V-Blank)
        InitPAD(&pad_buff[0][0], 34, &pad_buff[1][0], 34);

        // Start pad
        StartPAD();

        // Don't make pad driver acknowledge V-Blank IRQ (recommended)
        ChangeClearPAD(0);

        // Load font and open a text stream
        FntLoad(960, 0);
        FntOpen(0, 8, 320, 216, 0, 100);

        _orderingTable = &db[db_active]._orderingTable[0];
    }
};

#endif //_GRAPHICSV2_H_