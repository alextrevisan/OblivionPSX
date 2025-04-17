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
    static constexpr int k[3] = { 100, 25, 15 };
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
    inline void Draw(VectorType values, const SVECTOR &normal, TIM_IMAGE *texture = nullptr, const DVECTOR (&uvs)[] = {}, const CVECTOR (&color)[] = {}, bool tiling = false, uint8_t level = 0, int z = 0, bool semitransparent = false)
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
            if (p < 0)
                return;
        }

        

        // Calculate average Z for depth sorting
        gte_avsz3();
        gte_stotz_m(p);
        

        // Skip if clipping off
        // (the shift right operator is to scale the depth precision)
        if (((p) <= 0) || ((p + z) >= OT_LEN))
            return;


        if (level < 0 && p <= k[level] && is_same<GeometryType, POLY_GT3>::value)
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

            // Define os novos triângulos com os UVs correspondentes
            const SVECTOR triangle1[3] = {values[0], m1, m3};
            const DVECTOR triangle1_uvs[3] = {uvs[0], uv_m1, uv_m3};

            const SVECTOR triangle2[3] = {m1, values[1], m2};
            const DVECTOR triangle2_uvs[3] = {uv_m1, uvs[1], uv_m2};

            /*
            const SVECTOR triangle3[3] = {m3, m2, values[2]};
            const DVECTOR triangle3_uvs[3] = {uv_m3, uv_m2, uvs[2]};

            const SVECTOR triangle4[3] = {m1, m2, m3};
            const DVECTOR triangle4_uvs[3] = {uv_m1, uv_m2, uv_m3};
            */

            const SVECTOR quad[4] = {m3, m1, values[2], m2};
            const DVECTOR quad_uvs[4] = {uv_m3, uv_m1, uvs[2], uv_m2};
            
            //FILL cracks in the subdivision
            const SVECTOR fill1[3] = {m1, values[0], values[1]};
            const DVECTOR fill1_uv[3] = {uv_m1, uvs[0], uvs[1]};

            const SVECTOR fill2[3] = {m2, values[1], values[2]};
            const DVECTOR fill2_uv[3] = {uv_m2, uvs[1], uvs[2]};

            const SVECTOR fill3[3] = {m3, values[2], values[0]};
            const DVECTOR fill3_uv[3] = {uv_m3, uvs[2], uvs[0]};
            
            Draw<POLY_GT3>(triangle1, normal, texture, triangle1_uvs, color, tiling, level + 1);
            Draw<POLY_GT3>(triangle2, normal, texture, triangle2_uvs, color, tiling, level + 1);
            //Draw<POLY_FT3>(triangle3, normal, texture, triangle3_uvs, color, tiling, level + 1);
            //Draw<POLY_FT3>(triangle4, normal, texture, triangle4_uvs, color, tiling, level + 1);

            Draw<POLY_GT4>(quad, normal, texture, quad_uvs, color, tiling, level + 1);
            Draw<POLY_FT3>(fill1, normal, texture, fill1_uv, color, tiling, 255);
            Draw<POLY_FT3>(fill2, normal, texture, fill2_uv, color, tiling, 255);
            Draw<POLY_FT3>(fill3, normal, texture, fill3_uv, color, tiling, 255);

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
        // Test if quad is off-screen, discard if so
        if constexpr (false && TriangleClip && (is_same<GeometryType, POLY_F4>::value || is_same<GeometryType, POLY_GT4>::value))
        {
            if (quad_clip<screen_clip>(
                    (DVECTOR *)&pol4->x0, (DVECTOR *)&pol4->x1,
                    (DVECTOR *)&pol4->x2, (DVECTOR *)&pol4->x3))
                return;
        }
        if constexpr (false &&TriangleClip && (is_same<GeometryType, POLY_FT3>::value || is_same<GeometryType, POLY_GT3>::value))
        {
            if (tri_clip<screen_clip>(
                    (DVECTOR *)&pol4->x0, (DVECTOR *)&pol4->x1,
                    (DVECTOR *)&pol4->x2))
                return;
        }

        /*

                const int fog_near = 0;
                const int fog_far = 1800;
                //246,215,176
                int fog_r = 246;
                int fog_g = 215;
                int fog_b = 176;
                const CVECTOR fogRGB{fog_r, fog_g, fog_b};

                int dist = gte_getir1();

                // Compute the fog factor
                int fog_factor = (dist - fog_near) * 4096 / (fog_far - fog_near);
                fog_factor = fog_factor > ONE ? ONE : fog_factor;//clamp(fog_factor, 0, 4096);
                fog_factor = fog_factor < 0 ? 0 : fog_factor;

                // Blend the color with the fog color
                int r = color.r;//(color.r >> 16) & 0xFF;
                int g = color.g;//(vertex_color >> 8) & 0xFF;
                int b = color.b;//vertex_color & 0xFF;
                r = (r * (4096 - fog_factor) + fog_r * fog_factor) >> 12;
                g = (g * (4096 - fog_factor) + fog_g * fog_factor) >> 12;
                b = (b * (4096 - fog_factor) + fog_b * fog_factor) >> 12;


                CVECTOR out;
                CVECTOR fogColor {r,g,b};
                //gte_DpqColor(&fogColor, p, &out);
                setRGB0(pol4, fogColor.r, fogColor.g, fogColor.b);
                */

        /*** estava funcionando
         int dist = gte_getir3()<<1;
         CVECTOR out;

         gte_DpqColor(&color, dist, &out);
         setRGB0(pol4, out.r, out.g, out.b);
         if constexpr (ComputeNormal)
         {
             gte_ldrgb(&pol4->r0);

             // Load the face normal
             gte_ldv0_f(normal);

             // Normal Color Single
             gte_ncs();

             // Store result to the primitive
             gte_strgb(&pol4->r0);
         }
         ***/
        const CVECTOR colors[] = {
            CVECTOR{ 255, 64, 64},
            CVECTOR{ 64, 255, 64},
            CVECTOR{ 64, 64, 255},
            CVECTOR{ 255, 255, 64},
            CVECTOR{ 255, 64, 255},
            CVECTOR{ 64, 255, 255},
            CVECTOR{ 255, 255, 255},
        };

        setRGB0(pol4, color[0].r, color[0].g, color[0].b);

        //gte_ldrgb( &pol4->r0 );
        //gte_ldv0_f( normal );
        //gte_ncs();
        //gte_strgb( &color[level].r );
        //gte_strgb3(&pol4->r0,&pol4->r1,&pol4->r2);
        //setRGB0(pol4, color[0].r, color[0].g, color[0].b);

        if constexpr (is_same<GeometryType, POLY_G3>::value || is_same<GeometryType, POLY_GT3>::value)
        {
            *(unsigned int *)&pol4->r1 = *(unsigned int *)(&color[1]);
            *(unsigned int *)&pol4->r2 = *(unsigned int *)(&color[2]);
            // setColor1(pol4, *(int*)(&color[1]));
            // setColor2(pol4, *(int*)(&color[2]));
            // setRGB2(pol4, color[2].r, color[2].g, color[2].b);
        }
        if constexpr (is_same<GeometryType, POLY_G4>::value || is_same<GeometryType, POLY_GT4>::value)
        {
            *(unsigned int *)&pol4->r1 = *(unsigned int *)(&color[1]);
            *(unsigned int *)&pol4->r2 = *(unsigned int *)(&color[2]);
            *(unsigned int *)&pol4->r3 = *(unsigned int *)(&color[3]);
            // setColor1(pol4, *(int*)(&color[1]));
            // setColor2(pol4, *(int*)(&color[2]));
            // setColor3(pol4, *(int*)(&color[3]));
            // setRGB2(pol4, color[2].r, color[2].g, color[2].b);
            // setRGB3(pol4, color[3].r, color[3].g, color[3].b);
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
    template <typename GeometryType, bool BackfaceCulling = true, bool TriangleClip = true, bool ComputeNormal = true, typename VectorType = const SVECTOR (&)[]>
    inline void DrawV2(VectorType values, const SVECTOR &normal, TIM_IMAGE *texture = nullptr, const DVECTOR (&uvs)[] = {}, const CVECTOR (&color)[] = {}, bool tiling = true)
    {
        register uint32_t   ur0     asm("$16");
        register uint32_t   ur1     asm("$17");
        register uint32_t   ur2     asm("$18");
        register uint32_t   ur3     asm("$19");
        register uint32_t   ur4     asm("$20");
        register uint32_t   ur5     asm("$21");
        GeometryType *pol4 = (GeometryType *)db_nextpri;
        // Load the first 3 vertices of a quad to the GTE
        // Copy Tri vertices from ram to cpu registers casting as ulong so that ur0 (len 32bits) contains vx and vy (2 * 8bits) 
        // Hence the use of vx, vz members
        cpu_ldr(ur0,(uint32_t*)&values[0].vx); // Put vx, vy value in ur0
        cpu_ldr(ur1,(uint32_t*)&values[0].vz); // Put vz, pad value in ur1
        cpu_ldr(ur2,(uint32_t*)&values[1].vx);
        cpu_ldr(ur3,(uint32_t*)&values[1].vz);
        cpu_ldr(ur4,(uint32_t*)&values[2].vx);
        cpu_ldr(ur5,(uint32_t*)&values[2].vz);
        // Load the gte registers from the cpu registers (gte-cpu move 1 cycle) - mtc2 %0, $0;
        cpu_gted0(ur0);
        cpu_gted1(ur1);
        cpu_gted2(ur2);
        cpu_gted3(ur3);
        cpu_gted4(ur4);
        cpu_gted5(ur5);
        // Rotation, Translation and Perspective Triple
        gte_rtpt();
        int p;
        if constexpr (BackfaceCulling)
        {
            // Compute normal clip for backface culling
            gte_nclip_b();

            // Get result
            gte_stopz_m(p);

            // Skip this face if backfaced
            if (p < 0)
                return;
        }
        // Calculate average Z for depth sorting
        gte_avsz3_b();
        gte_stotz_m(p);

        // Skip if clipping off
        // (the shift right operator is to scale the depth precision)
        if (((p) <= 0) || ((p) >= OT_LEN))
            return;

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

        if constexpr (is_same<GeometryType, POLY_F3>::value)
        {
            // Initialize a quad primitive
            setPolyF3(pol4);
            // setSemiTrans(pol4, 1);
            //  Set the projected vertices to the primitive
            gte_stsxy3_f3(pol4);
        }
        if constexpr (is_same<GeometryType, POLY_FT4>::value)
        {
            // Initialize a quad primitive
            setPolyFT4(pol4);
            // setSemiTrans(pol4, 1);

            // Set the projected vertices to the primitive
            gte_stsxy3_ft4(pol4);
        }

        if constexpr (is_same<GeometryType, POLY_F4>::value ||
                      is_same<GeometryType, POLY_FT4>::value ||
                      is_same<GeometryType, POLY_GT4>::value ||
                      is_same<GeometryType, POLY_G4>::value)
        {
            // Compute the last vertex and set the result
            gte_ldv0_f(values[3]);
            gte_rtps_b();
            gte_stsxy(&pol4->x3);
        }
        // Test if quad is off-screen, discard if so
        if constexpr (TriangleClip && (is_same<GeometryType, POLY_F4>::value || is_same<GeometryType, POLY_GT4>::value))
        {
            if (quad_clip<screen_clip>(
                    (DVECTOR *)&pol4->x0, (DVECTOR *)&pol4->x1,
                    (DVECTOR *)&pol4->x2, (DVECTOR *)&pol4->x3))
                return;
        }

        /*

                const int fog_near = 0;
                const int fog_far = 1800;
                //246,215,176
                int fog_r = 246;
                int fog_g = 215;
                int fog_b = 176;
                const CVECTOR fogRGB{fog_r, fog_g, fog_b};

                int dist = gte_getir1();

                // Compute the fog factor
                int fog_factor = (dist - fog_near) * 4096 / (fog_far - fog_near);
                fog_factor = fog_factor > ONE ? ONE : fog_factor;//clamp(fog_factor, 0, 4096);
                fog_factor = fog_factor < 0 ? 0 : fog_factor;

                // Blend the color with the fog color
                int r = color.r;//(color.r >> 16) & 0xFF;
                int g = color.g;//(vertex_color >> 8) & 0xFF;
                int b = color.b;//vertex_color & 0xFF;
                r = (r * (4096 - fog_factor) + fog_r * fog_factor) >> 12;
                g = (g * (4096 - fog_factor) + fog_g * fog_factor) >> 12;
                b = (b * (4096 - fog_factor) + fog_b * fog_factor) >> 12;


                CVECTOR out;
                CVECTOR fogColor {r,g,b};
                //gte_DpqColor(&fogColor, p, &out);
                setRGB0(pol4, fogColor.r, fogColor.g, fogColor.b);
                */

        /*** estava funcionando
         int dist = gte_getir3()<<1;
         CVECTOR out;

         gte_DpqColor(&color, dist, &out);
         setRGB0(pol4, out.r, out.g, out.b);
         if constexpr (ComputeNormal)
         {
             gte_ldrgb(&pol4->r0);

             // Load the face normal
             gte_ldv0_f(normal);

             // Normal Color Single
             gte_ncs_b();

             // Store result to the primitive
             gte_strgb(&pol4->r0);
         }
         ***/

        setRGB0(pol4, color[0].r, color[0].g, color[0].b);

        if constexpr (is_same<GeometryType, POLY_G3>::value || is_same<GeometryType, POLY_GT3>::value)
        {
            *(unsigned int *)&pol4->r1 = *(unsigned int *)(&color[1]);
            *(unsigned int *)&pol4->r2 = *(unsigned int *)(&color[2]);
            // setColor1(pol4, *(int*)(&color[1]));
            // setColor2(pol4, *(int*)(&color[2]));
            // setRGB2(pol4, color[2].r, color[2].g, color[2].b);
        }
        if constexpr (is_same<GeometryType, POLY_G4>::value || is_same<GeometryType, POLY_GT4>::value)
        {
            *(unsigned int *)&pol4->r1 = *(unsigned int *)(&color[1]);
            *(unsigned int *)&pol4->r2 = *(unsigned int *)(&color[2]);
            *(unsigned int *)&pol4->r3 = *(unsigned int *)(&color[3]);
            // setColor1(pol4, *(int*)(&color[1]));
            // setColor2(pol4, *(int*)(&color[2]));
            // setColor3(pol4, *(int*)(&color[3]));
            // setRGB2(pol4, color[2].r, color[2].g, color[2].b);
            // setRGB3(pol4, color[3].r, color[3].g, color[3].b);
        }

        gte_ldrgb(pol4->r0);

        if constexpr (is_same<GeometryType, POLY_FT3>::value ||
                      is_same<GeometryType, POLY_FT4>::value ||
                      is_same<GeometryType, POLY_GT4>::value ||
                      is_same<GeometryType, POLY_GT3>::value)
        {

            // Set tpage
            pol4->tpage = getTPage(texture->mode, 0, texture->prect->x, texture->prect->y);

            if (texture->mode & 0x8)
            {
                // Set CLUT
                setClut(pol4, texture->crect->x, texture->crect->y);
            }
            // setUV3
            pol4->u0 = uvs[0].vy;
            pol4->v0 = uvs[0].vy;

            pol4->u1 = uvs[1].vx;
            pol4->v1 = uvs[1].vy;

            pol4->u2 = uvs[2].vx;
            pol4->v2 = uvs[2].vy;
            if constexpr (is_same<GeometryType, POLY_FT4>::value || is_same<GeometryType, POLY_GT4>::value)
            {
                pol4->u3 = uvs[3].vx;
                pol4->v3 = uvs[3].vy;
            }
        }
        // Sort primitive to the ordering table
        addPrim(_orderingTable + (p), pol4);

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
        setRGB0(&db[0].draw, 180, 180, 255);
        db[0].draw.isbg = 1;
        db[0].draw.dtd = 1;

        // Define the second set of display/draw environments
        SetDefDispEnv(&db[1].disp, 0, 0, SCREEN_XRES, SCREEN_YRES);
        SetDefDrawEnv(&db[1].draw, 0, SCREEN_YRES, SCREEN_XRES, SCREEN_YRES);

        setRGB0(&db[1].draw, 180, 180, 255);
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
        gte_SetBackColor(127, 127, 127);
        gte_SetFarColor(127, 127, 127);
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