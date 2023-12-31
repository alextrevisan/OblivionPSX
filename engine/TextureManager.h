#ifndef _TEXTURE_MANAGER_H_
#define _TEXTURE_MANAGER_H_

#include <psxgpu.h>

class TextureManager
{
public:
    static void LoadTexture(const uint32_t* bytes, TIM_IMAGE &texture)
    {
        GetTimInfo(bytes, &texture);
            
        LoadImage(texture.prect, texture.paddr);
        DrawSync(0);
        if(texture.mode & 0x8)
        {
            LoadImage(texture.crect, texture.caddr);
            DrawSync(0);
        }
    }
};



#endif //_TEXTURE_MANAGER_H_