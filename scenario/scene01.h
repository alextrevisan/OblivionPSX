#ifndef _SCENE01_H_
#define _SCENE01_H_

#include "scene_renderer.h"

#include "meshs/oblivion.h"
#include "meshs/oblivion2.h"
#include "meshs/skeleton.h"
#include "meshs/lightshaft.h"
#include "meshs/torch.h"
#include "meshs/fire.h"
#include "meshs/level01secretdoor.h"
#include "meshs/table.h"
#include "meshs/cavern.h"
class scene01
{
public:
    scene01(Graphics* graphics, TIM_IMAGE* textures_lvl1_texture, TIM_IMAGE* skeleton_texture, TIM_IMAGE* light_shaft_texture, TIM_IMAGE* fire_texture)
    :graphics(graphics), textures_lvl1_texture(textures_lvl1_texture), skeleton_texture(skeleton_texture), light_shaft_texture(light_shaft_texture), fire_texture(fire_texture)
    {
        
    }

    void Render(MATRIX* cameraMatrix, VECTOR* cam_pos)
    {
        render3DModel(graphics, cameraMatrix, oblivionModel, textures_lvl1_texture);
        render3DModel(graphics, cameraMatrix, oblivion2Model, textures_lvl1_texture);
        render3DModel(graphics, cameraMatrix, skeletonModel, skeleton_texture);
        render3DModel<lightshaft, true>(graphics, cameraMatrix, lightshaftModel, light_shaft_texture);
        render3DModel(graphics, cameraMatrix, torchModel, textures_lvl1_texture);
        //render3DModel(graphics, cameraMatrix, secret_doorModel, textures_lvl1_texture);
        render3DModel(graphics, cameraMatrix, tableModel, textures_lvl1_texture);
        render3DModel(graphics, cameraMatrix, cavernModel, textures_lvl1_texture);
        render3DModelBillboard<fire>(graphics, cameraMatrix, fireModel, fire_texture, cam_pos);        
    }
private:
    Graphics* graphics;

    TIM_IMAGE* textures_lvl1_texture;
    TIM_IMAGE* skeleton_texture;
    TIM_IMAGE* light_shaft_texture;
    TIM_IMAGE* fire_texture;

    oblivion oblivionModel;
    oblivion2 oblivion2Model;
    skeleton skeletonModel;
    lightshaft lightshaftModel;
    level01secretdoor secret_doorModel;
    torch torchModel;
    fire fireModel;
    table tableModel;
    cavern cavernModel;
};

#endif // _SCENE01_H_
