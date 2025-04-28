#ifndef _SCENE_RENDERER_H_
#define _SCENE_RENDERER_H_

#include "inline_c.h"
#include "GraphicsV2.h"
#include "Frustum.h"

static FRUSTUM mainFrustum;

template<typename T, bool semiTransparent = false>
void render3DModel(Graphics* graphics, MATRIX* cameraMatrix, const T& model, TIM_IMAGE* texture)
{
    MATRIX omtx;
    VECTOR pos = model.position;
    SVECTOR rot = model.rotation;
    TransMatrix(&omtx, &pos);
    RotMatrix(&rot, &omtx);
    CompMatrixLV(cameraMatrix, &omtx, &omtx);
    gte_SetTransMatrix(&omtx);
    gte_SetRotMatrix(&omtx);

    if(!isAABBInFrustum(&mainFrustum, model.aabb_min, model.aabb_max))
    {
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
