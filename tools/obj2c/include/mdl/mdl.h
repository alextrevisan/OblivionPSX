#ifndef __MDL_H__
#define __MDL_H__

/* Vector */
typedef float vec3_t[3];

/* MDL header */
struct mdl_header_t
{
    int ident;            /* magic number: "IDPO" */
    int version;          /* version: 6 */

    vec3_t scale;         /* scale factor */
    vec3_t translate;     /* translation vector */
    float boundingradius;
    vec3_t eyeposition;   /* eyes' position */

    int num_skins;        /* number of textures */
    int skinwidth;        /* texture width */
    int skinheight;       /* texture height */

    int num_verts;        /* number of vertices */
    int num_tris;         /* number of triangles */
    int num_frames;       /* number of frames */

    int synctype;         /* 0 = synchron, 1 = random */
    int flags;            /* state flag */
    float size;
};

/* Skin */
struct mdl_skin_t
{
    int group;      /* 0 = single, 1 = group */
    uint8_t *data;  /* texture data */
};

/* Texture coords */
struct mdl_texcoord_t
{
    int onseam;
    int s;
    int t;
};

/* Triangle info */
struct mdl_triangle_t
{
    int facesfront;  /* 0 = backface, 1 = frontface */
    int vertex[3];   /* vertex indices */
};

/* Compressed vertex */
struct mdl_vertex_t
{
    unsigned char v[3];
    unsigned char normalIndex;
};

/* Simple frame */
struct mdl_simpleframe_t
{
    struct mdl_vertex_t bboxmin; /* bouding box min */
    struct mdl_vertex_t bboxmax; /* bouding box max */
    char name[16];
    struct mdl_vertex_t *verts;  /* vertex list of the frame */
};

/* Model frame */
struct mdl_frame_t
{
    int type;                        /* 0 = simple, !0 = group */
    struct mdl_simpleframe_t frame;  /* this program can't read models
				      composed of group frames! */
};

/* MDL model structure */
struct mdl_model_t
{
    struct mdl_header_t header;

    struct mdl_skin_t *skins;
    struct mdl_texcoord_t *texcoords;
    struct mdl_triangle_t *triangles;
    struct mdl_frame_t *frames;

    uint8_t *tex_id;
    int iskin;
};


#endif // __MDL_H__
