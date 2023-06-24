#ifndef __MDLLOADER_H__
#define __MDLLOADER_H__

#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <array>

#include "mdl/mdl.h"

class MDLLoader
{
public:
    MDLLoader(const fs::path& path, const fs::path& output, const int verticeScale, const int textureSizeU, const int textureSizeV, const int normalScale)
    {
        mdl_model_t mdl;
        std::ifstream file(path, std::ios::in | std::ios::binary);
        if (file.is_open())
        {
            file.read((char*)&mdl.header, sizeof(mdl_header_t));
            //std::cout<<"Read "<<file.gcount()<<" bytes\n";
            if ((mdl.header.ident != 1330660425) ||
                    (mdl.header.version != 6))
            {
                /* Error! */
                fprintf (stderr, "Error: bad version or identifier\n");
                file.close();
                return;
            }

            /* Memory allocations */
            mdl.skins = new mdl_skin_t[mdl.header.num_skins];
            mdl.texcoords = new mdl_texcoord_t[mdl.header.num_verts];
            mdl.triangles = new mdl_triangle_t[mdl.header.num_tris];
            mdl.frames = new mdl_frame_t[mdl.header.num_frames];
            mdl.tex_id = new uint8_t[mdl.header.num_skins];

            mdl.iskin = 0;

            /* Read texture data */
            for (int i = 0; i < mdl.header.num_skins; ++i)
            {
                mdl.skins[i].data = new uint8_t[mdl.header.skinwidth * mdl.header.skinheight];
                file.read(reinterpret_cast<char*>(&mdl.skins[i].group), sizeof(int));
                file.read(reinterpret_cast<char*>(mdl.skins[i].data), sizeof (uint8_t) * mdl.header.skinwidth * mdl.header.skinheight);

                //mdl.tex_id[i] = MakeTextureFromSkin (i, mdl);
                //free (mdl.skins[i].data);
                //mdl.skins[i].data = NULL;
            }

            file.read(reinterpret_cast<char*>(mdl.texcoords), sizeof(struct mdl_texcoord_t) * mdl.header.num_verts);
            //fread (mdl.texcoords, sizeof (struct mdl_texcoord_t), mdl.header.num_verts, fp);
            file.read(reinterpret_cast<char*>(mdl.triangles), sizeof(struct mdl_triangle_t) * mdl.header.num_tris);
            //std::cout<<"Read "<<file.gcount()<<" bytes\n";
            //fread (mdl.triangles, sizeof (struct mdl_triangle_t), mdl.header.num_tris, fp);
            /*for(int i = 0; i < mdl.header.num_tris;++i)
            {
                const auto textCoord = mdl.texcoords[i];
                const auto triangles = mdl.triangles[i];
                std::cout<<"S="<<textCoord.s<<" T="<<textCoord.t<<'\n';
                std::cout<<"V0="<<triangles.vertex[0]<<" V1="<<triangles.vertex[1]<<" V2="<<triangles.vertex[2]<<'\n';
            }*/
            for (int i = 0; i < mdl.header.num_frames; ++i)
            {

                /* Memory allocation for vertices of this frame */
                mdl.frames[i].frame.verts = new mdl_vertex_t[sizeof (struct mdl_vertex_t) * mdl.header.num_verts];

                /* Read frame data */
                file.read(reinterpret_cast<char*>(&mdl.frames[i].type), sizeof (int));

                file.read(reinterpret_cast<char*>(&mdl.frames[i].frame.bboxmin),sizeof (struct mdl_vertex_t));

                file.read(reinterpret_cast<char*>(&mdl.frames[i].frame.bboxmax),sizeof (struct mdl_vertex_t));

                file.read(mdl.frames[i].frame.name, sizeof (char)*16);
                printf("Name: %s\n", mdl.frames[i].frame.name);
                file.read(reinterpret_cast<char*>(mdl.frames[i].frame.verts), sizeof (struct mdl_vertex_t)*mdl.header.num_verts);
            }
        }
    }

    ~MDLLoader(){};
protected:
private:
};



#endif // __MDLLOADER_H__
