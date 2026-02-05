#ifndef _SCENE_RENDERER_H_
#define _SCENE_RENDERER_H_

#include "inline_c.h"
#include "GraphicsV2.h"
#include "Frustum.h"

static FRUSTUM mainFrustum;

#define COPY_COLOR(src, dest) *(uint32_t*)dest = *(uint32_t*)src;
#define COPY_POS(src, dest) *(uint32_t*)dest = *(uint32_t*)src;
#define COPY_UV(src, dest) *(uint16_t*)dest = *(uint16_t*)src;

enum BlendMode
{
    Alpha50,
    Aditive,
    Subtractive,
    Subtractive50
};

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
        graphics->Draw<POLY_GT3, false>(triangle, normal, texture, uvs, colors, false, 0, 0, semiTransparent);
    }

    constexpr int P_DIST_SUBDIVIDE = 400;
    auto orderingTable = graphics->GetOrderingTable();
    auto next_primitive = (POLY_GT4 *)graphics->NextPrimitive();
    auto tpage = getTPage(texture->mode, semiTransparent ? BlendMode::Aditive : BlendMode::Alpha50, texture->prect->x, texture->prect->y);
    auto clut = getClut(texture->crect->x, texture->crect->y);
    CVECTOR out[4];
    int p;
    SVECTOR screenPts[9];
    for(auto quadIndex : model.quads)
    {
        const CVECTOR* c0 = &model.colors[quadIndex.color0];
        const CVECTOR* c1 = &model.colors[quadIndex.color1];
        const CVECTOR* c2 = &model.colors[quadIndex.color2];
        const CVECTOR* c3 = &model.colors[quadIndex.color3];
        const DVECTOR* uv0 = &model.uvs[quadIndex.uv0];
        const DVECTOR* uv1 = &model.uvs[quadIndex.uv1];
        const DVECTOR* uv2 = &model.uvs[quadIndex.uv2];
        const DVECTOR* uv3 = &model.uvs[quadIndex.uv3];
        const SVECTOR* v0 = &model.vertices[quadIndex.vertice0];
        const SVECTOR* v1 = &model.vertices[quadIndex.vertice1];
        const SVECTOR* v2 = &model.vertices[quadIndex.vertice2];
        const SVECTOR* v3 = &model.vertices[quadIndex.vertice3];
        
        gte_ldv3_f(*v1, *v0, *v2);
        gte_rtpt_b();
        gte_nclip_b();
        gte_stopz_m(p);
        if(p <= 0)
            continue;
        gte_avsz3_b();
        gte_stotz_m(p);
        if (p <= 0 || p >= OT_LEN)
            continue;
        gte_stsxy0(&screenPts[0].vx);
        gte_stsxy1(&screenPts[1].vx);
        gte_stsxy2(&screenPts[2].vx);
        
        int dist = p<<2;
        if (p > P_DIST_SUBDIVIDE)
        {
            setPolyGT4(next_primitive);
            setSemiTrans(next_primitive, semiTransparent ? BlendMode::Aditive : BlendMode::Alpha50);
            next_primitive->x0 = screenPts[0].vx;
            next_primitive->y0 = screenPts[0].vy;
            next_primitive->x1 = screenPts[1].vx;
            next_primitive->y1 = screenPts[1].vy;
            next_primitive->x2 = screenPts[2].vx;
            next_primitive->y2 = screenPts[2].vy;
            gte_ldv0_f(*v3);
            gte_rtps_b();
            gte_stsxy(&screenPts[3].vx);
            next_primitive->x3 = screenPts[3].vx;
            next_primitive->y3 = screenPts[3].vy;
            
            gte_lddp(dist);
            gte_ldrgb3(c1, c0, c2);
            gte_dpct_b();
            gte_strgb3(&out[0], &out[1], &out[2]);
            setRGB0(next_primitive, out[0].r, out[0].g, out[0].b);
            setRGB1(next_primitive, out[1].r, out[1].g, out[1].b);
            setRGB2(next_primitive, out[2].r, out[2].g, out[2].b);

            gte_ldrgb(c3);
            gte_dpcs_b();
            gte_strgb(&out[3]);
            setRGB3(next_primitive, out[3].r, out[3].g, out[3].b);
            
            next_primitive->tpage = tpage;
            next_primitive->clut = clut;
            
            next_primitive->u0 = uv1->vx;
            next_primitive->v0 = uv1->vy;
            next_primitive->u1 = uv0->vx;
            next_primitive->v1 = uv0->vy;
            next_primitive->u2 = uv2->vx;
            next_primitive->v2 = uv2->vy;
            next_primitive->u3 = uv3->vx;
            next_primitive->v3 = uv3->vy;
            
            addPrim(orderingTable + p, next_primitive);
            next_primitive++;
            continue;
        }
        // 0-----1       0--4--1
        // |     |       |  |  |
        // |     |  -->  5--8--6  + filler at the edges to fix gaps
        // |     |       |  |  |
        // 2-----3       2--7--3
        POLY_GT4* quad_0458 = next_primitive++;
        POLY_GT4* quad_4186 = next_primitive++;
        POLY_GT4* quad_5827 = next_primitive++;
        POLY_GT4* quad_8673 = next_primitive++;
        
        POLY_GT3* tri_014 = (POLY_GT3*)next_primitive;    next_primitive = (POLY_GT4*)((uint8_t*)next_primitive + sizeof(POLY_GT3));
        POLY_GT3* tri_052 = (POLY_GT3*)next_primitive;    next_primitive = (POLY_GT4*)((uint8_t*)next_primitive + sizeof(POLY_GT3));
        POLY_GT3* tri_136 = (POLY_GT3*)next_primitive;    next_primitive = (POLY_GT4*)((uint8_t*)next_primitive + sizeof(POLY_GT3));
        POLY_GT3* tri_273 = (POLY_GT3*)next_primitive;    next_primitive = (POLY_GT4*)((uint8_t*)next_primitive + sizeof(POLY_GT3));

        gte_ldir0(2048);
        const SVECTOR v4 = midpoint(*v1, *v0);
        const SVECTOR v5 = midpoint(*v1, *v2);
        const SVECTOR v6 = midpoint(*v0, *v3);
        const SVECTOR v7 = midpoint(*v2, *v3);
        const SVECTOR v8 = midpoint(v5, v6);

        const DVECTOR uv4 = midpoint_uv(*uv1, *uv0);
        const DVECTOR uv5 = midpoint_uv(*uv1, *uv2);
        const DVECTOR uv6 = midpoint_uv(*uv0, *uv3);
        const DVECTOR uv7 = midpoint_uv(*uv2, *uv3);
        const DVECTOR uv8 = midpoint_uv(uv5, uv6);

        const CVECTOR color4 = midpoint_color(*c1, *c0);
        const CVECTOR color5 = midpoint_color(*c1, *c2);
        const CVECTOR color6 = midpoint_color(*c0, *c3);
        const CVECTOR color7 = midpoint_color(*c2, *c3);
        const CVECTOR color8 = midpoint_color(color5, color6);

        CVECTOR out_colors[9];
        gte_lddp(dist);
        gte_ldrgb3(c1, c0, c2);
        gte_dpct_b();
        gte_strgb3(&out_colors[0], &out_colors[1], &out_colors[2]);
        
        gte_ldrgb(c3);
        gte_dpcs_b();
        gte_strgb(&out_colors[3]);
        
        gte_ldrgb3(&color4, &color5, &color6);
        gte_dpct_b();
        gte_strgb3(&out_colors[4], &out_colors[5], &out_colors[6]);
        
        gte_ldrgb(&color7);
        gte_dpcs_b();
        gte_strgb(&out_colors[7]);
        
        gte_ldrgb(&color8);
        gte_dpcs_b();
        gte_strgb(&out_colors[8]);

        gte_ldv3(&v4, &v5, &v8);
        gte_rtpt();
        gte_stsxy0(&screenPts[4].vx);
        gte_stsxy1(&screenPts[5].vx);
        gte_stsxy2(&screenPts[8].vx);
        gte_ldv3(v3, &v6, &v7);
        gte_rtpt();
        gte_stsxy0(&screenPts[3].vx);
        gte_stsxy1(&screenPts[6].vx);
        gte_stsxy2(&screenPts[7].vx);
        
        setXY4(quad_0458, screenPts[0].vx, screenPts[0].vy, screenPts[4].vx, screenPts[4].vy, screenPts[5].vx, screenPts[5].vy, screenPts[8].vx, screenPts[8].vy);
        setXY4(quad_4186, screenPts[4].vx, screenPts[4].vy, screenPts[1].vx, screenPts[1].vy, screenPts[8].vx, screenPts[8].vy, screenPts[6].vx, screenPts[6].vy);
        setXY4(quad_5827, screenPts[5].vx, screenPts[5].vy, screenPts[8].vx, screenPts[8].vy, screenPts[2].vx, screenPts[2].vy, screenPts[7].vx, screenPts[7].vy);
        setXY4(quad_8673, screenPts[8].vx, screenPts[8].vy, screenPts[6].vx, screenPts[6].vy, screenPts[7].vx, screenPts[7].vy, screenPts[3].vx, screenPts[3].vy);

        POLY_GT4* quads[4] = {quad_0458, quad_4186, quad_5827, quad_8673};
        for(int i = 0; i < 4; i++) {
            setPolyGT4(quads[i]);
            setSemiTrans(quads[i], semiTransparent ? BlendMode::Aditive : BlendMode::Alpha50);
            quads[i]->clut = clut;
            quads[i]->tpage = tpage;
        }

        quad_0458->u0 = uv1->vx;
        quad_0458->v0 = uv1->vy;
        quad_0458->u1 = uv4.vx;
        quad_0458->v1 = uv4.vy;
        quad_0458->u2 = uv5.vx;
        quad_0458->v2 = uv5.vy;
        quad_0458->u3 = uv8.vx;
        quad_0458->v3 = uv8.vy;

        quad_4186->u0 = uv4.vx;
        quad_4186->v0 = uv4.vy;
        quad_4186->u1 = uv0->vx;
        quad_4186->v1 = uv0->vy;
        quad_4186->u2 = uv8.vx;
        quad_4186->v2 = uv8.vy;
        quad_4186->u3 = uv6.vx;
        quad_4186->v3 = uv6.vy;

        quad_5827->u0 = uv5.vx;
        quad_5827->v0 = uv5.vy;
        quad_5827->u1 = uv8.vx;
        quad_5827->v1 = uv8.vy;
        quad_5827->u2 = uv2->vx;
        quad_5827->v2 = uv2->vy;
        quad_5827->u3 = uv7.vx;
        quad_5827->v3 = uv7.vy;

        quad_8673->u0 = uv8.vx;
        quad_8673->v0 = uv8.vy;
        quad_8673->u1 = uv6.vx;
        quad_8673->v1 = uv6.vy;
        quad_8673->u2 = uv7.vx;
        quad_8673->v2 = uv7.vy;
        quad_8673->u3 = uv3->vx;
        quad_8673->v3 = uv3->vy;

        setRGB0(quad_0458, out_colors[0].r, out_colors[0].g, out_colors[0].b);
        setRGB1(quad_0458, out_colors[4].r, out_colors[4].g, out_colors[4].b);
        setRGB2(quad_0458, out_colors[5].r, out_colors[5].g, out_colors[5].b);
        setRGB3(quad_0458, out_colors[8].r, out_colors[8].g, out_colors[8].b);

        setRGB0(quad_4186, out_colors[4].r, out_colors[4].g, out_colors[4].b);
        setRGB1(quad_4186, out_colors[1].r, out_colors[1].g, out_colors[1].b);
        setRGB2(quad_4186, out_colors[8].r, out_colors[8].g, out_colors[8].b);
        setRGB3(quad_4186, out_colors[6].r, out_colors[6].g, out_colors[6].b);

        setRGB0(quad_5827, out_colors[5].r, out_colors[5].g, out_colors[5].b);
        setRGB1(quad_5827, out_colors[8].r, out_colors[8].g, out_colors[8].b);
        setRGB2(quad_5827, out_colors[2].r, out_colors[2].g, out_colors[2].b);
        setRGB3(quad_5827, out_colors[7].r, out_colors[7].g, out_colors[7].b);

        setRGB0(quad_8673, out_colors[8].r, out_colors[8].g, out_colors[8].b);
        setRGB1(quad_8673, out_colors[6].r, out_colors[6].g, out_colors[6].b);
        setRGB2(quad_8673, out_colors[7].r, out_colors[7].g, out_colors[7].b);
        setRGB3(quad_8673, out_colors[3].r, out_colors[3].g, out_colors[3].b);

        addPrim(orderingTable + p, quad_0458);
        addPrim(orderingTable + p, quad_4186);
        addPrim(orderingTable + p, quad_5827);
        addPrim(orderingTable + p, quad_8673);
        
        setXY3(tri_014, screenPts[0].vx, screenPts[0].vy, screenPts[1].vx, screenPts[1].vy, screenPts[4].vx, screenPts[4].vy);
        setXY3(tri_052, screenPts[0].vx, screenPts[0].vy, screenPts[5].vx, screenPts[5].vy, screenPts[2].vx, screenPts[2].vy);
        setXY3(tri_136, screenPts[1].vx, screenPts[1].vy, screenPts[3].vx, screenPts[3].vy, screenPts[6].vx, screenPts[6].vy);
        setXY3(tri_273, screenPts[2].vx, screenPts[2].vy, screenPts[7].vx, screenPts[7].vy, screenPts[3].vx, screenPts[3].vy);
        
        POLY_GT3* tris[4] = {tri_014, tri_052, tri_136, tri_273};
        for(int i = 0; i < 4; i++) {
            setPolyGT3(tris[i]);
            setSemiTrans(tris[i], semiTransparent ? BlendMode::Aditive : BlendMode::Alpha50);
            tris[i]->clut = clut;
            tris[i]->tpage = tpage;
        }
        
        tri_014->u0 = uv1->vx; tri_014->v0 = uv1->vy;
        tri_014->u1 = uv0->vx; tri_014->v1 = uv0->vy;
        tri_014->u2 = uv4.vx;  tri_014->v2 = uv4.vy;
        
        tri_052->u0 = uv1->vx; tri_052->v0 = uv1->vy;
        tri_052->u1 = uv5.vx;  tri_052->v1 = uv5.vy;
        tri_052->u2 = uv2->vx; tri_052->v2 = uv2->vy;
        
        tri_136->u0 = uv0->vx; tri_136->v0 = uv0->vy;
        tri_136->u1 = uv3->vx; tri_136->v1 = uv3->vy;
        tri_136->u2 = uv6.vx;  tri_136->v2 = uv6.vy;
        
        tri_273->u0 = uv2->vx; tri_273->v0 = uv2->vy;
        tri_273->u1 = uv7.vx;  tri_273->v1 = uv7.vy;
        tri_273->u2 = uv3->vx; tri_273->v2 = uv3->vy;
        
        setRGB0(tri_014, out_colors[0].r, out_colors[0].g, out_colors[0].b);
        setRGB1(tri_014, out_colors[1].r, out_colors[1].g, out_colors[1].b);
        setRGB2(tri_014, out_colors[4].r, out_colors[4].g, out_colors[4].b);
        
        setRGB0(tri_052, out_colors[0].r, out_colors[0].g, out_colors[0].b);
        setRGB1(tri_052, out_colors[5].r, out_colors[5].g, out_colors[5].b);
        setRGB2(tri_052, out_colors[2].r, out_colors[2].g, out_colors[2].b);
        
        setRGB0(tri_136, out_colors[1].r, out_colors[1].g, out_colors[1].b);
        setRGB1(tri_136, out_colors[3].r, out_colors[3].g, out_colors[3].b);
        setRGB2(tri_136, out_colors[6].r, out_colors[6].g, out_colors[6].b);
        
        setRGB0(tri_273, out_colors[2].r, out_colors[2].g, out_colors[2].b);
        setRGB1(tri_273, out_colors[7].r, out_colors[7].g, out_colors[7].b);
        setRGB2(tri_273, out_colors[3].r, out_colors[3].g, out_colors[3].b);
        
        addPrim(orderingTable + p, tri_014);
        addPrim(orderingTable + p, tri_052);
        addPrim(orderingTable + p, tri_136);
        addPrim(orderingTable + p, tri_273);
    }
    graphics->NextPrimitive((uint8_t*)next_primitive);
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
