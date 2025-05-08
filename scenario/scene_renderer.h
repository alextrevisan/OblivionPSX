#ifndef _SCENE_RENDERER_H_
#define _SCENE_RENDERER_H_

#include "inline_c.h"
#include "GraphicsV2.h"
#include "Frustum.h"

static FRUSTUM mainFrustum;

template<typename T, bool semiTransparent = false>
void render3DModel(Graphics* graphics, MATRIX* cameraMatrix, const T& model, TIM_IMAGE* texture, bool test = false)
{
    MATRIX omtx;
    VECTOR pos = model.position;
    SVECTOR rot = model.rotation;
    TransMatrix(&omtx, &pos);
    RotMatrix(&rot, &omtx);
    CompMatrixLV(cameraMatrix, &omtx, &omtx);
    gte_SetTransMatrix(&omtx);
    gte_SetRotMatrix(&omtx);

    total_count++;
    VECTOR aabb_min = {model.aabb_min.vx + model.position.vx, model.aabb_min.vy + model.position.vy, model.aabb_min.vz + model.position.vz};
    VECTOR aabb_max = {model.aabb_max.vx + model.position.vx, model.aabb_max.vy + model.position.vy, model.aabb_max.vz + model.position.vz};
    if(!isAABBInFrustum(&mainFrustum, aabb_min, aabb_max))
    {
        culling_count++;
        return;
    }
    
    

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

    constexpr int P_DIST_SUBDIVIDE = 125;
    auto orderingTable = graphics->GetOrderingTable();
    auto next_primitive = (POLY_GT4 *)graphics->NextPrimitive();
    auto tpage = getTPage(texture->mode, 0, texture->prect->x, texture->prect->y);
    auto clut = getClut(texture->crect->x, texture->crect->y);
    CVECTOR out[4];
    for(auto quadIndex : model.quads)
    {
        /*const SVECTOR quad[4] = {
            model.vertices[quadIndex.vertice1],
            model.vertices[quadIndex.vertice0],
            model.vertices[quadIndex.vertice2],
            model.vertices[quadIndex.vertice3]
        };*/

        //const SVECTOR normal = model.normals[quadIndex.normal0];
        //const DVECTOR uvs[] = {model.uvs[quadIndex.uv1], model.uvs[quadIndex.uv0], model.uvs[quadIndex.uv2], model.uvs[quadIndex.uv3]};
        //const CVECTOR colors[] = {model.colors[quadIndex.color1],model.colors[quadIndex.color0], model.colors[quadIndex.color2], model.colors[quadIndex.color3]};
        
        //graphics->Draw<POLY_GT4>(quad, normal, texture, uvs, colors, false, 0 ,0, semiTransparent);
        //continue;
        gte_ldv3_f(model.vertices[quadIndex.vertice1], model.vertices[quadIndex.vertice0], model.vertices[quadIndex.vertice2]);
        gte_rtpt_b();
        gte_nclip_b();
        int p;
        gte_stopz_m(p);
        if(p <= 0)
            continue;
        gte_avsz3_b();
        gte_stotz_m(p);
        if (p <= 0 || p >= OT_LEN)
            continue;
    
        setPolyGT4(next_primitive);
        gte_stsxy3_gt4(next_primitive);
        gte_ldv0_f(model.vertices[quadIndex.vertice3]);
        gte_rtps_b();
        gte_stsxy(&next_primitive->x3);
        int dist = p<<2;
        //gte_DpqColor3(&model.colors[quadIndex.color1], &model.colors[quadIndex.color0], &model.colors[quadIndex.color2], dist, &out[0], &out[1], &out[2]);
        gte_ldrgb3(&model.colors[quadIndex.color1], &model.colors[quadIndex.color0], &model.colors[quadIndex.color2]);
        gte_lddp(dist);
        gte_dpct_b();
        gte_strgb3(&out[0], &out[1], &out[2]);
        setRGB0(next_primitive, out[0].r, out[0].g, out[0].b);
        setRGB1(next_primitive, out[1].r, out[1].g, out[1].b);
        setRGB2(next_primitive, out[2].r, out[2].g, out[2].b);

        gte_DpqColor(&model.colors[quadIndex.color3], dist, &out[3]);
        setRGB3(next_primitive, out[3].r, out[3].g, out[3].b);

        next_primitive->tpage = tpage;
        next_primitive->clut = clut;
        
        next_primitive->u0 = model.uvs[quadIndex.uv1].vx ;
        next_primitive->v0 = model.uvs[quadIndex.uv1].vy;
        next_primitive->u1 = model.uvs[quadIndex.uv0].vx;
        next_primitive->v1 = model.uvs[quadIndex.uv0].vy;
        next_primitive->u2 = model.uvs[quadIndex.uv2].vx;
        next_primitive->v2 = model.uvs[quadIndex.uv2].vy;
        next_primitive->u3 = model.uvs[quadIndex.uv3].vx;
        next_primitive->v3 = model.uvs[quadIndex.uv3].vy;
        
        addPrim(orderingTable + p, next_primitive);
        next_primitive++;
    }
    graphics->NextPrimitive((uint8_t*)next_primitive);
    
    // 0-----1       0--4--1
    // |     |       |  |  |
    // |     |  -->  5--8--6  + filler at the edges to fix gaps
    // |     |       |  |  |
    // 2-----3       2--7--3
    
}

int uv_index = 0;
int uv_fps = 0;
template<typename T, bool semiTransparent = false>
void render3DModelBillboard(Graphics* graphics, MATRIX* cameraMatrix, const T& model, TIM_IMAGE* texture, VECTOR* cameraPos)
{
    MATRIX omtx;
    VECTOR pos = {0, 0, 0};
    SVECTOR rot = {0, 0, 0};
    TransMatrix(&omtx, &pos);
    RotMatrix(&rot, &omtx);
    CompMatrixLV(cameraMatrix, &omtx, &omtx);
    gte_SetTransMatrix(&omtx);
    gte_SetRotMatrix(&omtx);

    const CVECTOR colors[] = {model.colors[1],model.colors[0], model.colors[2], model.colors[3]};
    uv_fps++;
    if(uv_fps > 10)
    {
        uv_fps = 0;
        uv_index++;
    }

    graphics->DrawBillboard(model.vertices[0], texture, model.uvs, uv_index%model.max_frame_index+1);
}

#endif // _SCENE_RENDERER_H_
