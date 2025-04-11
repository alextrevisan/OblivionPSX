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
//int32_t* _scratchData = reinterpret_cast<int32_t*>(0x1F800000);
//#define getScratchAddr(offset)  ((int32_t *)(_scratchData+(offset)*4))
// OT and Packet Buffer sizes
#define OT_LEN 1024
#define PACKET_LEN 32768*2
// Double buffer structure
typedef struct
{
    DISPENV disp;        // Display environment
    DRAWENV draw;        // Drawing environment
    u_long _orderingTable[OT_LEN];    // Ordering table
    uint8_t _packetBuffer[PACKET_LEN]; // Packet buffer
} DB;
class Graphics
{
    // Double buffer variables
    DB* db;
    int db_active = 0;
    uint8_t *db_nextpri;

    DISPENV *disp;
    DRAWENV *draw;
    u_long *_orderingTable;
    uint8_t *_packetBuffer;
    enum BlendMode
    {
        Blend = 0,
        Add = 1,
        Sub = 2,
        Mul = 3
    };

public:
    int currentAvz = 0;
    POLY_GT4* primitive;
    [[nodiscard]] POLY_GT4* LoadPolyGT4(const SVECTOR(&values) [4])
    {
        primitive = (POLY_GT4 *)db_nextpri;
        gte_ldv3_f(
            values[0],
            values[1],
            values[2]);
        gte_rtpt();

        gte_nclip();
        gte_stopz_m(currentAvz);
        if (currentAvz < 0)
            return primitive = nullptr;

        gte_avsz3();
        gte_stotz_m(currentAvz);

        if (((currentAvz) <= 0) || ((currentAvz) >= OT_LEN))
            return primitive = nullptr;
        setPolyGT4(primitive);
        gte_stsxy3_gt4(primitive);

        gte_ldv0_f(values[3]);
        gte_rtps_b();
        gte_stsxy(&primitive->x3);

        if (quad_clip<screen_clip>(
                    (DVECTOR *)&primitive->x0, (DVECTOR *)&primitive->x1,
                    (DVECTOR *)&primitive->x2, (DVECTOR *)&primitive->x3))
                return primitive = nullptr;
        return primitive;
    }

    void LoadColor(const CVECTOR (&color)[4] = {})
    {
        setRGB0(primitive, color[0].r, color[0].g, color[0].b);

        *(unsigned int *)&primitive->r1 = *(unsigned int *)(&color[1]);
        *(unsigned int *)&primitive->r2 = *(unsigned int *)(&color[2]);
        *(unsigned int *)&primitive->r3 = *(unsigned int *)(&color[3]);
    }

    void LoadTexture(TIM_IMAGE* texture, const DVECTOR (&uvs)[4] = {})
    {
        primitive->tpage = getTPage(texture->mode, 0, texture->prect->x, texture->prect->y);

        if (texture->mode & 0x8)
        {
            setClut(primitive, texture->crect->x, texture->crect->y);
        }

        primitive->u0 = uvs[0].vx;
        primitive->v0 = uvs[0].vy;

        primitive->u1 = uvs[1].vx;
        primitive->v1 = uvs[1].vy;

        primitive->u2 = uvs[2].vx;
        primitive->v2 = uvs[2].vy;

        primitive->u3 = uvs[3].vx;
        primitive->v3 = uvs[3].vy;
    }

    void Enqueue()
    {
        addPrim(_orderingTable + (currentAvz), primitive);

        primitive++;
        db_nextpri = (uint8_t *)primitive;
    }
    template <typename VectorType = const SVECTOR (&)[]>
    inline void DrawGT4(VectorType values, const SVECTOR &normal, TIM_IMAGE *texture = nullptr, const DVECTOR (&uvs)[] = {}, const CVECTOR (&color)[] = {})
    {
        

        

        
    }    
template <typename GeometryType, bool BackfaceCulling = true, bool TriangleClip = true, bool ComputeNormal = true, typename VectorType = const SVECTOR (&)[]>
    inline void Draw(VectorType values, const SVECTOR &normal, TIM_IMAGE *texture = nullptr, const DVECTOR (&uvs)[] = {}, const CVECTOR (&color)[] = {}, bool semiTransparent = false)
    {
        GeometryType *pol4 = (GeometryType *)db_nextpri;
        // Load the first 3 vertices of a quad to the GTE
        gte_ldv3_f(
            values[0],
            values[1],
            values[2]);
        // Rotation, Translation and Perspective Triple
        gte_rtpt();
        int p;
        if constexpr (false)
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
        if (((p) <= 0) || ((p) >= OT_LEN))
            return;

        if constexpr (is_same<GeometryType, POLY_F4>::value)
        {
            // Initialize a quad primitive
            setPolyF4(pol4);
            
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

        //if(semiTransparent)
        //    setSemiTrans(pol4, 1);

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

        //gte_ldrgb(pol4->r0);

        if constexpr (is_same<GeometryType, POLY_FT3>::value ||
                      is_same<GeometryType, POLY_FT4>::value ||
                      is_same<GeometryType, POLY_GT4>::value ||
                      is_same<GeometryType, POLY_GT3>::value)
        {

            // Set tpage
            pol4->tpage = getTPage(texture->mode, semiTransparent ? 3 : 0, texture->prect->x, texture->prect->y);

            if (texture->mode & 0x8)
            {
                // Set CLUT
                setClut(pol4, texture->crect->x, texture->crect->y);
            }
            // setUV3 1, 0, 2
            pol4->u0 = uvs[0].vx;
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
    template <typename GeometryType, bool BackfaceCulling = true, bool TriangleClip = true, bool ComputeNormal = true, typename VectorType = const SVECTOR (&)[]>
    inline void DrawV2(VectorType values, const SVECTOR &normal, TIM_IMAGE *texture = nullptr, const DVECTOR (&uvs)[] = {}, const CVECTOR (&color)[] = {})
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
            pol4->u0 = uvs[0].vx;
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

        fps_measure++;
        // Wait for GPU to finish drawing and vertical retrace
         DrawSync( 0 );
         VSync( 0 );

        // Swap buffers
        db_active ^= 1;
        db_nextpri = db[db_active]._packetBuffer;

        // Clear the OT of the next frame
        ClearOTagR(db[db_active]._orderingTable, OT_LEN);

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
        gte_SetColorMatrix(&color_mtx);

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