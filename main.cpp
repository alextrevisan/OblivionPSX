#include <sys/types.h>
#include <stdio.h>
#include <psxgpu.h>
#include <psxgte.h>
#include <psxpad.h>
#include <psxapi.h>
#include <psxetc.h>
#include <inline_c.h>
#include <gtemac.h>
#include "engine/NavMesh.h"

#include "meshs/navmesh0_navmesh.h"

#include "engine/collision/CollisionSolver.h"
#include "engine/collision/PlaneObject.h"

#include "scenario/meshs/oblivion.h"
#include "scenario/meshs/oblivion2.h"
#include "scenario/meshs/skeleton.h"
//#include "scenario/meshs/skeleton2.h"
#include "scenario/meshs/lightshaft.h"
#include "scenario/meshs/torch.h"
#include "scenario/meshs/fire.h"
#include "scenario/meshs/plane.h"
#include "scenario/scene01.h"
#include "clip.h"
#include "engine/lookat.h"
#include "engine/fast_draw_functions.h"

#include "engine/TextureManager.h"

//#define FASTMEM __attribute__((section(".fastmem")))


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


extern uint32_t light_shaft_tim[];
extern uint32_t textures_lvl1_tim[];
extern uint32_t skeleton_tim[];
extern uint32_t fire_tim[];

TIM_IMAGE light_shaft_texture;
TIM_IMAGE textures_lvl1_texture;
TIM_IMAGE skeleton_texture;
TIM_IMAGE fire_texture;
scene01 *scene;
int PositionScale = 0;

uint32_t* getScratchAddr(uint32_t offset = 0)
{
    return (uint32_t*)0x1F800000;
}
// Pad data buffer

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

//generate cube_verts2 with twice the height


//generate cube_verts2 with twice the height


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

// generate pyramid vertices


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


// Function declarations
#include <GraphicsV2.h>

void draw_tree(MATRIX *mtx, VECTOR *pos, SVECTOR *rot);
void draw_mybox(MATRIX *mtx, VECTOR *pos, SVECTOR *rot, VECTOR* cameraPos);
void draw_floor(MATRIX *mtx, VECTOR *pos, SVECTOR *rot);
void renderPlane(const Vector3& position, const FixedPoint& sizeX, const FixedPoint& sizeZ, const Vector3& rotation);
Graphics *graphics;
SVECTOR verts[17][17]; // Vertex array for floor
int px, py;


PhysicsEnginePSX::PlaneObject ground(
    Vector3(0,0,0),
    600,0,-200,
    1600<<3,
    800<<3);

PhysicsEnginePSX::PlaneObject walls[] = {
{Vector3(0, 0, FixedPoint::FromFixedPoint(-32768)), 1400, -400, -200, 800, 800},
{Vector3(FixedPoint::FromFixedPoint(32768), 0, 0), 300, -400, 200, 600, 800},
{Vector3(FixedPoint::FromFixedPoint(32768), 0, 0), 1300, -400, 200, 200, 800},
{Vector3(FixedPoint::FromFixedPoint(32768), 0, 0), 900, -400, 400, 600, 800},
{Vector3(0, 0, FixedPoint::FromFixedPoint(32768)), 600, -400, 300, 800, 200},
{Vector3(FixedPoint::FromFixedPoint(0), FixedPoint::FromFixedPoint(0), FixedPoint::FromFixedPoint(-32768)), 1200, -400, 300, 800, 200},
};


PhysicsEnginePSX::SphereObject player(Vector3(FixedPoint{1073},FixedPoint{-320},FixedPoint{-255}), FixedPoint{40});

