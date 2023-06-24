#include <sys/types.h>
#include <stdio.h>
#include <psxgpu.h>
#include <psxgte.h>
#include <psxpad.h>
#include <psxapi.h>
#include <psxetc.h>
#include <inline_c.h>
#include <gtemac.h>

#include "scenario/meshs/oblivion.h"
#include "scenario/meshs/lightshaft.h"

#include "clip.h"
#include "engine/lookat.h"
#include "engine/fast_draw_functions.h"

#include "engine/TextureManager.h"

#include "engine/tinyphysicsengine.h"
// OT and Packet Buffer sizes
#define OT_LEN 4096
#define PACKET_LEN 32768

// Screen resolution
#define SCREEN_XRES 320
#define SCREEN_YRES 240

// Screen center position
#define CENTERX SCREEN_XRES >> 1
#define CENTERY SCREEN_YRES >> 1
#define gte_getir1( )			\
	({ long r0;					\
	__asm__ volatile (			\
	"mfc2	%0, $9;"			\
	: "=r"( r0 )				\
	:							\
	);							\
	r0; })

/* Get the IR3 register from the GTE */
#define gte_getir3( )			\
	({ long r0;					\
	__asm__ volatile (			\
	"mfc2	%0, $11;"			\
	: "=r"( r0 )				\
	:							\
	);							\
	r0; })
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

// Double buffer structure
typedef struct
{
    DISPENV disp;        // Display environment
    DRAWENV draw;        // Drawing environment
    u_long _orderingTable[OT_LEN];    // Ordering table
    uint8_t _packetBuffer[PACKET_LEN]; // Packet buffer
} DB;

extern uint32_t light_shaft_tim[];
extern uint32_t textures_lvl1_tim[];

TIM_IMAGE light_shaft_texture;
TIM_IMAGE textures_lvl1_texture;

constexpr RECT screen_clip{0, 0, SCREEN_XRES, SCREEN_YRES};

// Pad data buffer
uint8_t pad_buff[2][34];

// For easier handling of vertex indexes
typedef struct
{
    short v0, v1, v2, v3;
} INDEX;

// Cube vertices
SVECTOR cube_verts[] = {
    {-100, -100, -100, 0},
    {100, -100, -100, 0},
    {-100, 100, -100, 0},
    {100, 100, -100, 0},
    {100, -100, 100, 0},
    {-100, -100, 100, 0},
    {100, 100, 100, 0},
    {-100, 100, 100, 0}};

// Cube face normals
SVECTOR cube_norms[] = {
    {0, 0, -ONE, 0},
    {0, 0, ONE, 0},
    {0, -ONE, 0, 0},
    {0, ONE, 0, 0},
    {-ONE, 0, 0, 0},
    {ONE, 0, 0, 0}};

// Cube vertex indices
INDEX cube_indices[] = {
    {0, 1, 2, 3},
    {4, 5, 6, 7},
    {5, 4, 0, 1},
    {6, 7, 3, 2},
    {0, 2, 5, 7},
    {3, 1, 6, 4}};

// Number of faces of cube
#define CUBE_FACES 6

// Light color matrix
// Each column represents the color matrix of each light source and is
// used as material color when using gte_ncs() or multiplied by a
// source color when using gte_nccs(). 4096 is 1.0 in this matrix
// A column of zeroes effectively disables the light source.
MATRIX color_mtx = {
    ONE>>1, 0, 0, // Red
    ONE>>1, 0, 0,   // Green
    ONE>>1, 0, 0  // Blue
};

// Light matrix
// Each row represents a vector direction of each light source.
// An entire row of zeroes effectively disables the light source.
MATRIX light_mtx = {
    /* X,  Y,  Z */
    -4058, -2048, -2048,
    0, 0, 0,
    0, 0, 0};

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

class Graphics
{
    // Double buffer variables
    DB db[2];
    int db_active = 0;
    uint8_t *db_nextpri;

