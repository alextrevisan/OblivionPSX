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
#include "meshs/cavern2.h"
#include "meshs/cavern3.h"
#include "meshs/cavern4.h"
#include "meshs/torch2.h"
#include "meshs/fire2.h"
#include "meshs/plane.h"

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
        render3DModel(graphics, cameraMatrix, torch2Model, textures_lvl1_texture);
        //render3DModel(graphics, cameraMatrix, secret_doorModel, textures_lvl1_texture);
        render3DModel(graphics, cameraMatrix, tableModel, textures_lvl1_texture);
        render3DModel(graphics, cameraMatrix, cavernModel, textures_lvl1_texture);
        render3DModel(graphics, cameraMatrix, cavern2Model, textures_lvl1_texture);
        render3DModel(graphics, cameraMatrix, cavern3Model, textures_lvl1_texture);
        render3DModel(graphics, cameraMatrix, cavern4Model, textures_lvl1_texture, true);
        render3DModelBillboard<fire>(graphics, cameraMatrix, fireModel, fire_texture, cam_pos);
        render3DModelBillboard<fire2>(graphics, cameraMatrix, fire2Model, fire_texture, cam_pos);        
        //render3DModel(graphics, cameraMatrix, planeModel, textures_lvl1_texture);
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
    torch2 torch2Model;
    fire fireModel;
    fire2 fire2Model;
    table tableModel;
    cavern cavernModel;
    cavern2 cavern2Model;
    cavern3 cavern3Model;
    cavern4 cavern4Model;
    plane planeModel;
};

#endif // _SCENE01_H_