FixedPoint playerJump = FixedPoint::Zero();
Vector3 moveSpeed = { FixedPoint(0), FixedPoint(0), FixedPoint(0) };
VECTOR cam_pos; // Camera position (in fixed point integers)
bool onGround = false;
const FixedPoint gravity = FixedPoint::FromFixedPoint(401);
void ProcessPhysics()
{
    player.Acceleration[1] += gravity;
    player.Velocity[1] += playerJump + moveSpeed[1];
    playerJump = FixedPoint::Zero();

    player.Velocity[0] += player.Acceleration[0];
    player.Velocity[1] += player.Acceleration[1];
    player.Velocity[2] += player.Acceleration[2];
    player.Position[0] += player.Velocity[0] + moveSpeed[0];  
    player.Position[1] += player.Velocity[1] + moveSpeed[1];  
    player.Position[2] += player.Velocity[2] + moveSpeed[2];  

    //onGround = PhysicsEnginePSX::CollisionSolver::ResolveSpherePlaneCollisions(player, wall);
    for(auto& wall : walls)
    {
        onGround |= PhysicsEnginePSX::CollisionSolver::ResolveSpherePlaneCollisions(player, wall);
    }
    onGround |= PhysicsEnginePSX::CollisionSolver::ResolveSpherePlaneCollisions(player, ground);
    cam_pos.vx = player.Position[0].AsFixedPoint();
    cam_pos.vy = player.Position[1].AsFixedPoint() - (280<<12);
    cam_pos.vz = player.Position[2].AsFixedPoint();
}