    DISPENV* disp;
    DRAWENV* draw;
    u_long* _orderingTable;
    uint8_t* _packetBuffer;
	enum BlendMode {
		Blend = 0,
		Add = 1,
		Sub = 2,
		Mul =  3
	};
public:
    template <typename GeometryType, bool BackfaceCulling = true, bool TriangleClip = true, bool ComputeNormal = true>
    inline void Draw(const SVECTOR (&values)[], const SVECTOR &normal, TIM_IMAGE *texture = nullptr, const DVECTOR (&uvs)[] = {}, const CVECTOR (&color)[] = {})
    {
        GeometryType* pol4 = (GeometryType *)db_nextpri;
        // Load the first 3 vertices of a quad to the GTE
        gte_ldv3_f(
            values[0],
            values[1],
            values[2]);
        // Rotation, Translation and Perspective Triple
        gte_rtpt_b();
        int p;
        if constexpr (false)
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
        if (((p ) <= 0) || ((p ) >= OT_LEN))
            return;
        
        if constexpr(is_same<GeometryType, POLY_F4>::value)
        {
            // Initialize a quad primitive
            setPolyF4((GeometryType *)pol4);
            setSemiTrans(pol4, 1);
            // Set the projected vertices to the primitive
            gte_stsxy3_f4(pol4);
        }

        if constexpr(is_same<GeometryType, POLY_GT3>::value)
        {
            // Initialize a quad primitive
            setPolyGT3((GeometryType *)pol4);
            setSemiTrans(pol4, 1);
            // Set the projected vertices to the primitive
            gte_stsxy3_gt3(pol4);
        }

        if constexpr(is_same<GeometryType, POLY_GT4>::value)
        {
            // Initialize a quad primitive
            setPolyGT4((GeometryType *)pol4);
            setSemiTrans(pol4, 1);
            // Set the projected vertices to the primitive
            gte_stsxy3_gt4(pol4);
        }

		if constexpr(is_same<GeometryType, POLY_G4>::value)
        {
            // Initialize a quad primitive
            setPolyG4((GeometryType *)pol4);
            setSemiTrans(pol4, 1);
			
            // Set the projected vertices to the primitive
            gte_stsxy3_g4(pol4);
        }

        if constexpr(is_same<GeometryType, POLY_F3>::value)
        {
            // Initialize a quad primitive
            setPolyF3((GeometryType *)pol4);
            setSemiTrans(pol4, 1);
            // Set the projected vertices to the primitive
            gte_stsxy3_f3(pol4);
        }
        if constexpr (is_same<GeometryType, POLY_FT4>::value)
        {
            // Initialize a quad primitive
            setPolyFT4((GeometryType *)pol4);
            setSemiTrans(pol4, 1);
            
            // Set the projected vertices to the primitive
            gte_stsxy3_ft4(pol4);
        }

        
        if constexpr(is_same<GeometryType, POLY_F4>::value || 
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
        setRGB0(pol4, color[0].r>>1, color[0].g>>1, color[0].b>>1);
        if constexpr(is_same<GeometryType, POLY_G3>::value || is_same<GeometryType, POLY_GT3>::value)
        {
            setRGB1(pol4, color[1].r>>1, color[1].g>>1, color[1].b>>1);
            setRGB2(pol4, color[2].r>>1, color[2].g>>1, color[2].b>>1);
        }
        if constexpr(is_same<GeometryType, POLY_G4>::value || is_same<GeometryType, POLY_GT4>::value)
        {
            setRGB1(pol4, color[1].r>>1, color[1].g>>1, color[1].b>>1);
            setRGB2(pol4, color[2].r>>1, color[2].g>>1, color[2].b>>1);
            setRGB3(pol4, color[3].r>>1, color[3].g>>1, color[3].b>>1);
			
        }

        gte_ldrgb(&pol4->r0);

        if constexpr (is_same<GeometryType, POLY_FT3>::value || 
                      is_same<GeometryType, POLY_FT4>::value || 
                      is_same<GeometryType, POLY_GT4>::value || 
                      is_same<GeometryType, POLY_GT3>::value)
        {


            // Set tpage
            pol4->tpage = getTPage(texture->mode, BlendMode::Mul, texture->prect->x, texture->prect->y);

            if(texture->mode&0x8)
            {
                // Set CLUT
                setClut(pol4, texture->crect->x, texture->crect->y);
            }
            //setUV3
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
        addPrim(_orderingTable + (p ), pol4);

        // Advance to make another primitive
        pol4++;
        db_nextpri = (uint8_t*) pol4;
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

        // Reset the GPU, also installs a VSync event handler
        ResetGraph(0);

        // Set display and draw environment areas
        // (display and draw areas must be separate, otherwise hello flicker)
        SetDefDispEnv(&db[0].disp, 0, SCREEN_YRES, SCREEN_XRES, SCREEN_YRES);
        SetDefDrawEnv(&db[0].draw, 0, 0, SCREEN_XRES, SCREEN_YRES);
        

        // Enable draw area clear and dither processing
        setRGB0(&db[0].draw, 180,180,255);
        db[0].draw.isbg = 1;
        db[0].draw.dtd = 1;

        // Define the second set of display/draw environments
        SetDefDispEnv(&db[1].disp, 0, 0, SCREEN_XRES, SCREEN_YRES);
        SetDefDrawEnv(&db[1].draw, 0, SCREEN_YRES, SCREEN_XRES, SCREEN_YRES);


        setRGB0(&db[1].draw, 180,180,255);
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

// Function declarations

void draw_tree(MATRIX *mtx, VECTOR *pos, SVECTOR *rot);
void draw_mybox(MATRIX *mtx, VECTOR *pos, SVECTOR *rot);
void draw_floor(MATRIX *mtx, VECTOR *pos, SVECTOR *rot);

Graphics *graphics;
SVECTOR verts[17][17]; // Vertex array for floor
int px, py;

template<typename T>
T abs(T& value)
{
    if(value < 0)
        return value * -1;
    return value;
}

TPE_Unit elevatorHeight;
TPE_Unit ramp[6] = { 1600,0, -500,1400, -700,0 };
TPE_Unit ramp2[6] = { 2000,-5000, 1500,1700, -5000,-500 };

constexpr TPE_Vec3 aabbMin{-625<<2, 0, -625<<2};
constexpr TPE_Vec3 aabbMax{625<<2, 625<<2, 625<<2};

TPE_Vec3 environmentDistance(TPE_Vec3 p, TPE_Unit maxD)
{
  // manually created environment to match the 3D model of it
  TPE_ENV_START( TPE_envAABoxInsideAA(p,aabbMin,aabbMax),p )
  TPE_ENV_END
}

TPE_Vec3 TPE_neg(TPE_Vec3 v)
{
    return TPE_vec3(-v.x, -v.y, -v.z);
}

int main()
{

    TPE_Body bodies[10];
    TPE_Joint joints[30];
    TPE_Connection tpe_connections[60];
    int jointsUsed = 0, helper_connectionsUsed = 0;
    TPE_World world;
    int frame = 0;
    TPE_worldInit(&world, bodies, 0,0);
    world.environmentFunction = environmentDistance;





    TPE_make2Line(joints+jointsUsed, tpe_connections+helper_connectionsUsed, 400, 300);

    TPE_bodyInit(&bodies[0],
        &joints[jointsUsed], 2,
        &tpe_connections[helper_connectionsUsed], 1 , 400);
    
    jointsUsed += 2;
    helper_connectionsUsed += 1;

    world.bodyCount++;

    TPE_Body *playerBody = 0;
    playerBody = &(world.bodies[0]);

    TPE_bodyMoveBy(&world.bodies[0],TPE_vec3(0,ONE>>2,0));
    TPE_bodyRotateByAxis(&world.bodies[0],TPE_vec3(0,0,TPE_F / 4));
    playerBody->elasticity = 0;
    playerBody->friction = 0;   
    playerBody->flags |= TPE_BODY_FLAG_NONROTATING; // make it always upright
    playerBody->flags |= TPE_BODY_FLAG_ALWAYS_ACTIVE;
    TPE_Unit groundDist = TPE_JOINT_SIZE(playerBody->joints[0]) + 512;

    int i, p, xy_temp;
    

    SVECTOR rot; // Rotation vector for cube
    VECTOR pos;     // Position vector for cube

    

    VECTOR cam_pos; // Camera position (in fixed point integers)
    VECTOR cam_rot; // Camera view angle (in fixed point integers)
    int cam_mode;    // Camera mode (between first-person and look-at)

    VECTOR tpos;      // Translation value for matrix calculations
    SVECTOR trot;      // Rotation value for matrix calculations
    MATRIX mtx, lmtx; // Rotation matrices for geometry and lighting

    PADTYPE *pad; // Pad structure pointer for parsing controller

    POLY_F4 *pol4; // Flat shaded quad primitive pointer

    graphics = new Graphics();
    // Init graphics and GTE
    graphics->init();

    TextureManager::LoadTexture(textures_lvl1_tim, textures_lvl1_texture);
    TextureManager::LoadTexture(light_shaft_tim, light_shaft_texture);

    // Set coordinates to the vertex array for the floor
    for (py = 0; py < 17; py++)
    {
        for (px = 0; px < 17; px++)
        {

            setVector(&verts[py][px],
                      (100 * (px - 8)) - 50,
                      0,
                      (100 * (py - 8)) - 50);
        }
    }

    // Camera default coordinates
    setVector(&cam_pos, 0, ONE * -200, ONE * -0);
    setVector(&cam_rot, 0, 0, 0);
    
    // Main loop
    while (1)
    {

        // Set pad buffer data to pad pointer
        pad = (PADTYPE *)&pad_buff[0][0];

        // Parse controller input
        cam_mode = 0;

        // Divide out fractions of camera rotation
        trot.vx = cam_rot.vx >> 12;
        trot.vy = cam_rot.vy >> 12;
        trot.vz = cam_rot.vz >> 12;

        TPE_worldStep(&world);

        TPE_Vec3 groundPoint = environmentDistance(playerBody->joints[0].position,groundDist);

        bool onGround = (playerBody->flags & TPE_BODY_FLAG_DEACTIVATED) ||
        (TPE_DISTANCE(playerBody->joints[0].position,groundPoint)
        <= groundDist && groundPoint.y < playerBody->joints[0].position.y - groundDist / 2);

        if (!onGround)
        {
            /* it's possible that the closest point is e.g. was a perpend wall so also
                additionally check directly below */

            onGround = TPE_DISTANCE( playerBody->joints[0].position,
            TPE_castEnvironmentRay(playerBody->joints[0].position,
            TPE_vec3(0,-1 * TPE_F,0),world.environmentFunction, 
            128,512,512)) <= groundDist;
        }

        TPE_bodyMultiplyNetSpeed(&world.bodies[0],onGround ? 300 : 505);
        if(!onGround)
            TPE_bodyApplyGravity(playerBody, 8);
        //TPE_Unit height = TPE_bodyGetCenterOfMass(playerBody).y;

        if (pad->stat == 0)
        {

            // For digital pad, dual-analog and dual-shock
            if ((pad->type == 0x4) || (pad->type == 0x5) || (pad->type == 0x7))
            {

                // The button status bits are inverted,
                // so 0 means pressed in this case

                // Look controls
                if (!(pad->btn & PAD_UP))
                {

                    // Look up
                    cam_rot.vx -= ONE * 16;
                }
                else if (!(pad->btn & PAD_DOWN))
                {

                    // Look down
                    cam_rot.vx += ONE * 16;
                }

                if (!(pad->btn & PAD_LEFT))
                {

                    // Look left
                    cam_rot.vy += ONE * 16;
                }
                else if (!(pad->btn & PAD_RIGHT))
                {

                    // Look right
                    cam_rot.vy -= ONE * 16;
                }

                    
                    

                if (!(pad->btn & PAD_CIRCLE))
                {
                    playerBody->joints[0].velocity[1] = 90;
                }

                // Movement controls
                if (!(pad->btn & PAD_TRIANGLE))
                {
                    playerBody->joints[0].velocity[0] = -TPE_sin(trot.vy>>3)>>3;
                    playerBody->joints[0].velocity[2] = TPE_cos(trot.vy>>3)>>3;
                    // Move forward
                    //sphere.velocity.vx = -((isin(trot.vy) * icos(trot.vx)) >> 12) << 2;
                    //sphere.velocity.vy = ((isin(trot.vy) * icos(trot.vx)) >> 12) << 2;
                    //sphere.velocity.vz = ((icos(trot.vy) * icos(trot.vx)) >> 12) << 2;
                    //cam_pos.vx -= ((isin(trot.vy) * icos(trot.vx)) >> 12) << 2;
                    //cam_pos.vy += isin(trot.vx) << 2;
                    //cam_pos.vz += ((icos(trot.vy) * icos(trot.vx)) >> 12) << 2;
                }
                else if (!(pad->btn & PAD_CROSS))
                {
                    playerBody->joints[0].velocity[0] = (TPE_sin(trot.vy>>3))>>3;
                    playerBody->joints[0].velocity[2] = -(TPE_cos(trot.vy>>3))>>3;
                    //sphere.velocity.vx = ((isin(trot.vy) * icos(trot.vx)) >> 12) << 2;
                    //sphere.velocity.vy = ((isin(trot.vy) * icos(trot.vx)) >> 12) << 2;
                    //sphere.velocity.vz = -((icos(trot.vy) * icos(trot.vx)) >> 12) << 2;
                    // Move backward
                    //cam_pos.vx += ((isin(trot.vy) * icos(trot.vx)) >> 12) << 2;
                    //cam_pos.vy -= isin(trot.vx) << 2;
                    //cam_pos.vz -= ((icos(trot.vy) * icos(trot.vx)) >> 12) << 2;
                }



                if (!(pad->btn & PAD_R1))
                {

                    // Slide up
                    cam_pos.vx -= ((isin(trot.vy) * isin(trot.vx)) >> 12) << 2;
                    cam_pos.vy -= icos(trot.vx) << 2;
                    cam_pos.vz += ((icos(trot.vy) * isin(trot.vx)) >> 12) << 2;
                }

                if (!(pad->btn & PAD_R2))
                {

                    // Slide down
                    cam_pos.vx += ((isin(trot.vy) * isin(trot.vx)) >> 12) << 2;
                    cam_pos.vy += icos(trot.vx) << 2;
                    cam_pos.vz -= ((icos(trot.vy) * isin(trot.vx)) >> 12) << 2;
                }

                // Look at cube
                if (!(pad->btn & PAD_L1))
                {

                    cam_mode = 1;
                }
            }

        }
        
        
        cam_pos.vx = ((playerBody->joints[1].position.x<<10));
        cam_pos.vy = (-(playerBody->joints[1].position.y<<10));
        cam_pos.vz = ((playerBody->joints[1].position.z<<10));
        

        

        //Sphere sphere = {{0, 1, 0}, {0, 0, 0}, {0, GRAVITY, 0}, 0.5, 1};
        
        FntPrint(-1, "FPS=%d\n",
                 fps);
        // Print out some info
        FntPrint(-1, "BUTTONS=%04x\n", pad->btn);
        FntPrint(-1, "X=%d Y=%d Z=%d\n",
                 cam_pos.vx>>2,
                 cam_pos.vy>>2,
                 cam_pos.vz>>2);
        FntPrint(-1, "RX=%d RY=%d\n",
                 cam_rot.vx >> 12,
                 cam_rot.vy >> 12);

        // First-person camera mode
        if (cam_mode == 0)
        {

            // Set rotation to the matrix
            RotMatrix(&trot, &mtx);

            // Divide out the fractions of camera coordinates and invert
            // the sign, so camera coordinates will line up to world
            // (or geometry) coordinates
            tpos.vx = -cam_pos.vx >> 12;
            tpos.vy = -cam_pos.vy >> 12;
            tpos.vz = -cam_pos.vz >> 12;

            // Apply rotation of matrix to translation value to achieve a
            // first person perspective
            ApplyMatrixLV(&mtx, &tpos, &tpos);

            // Set translation matrix
            TransMatrix(&mtx, &tpos);

            // Tracking mode
        }
        else
        {

            // Vector that defines the 'up' direction of the camera
            SVECTOR up = {0, -ONE, 0};

            // Divide out fractions of camera coordinates
            tpos.vx = cam_pos.vx >> 12;
            tpos.vy = cam_pos.vy >> 12;
            tpos.vz = cam_pos.vz >> 12;

            // Look at the cube
            LookAt(&tpos, &pos, &up, &mtx);
        }

        // Set rotation and translation matrix
        gte_SetRotMatrix(&mtx);
        gte_SetTransMatrix(&mtx);

        // Draw the floor

        

        // Update nextpri variable (very important)
        //db_nextpri = (char *)pol4;
        VECTOR position = {0,0,0};
        SVECTOR treeRot{0,0,0};
        
        //draw_tree(&mtx, &position, &treeRot);
        draw_mybox(&mtx, &position, &treeRot);
        //cam_pos = sphere.position;
        //draw_floor(&mtx, &position, &treeRot);
        // Position the cube going around the floor bouncily
        setVector(&pos,
                  isin(rot.vy) >> 4,
                  -300 + (isin(rot.vy << 2) >> 5),
                  icos(rot.vy) >> 3);

        // Sort cube
        

        // Make the cube SPEEN
        rot.vx += 8;
        rot.vy += 8;

        // Flush text to drawing area
        FntFlush(-1);

        // Swap buffers and draw the primitives
        graphics->display();
    }

    return 0;
}


void compute_normal(const SVECTOR triangle[3], SVECTOR& normal)
{
    // Compute the cross product of two edges of the triangle
    int vx1 = triangle[1].vx - triangle[0].vx;
    int vy1 = triangle[1].vy - triangle[0].vy;
    int vz1 = triangle[1].vz - triangle[0].vz;
    int vx2 = triangle[2].vx - triangle[0].vx;
    int vy2 = triangle[2].vy - triangle[0].vy;
    int vz2 = triangle[2].vz - triangle[0].vz;
    int nx = (vy1 * vz2) - (vz1 * vy2);
    int ny = (vz1 * vx2) - (vx1 * vz2);
    int nz = (vx1 * vy2) - (vy1 * vx2);

    // Normalize the result
    int length = SquareRoot0((nx * nx) + (ny * ny) + (nz * nz));
    normal.vx = (nx << 12) / length;
    normal.vy = (ny << 12) / length;
    normal.vz = (nz << 12) / length;
}


template<typename GeometryType, typename T, bool semiTransparent = false>
void render3DModel(const T& model, TIM_IMAGE* texture)
{
    const auto tri_size = sizeof(model.tris)/sizeof(oblivion::face3);
    for(int tri = 0; tri < tri_size; ++tri)
    {
        const auto triIndex = model.tris[tri];
        const SVECTOR triangle[3] = {
            model.vertices[triIndex.vertice0],
            model.vertices[triIndex.vertice1],
            model.vertices[triIndex.vertice2]
        };

        const SVECTOR normal = model.normals[triIndex.normal0];
		const DVECTOR uvs[] = {model.uvs[triIndex.uv1], model.uvs[triIndex.uv0], model.uvs[triIndex.uv2]};
        const CVECTOR colors[] = {model.colors[triIndex.color1],model.colors[triIndex.color0], model.colors[triIndex.color2]};
        graphics->Draw<POLY_GT3>(triangle, normal, texture, uvs, colors);
    }
    const auto quad_size = sizeof(model.quads)/sizeof(oblivion::face4);
    for(int index = 0; index < quad_size; ++index)
    {
        const auto triIndex = model.quads[index];
        const SVECTOR quad[4] = {
            model.vertices[triIndex.vertice1],
            model.vertices[triIndex.vertice0],
            model.vertices[triIndex.vertice2],
            model.vertices[triIndex.vertice3]
        };

        const SVECTOR normal = model.normals[triIndex.normal0];
        const DVECTOR uvs[] = {model.uvs[triIndex.uv1], model.uvs[triIndex.uv0], model.uvs[triIndex.uv2], model.uvs[triIndex.uv3]};
        const CVECTOR colors[] = {model.colors[triIndex.color1],model.colors[triIndex.color0], model.colors[triIndex.color2], model.colors[triIndex.color3]};
        graphics->Draw<GeometryType>(quad, normal, texture, uvs, colors);
    }
}

void draw_mybox(MATRIX *mtx, VECTOR *pos, SVECTOR *rot)
{

    int i, p;
    POLY_F4 *pol4;

    // Object and light matrix for object
    MATRIX omtx, lmtx;

    // Set object rotation and position
    RotMatrix(rot, &omtx);
    TransMatrix(&omtx, pos);

    // Multiply light matrix to object matrix
    MulMatrix0(&light_mtx, &omtx, &lmtx);

    // Set result to GTE light matrix
    gte_SetLightMatrix(&lmtx);

    // Composite coordinate matrix transform, so object will be rotated and
    // positioned relative to camera matrix (mtx), so it'll appear as
    // world-space relative.
    CompMatrixLV(mtx, &omtx, &omtx);

    // Save matrix
    PushMatrix();

    // Set matrices
    gte_SetRotMatrix(&omtx);
    gte_SetTransMatrix(&omtx);

    //constexpr oblivion mybox;
    render3DModel<POLY_GT4>(oblivion{}, &textures_lvl1_texture);
    render3DModel<POLY_GT4>(lightshaft{}, &light_shaft_texture);
    // Restore matrix
    PopMatrix();
}