int main()
{
    int frame = 0;

    //TPE_bodyMoveBy(playerBody,VECTOR(0,(ONE>>2) + 3072,0));
    //TPE_bodyRotateByAxis(&world.bodies[0],VECTOR(0,0,TPE_F / 4));
    


    int i, p, xy_temp;
    

    SVECTOR rot; // Rotation vector for cube
    VECTOR pos;     // Position vector for cube

    

    
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
    TextureManager::LoadTexture(skeleton_tim, skeleton_texture);
    TextureManager::LoadTexture(light_shaft_tim, light_shaft_texture);
    TextureManager::LoadTexture(fire_tim, fire_texture);

    scene = new scene01(graphics, &textures_lvl1_texture, &skeleton_texture, &light_shaft_texture, &fire_texture);
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

    setVector(&cam_pos, 50000, -822272, ONE * -0);
    setVector(&cam_rot, 0, 1024, 0);

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


        
        //TPE_Unit height = TPE_bodyGetCenterOfMass(playerBody).vy;
        moveSpeed.x = FixedPoint::Zero();
        moveSpeed.z = FixedPoint::Zero();

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

                    if(onGround)
                        playerJump = FixedPoint(-1);
                    
                    //FntPrint(-1, "pulando!\n");
                    //if(onGround)
                    //    PositionScale++;

                }
                if (!(pad->btn & PAD_SQUARE))
                {
                    //playerJump = FixedPoint(-10);
                    //if(onGround)
                    //    PositionScale--;
                }
                FixedPoint playerSpeed = FixedPoint(8);
                // Movement controls
                if (!(pad->btn & PAD_TRIANGLE))
                {

                    moveSpeed.x = playerSpeed * FixedPoint::FromFixedPoint(-(isin(trot.vy)));
                    moveSpeed.z = playerSpeed * FixedPoint::FromFixedPoint(icos(trot.vy));
                }
                else if (!(pad->btn & PAD_CROSS))
                {
                    moveSpeed.x = playerSpeed * FixedPoint::FromFixedPoint((isin(trot.vy)));
                    moveSpeed.z = playerSpeed * FixedPoint::FromFixedPoint(-icos(trot.vy));
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
        
        constexpr auto navmesh = navmesh0_navmesh{};
        player.Position[0] += moveSpeed[0];  
        player.Position[1] += moveSpeed[1];  
        player.Position[2] += moveSpeed[2];
        
        //cam_pos.vx = player.Position[0].AsFixedPoint();
        //cam_pos.vy = player.Position[1].AsFixedPoint();
        //cam_pos.vz = player.Position[2].AsFixedPoint();
        
        //ProcessPhysics();
        
        //cam_pos = {cam_pos.vx >> 12, cam_pos.vy >> 12, cam_pos.vz >> 12};
        //printf("cam_pos: %d %d %d\r\n", cam_pos.vx, cam_pos.vy, cam_pos.vz);
        //printf("cam_pos Antes: %d %d %d\r\n", cam_pos.vx, cam_pos.vy, cam_pos.vz);
        VECTOR input = {player.Position[0].AsInt(), player.Position[1].AsInt(), player.Position[2].AsInt()};
        auto res = ComputeNavmeshPosition(input, navmesh, -280);
        //printf("cam_pos depois: %d %d %d\r\n", cam_pos.vx, cam_pos.vy, cam_pos.vz);
        player.Position[0] = res.vx;
        player.Position[1] = res.vy;
        player.Position[2] = res.vz;
        
        cam_pos.vx = player.Position[0].AsFixedPoint();
        cam_pos.vy = player.Position[1].AsFixedPoint();
        cam_pos.vz = player.Position[2].AsFixedPoint();

        
        //Sphere sphere = {{0, 1, 0}, {0, 0, 0}, {0, GRAVITY, 0}, 0.5, 1};
        
        FntPrint(-1, "FPS=%d\n",
                 fps);
        // Print out some info
        /*FntPrint(-1, "BUTTONS=%04x\n", pad->btn);
        FntPrint(-1, "X=%d Y=%d Z=%d\n",
                 cam_pos.vx>>12,
                 cam_pos.vy>>12,
                 cam_pos.vz>>12);*/
        FntPrint(-1, "CIRCLE TO JUMP!");
        /*FntPrint(-1, "RX=%d RY=%d\n",
                 cam_rot.vx >> 12,
                 cam_rot.vy >> 12);*/

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
            //printf("tpos Antes: %d %d %d\r\n", tpos.vx, tpos.vy, tpos.vz);
            //tpos = ComputeNavmeshPosition<navmesh0_navmesh>(tpos, navmesh, FixedPoint(-280));
            //printf("tpos depois: %d %d %d\r\n", tpos.vx, tpos.vy, tpos.vz);

            //tpos = {-tpos.vx, -tpos.vy, -tpos.vz};
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
        

		setFrustumPlanes(&mainFrustum, cam_pos, &mtx, 0, 2000);
        
        //draw_tree(&mtx, &position, &treeRot);
        scene->Render(&mtx, &cam_pos);
        draw_mybox(&mtx, &position, &treeRot, &cam_pos);

        /*for(auto triIndex : navmesh.triangles)
        {
            const SVECTOR triangle[3] = {
                {navmesh.vertices[triIndex.vertice0].vx, navmesh.vertices[triIndex.vertice0].vy, navmesh.vertices[triIndex.vertice0].vz},
                {navmesh.vertices[triIndex.vertice1].vx, navmesh.vertices[triIndex.vertice1].vy, navmesh.vertices[triIndex.vertice1].vz},
                {navmesh.vertices[triIndex.vertice2].vx, navmesh.vertices[triIndex.vertice2].vy, navmesh.vertices[triIndex.vertice2].vz}
            };
            const CVECTOR colors[] = {128,128,128};
            const DVECTOR uvs[] = {0,0,0};
            graphics->Draw<POLY_F3>(triangle, {0, -4095,0}, nullptr, uvs, colors, false, 0, 0, false);
        }*/
        //for(int i = 8; i < COLLIDER_SIZE; i++)
        //{
        //    draw_collision(&mtx, &treeRot, colliders[i]);
        //}
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



template<typename T, bool semiTransparent = false>
void render3DModel(const T& model, TIM_IMAGE* texture)
{
    for(auto triIndex : model.tris)
    {
        const SVECTOR triangle[3] = {
            model.vertices[triIndex.vertice0],
            model.vertices[triIndex.vertice1],
            model.vertices[triIndex.vertice2]
        };

        const SVECTOR normal = model.normals[triIndex.normal0];
		const DVECTOR uvs[] = {model.uvs[triIndex.uv1], model.uvs[triIndex.uv0], model.uvs[triIndex.uv2]};
        const CVECTOR colors[] = {model.colors[triIndex.color1],model.colors[triIndex.color0], model.colors[triIndex.color2]};
        graphics->Draw<POLY_GT3>(triangle, normal, texture, uvs, colors, false, 0, 0, semiTransparent);
    }

    for(auto quadIndex : model.quads)
    {
        const SVECTOR quad[4] = {
            model.vertices[quadIndex.vertice1],
            model.vertices[quadIndex.vertice0],
            model.vertices[quadIndex.vertice2],
            model.vertices[quadIndex.vertice3]
        };

        const SVECTOR normal = model.normals[quadIndex.normal0];
        const DVECTOR uvs[] = {model.uvs[quadIndex.uv1], model.uvs[quadIndex.uv0], model.uvs[quadIndex.uv2], model.uvs[quadIndex.uv3]};
        const CVECTOR colors[] = {model.colors[quadIndex.color1],model.colors[quadIndex.color0], model.colors[quadIndex.color2], model.colors[quadIndex.color3]};
        
        graphics->Draw<POLY_GT4>(quad, normal, texture, uvs, colors, false, 0 ,0, semiTransparent);
    }
}

//create a function to multiply VECTOR with SVECTOR
static constexpr SVECTOR operator*(const VECTOR& v, const SVECTOR& s)
{
    return SVECTOR(v.vx * s.vx, v.vy * s.vy, v.vz * s.vz);
}


MATRIX omtx, lmtx;
POLY_F4 *pol4;

void draw_mybox(MATRIX *mtx, VECTOR *pos, SVECTOR *rot, VECTOR* cameraPos)
{

    int i, p;
    

    // Object and light matrix for object
    

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
    

    // Set matrices
    gte_SetRotMatrix(&omtx);
    gte_SetTransMatrix(&omtx);

    //constexpr oblivion mybox;
    //render3DModel<>(oblivion{}, &textures_lvl1_texture);
    //render3DModel<>(oblivion2{}, &textures_lvl1_texture);
    //render3DModel<lightshaft, true>(lightshaft{}, &light_shaft_texture);
    //render3DModel<>(torch{}, &textures_lvl1_texture);

    //render3DModel<>(skeleton{}, &skeleton_texture);
    //render3DModelBillboard<>(fire{}, &fire_texture, &cam_pos);
    //render3DModel<POLY_GT4>(skeleton2{}, &skeleton_texture);
    //renderPlane(ground.Position,ground.SizeX, ground.SizeZ, {});
    /*for(auto& wall : walls)
    {
        renderPlane(wall.Position,wall.MaxX - wall.MinX, wall.MaxZ - wall.MinZ, wall.Rotation);
        FntPrint(-1, "x=%d | z=%d\n", (wall.MaxX - wall.MinX).AsInt(), (wall.MaxZ - wall.MinZ).AsInt());
    }*/
    //render3DModel<POLY_GT4>(lightshaft{}, &light_shaft_texture);
    //drawAABox(collider);
    // Restore matrix
    
}

void renderPlane(const Vector3& position, const FixedPoint& sizeX, const FixedPoint& sizeZ, const Vector3& rotation)
{
    
    MATRIX mtx;
    VECTOR pos{position.x.AsInt(), position.y.AsInt() , position.z.AsInt()};
    
    
    SVECTOR rot{rotation.x.AsFixedPoint()>>5, rotation.y.AsFixedPoint()>>5, rotation.z.AsFixedPoint()>>5};
    
    //>>8 because of the size of the plane (256x256)
    VECTOR scale{sizeX.AsFixedPoint()>>8, 4096, sizeZ.AsFixedPoint()>>8};

    RotMatrix(&rot, &mtx);
    ScaleMatrix(&mtx, &scale);    
    TransMatrix(&mtx, &pos);

    CompMatrixLV(&omtx, &mtx, &mtx);

    gte_SetRotMatrix(&mtx);
    gte_SetTransMatrix(&mtx);


    const plane model;
    for(auto quadIndex : model.quads)
    {
        const SVECTOR quad[4] = {
            model.vertices[quadIndex.vertice1],
            model.vertices[quadIndex.vertice0],
            model.vertices[quadIndex.vertice2],
            model.vertices[quadIndex.vertice3]
        };

        const SVECTOR normal = model.normals[quadIndex.normal0];
        const DVECTOR uvs[] = {model.uvs[quadIndex.uv1], model.uvs[quadIndex.uv0], model.uvs[quadIndex.uv2], model.uvs[quadIndex.uv3]};
        const CVECTOR colors[] = {model.colors[quadIndex.color1],model.colors[quadIndex.color0], model.colors[quadIndex.color2], model.colors[quadIndex.color3]};
        
        graphics->Draw<POLY_F4>(quad, normal, nullptr, uvs, colors);
    }
    PopMatrix();
}

