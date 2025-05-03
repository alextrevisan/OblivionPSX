#ifndef _FRUSTUM_H
#define _FRUSTUM_H

#include <psxgte.h>
#include <inline_c.h>

typedef struct {
    VECTOR normal;
    int d;
} PLANE;

typedef struct {
    PLANE nearPlane;
    PLANE farPlane;
    PLANE leftPlane;
    PLANE rightPlane;
    VECTOR position;
    VECTOR direction;
} FRUSTUM;


void setFrustumPlanes(FRUSTUM* frustum, VECTOR cam_pos, const MATRIX* camMatrix, int nearDist, int farDist) {
    frustum->position = {cam_pos.vx>>12, cam_pos.vy>>12, cam_pos.vz>>12};

    // Pega os vetores diretamente da matrix
    VECTOR forward = {camMatrix->m[2][0], camMatrix->m[2][1], camMatrix->m[2][2]};
    VECTOR right   = {camMatrix->m[0][0], camMatrix->m[0][1], camMatrix->m[0][2]};

    frustum->direction = forward;
    frustum->farPlane.normal = { -forward.vx, -forward.vy, -forward.vz };
    frustum->farPlane.d = farDist << 12;

    frustum->nearPlane.normal = forward;
    frustum->nearPlane.d = nearDist << 12;

    MATRIX rotMatrix;
    VECTOR rotated;
    SVECTOR out;

    SVECTOR rotAngle = {0, 512, 0};
    RotMatrix(&rotAngle, &rotMatrix);    
    auto left = VECTOR{-right.vx, -right.vy, -right.vz};
    ApplyMatrixLV(&rotMatrix, &left, &rotated);
    VectorNormalS(&rotated, &out);
    frustum->leftPlane.normal = { out.vx, out.vy, out.vz };
    frustum->leftPlane.d = (out.vx * cam_pos.vx + 
                            out.vy * cam_pos.vy + 
                            out.vz * cam_pos.vz) >> 12;

    rotAngle = {0, -512, 0};
    RotMatrix(&rotAngle, &rotMatrix);
    ApplyMatrixLV(&rotMatrix, &right, &rotated);
    VectorNormalS(&rotated, &out);

    frustum->rightPlane.normal = { out.vx, out.vy, out.vz };
    frustum->rightPlane.d = (out.vx * cam_pos.vx + 
                             out.vy * cam_pos.vy + 
                             out.vz * cam_pos.vz) >> 12;
}
static int culling_count = 0;
static int total_count = 0;

int32_t dotProduct(VECTOR normal, SVECTOR point) {
    return normal.vx * point.vx + 
           normal.vy * point.vy + 
           normal.vz * point.vz;
}

int isPointInFrustum(FRUSTUM* frustum, SVECTOR point) {
    point.vx -= frustum->position.vx;
    point.vy -= frustum->position.vy;
    point.vz -= frustum->position.vz;
    
    int farDistance = dotProduct(frustum->farPlane.normal, point) + frustum->farPlane.d;
    if(farDistance < 0) return false;
    int nearDistance = dotProduct(frustum->nearPlane.normal, point) - frustum->nearPlane.d;
    if(nearDistance < 0) return false;
    int leftDistance = dotProduct(frustum->leftPlane.normal, point);
    if(leftDistance < 0) return false;
    int rightDistance = dotProduct(frustum->rightPlane.normal, point);
    if(rightDistance < 0) return false;
    
    return true;
}

int isAABBInFrustum(FRUSTUM* frustum, VECTOR min, VECTOR max) {
    // Transformar AABB para coordenadas relativas ao frustum
    SVECTOR relMin, relMax;
    relMin.vx = min.vx - frustum->position.vx;
    relMin.vy = min.vy - frustum->position.vy;
    relMin.vz = min.vz - frustum->position.vz;
    
    relMax.vx = max.vx - frustum->position.vx;
    relMax.vy = max.vy - frustum->position.vy;
    relMax.vz = max.vz - frustum->position.vz;
    
    // Testar cada plano do frustum contra a AABB
    // Para cada plano, encontramos o ponto mais próximo e o mais distante da AABB
    // em relação ao plano, e testamos se esses pontos estão do lado positivo ou negativo
    //Teste com o plane Near
    SVECTOR p_near;
    // Escolha o ponto da AABB mais distante ao longo da normal do plano Near
    p_near.vx = (frustum->nearPlane.normal.vx > 0) ? relMax.vx : relMin.vx;
    p_near.vy = (frustum->nearPlane.normal.vy > 0) ? relMax.vy : relMin.vy;
    p_near.vz = (frustum->nearPlane.normal.vz > 0) ? relMax.vz : relMin.vz;
    if (dotProduct(frustum->nearPlane.normal, p_near) - frustum->nearPlane.d < 0) {
        return 0; // AABB está completamente além do plano Near
    }
    // Teste com o plano Far
    SVECTOR p_far;
    // Escolha o ponto da AABB mais distante ao longo da normal do plano Far
    p_far.vx = (frustum->farPlane.normal.vx > 0) ? relMax.vx : relMin.vx;
    p_far.vy = (frustum->farPlane.normal.vy > 0) ? relMax.vy : relMin.vy;
    p_far.vz = (frustum->farPlane.normal.vz > 0) ? relMax.vz : relMin.vz;
    if (dotProduct(frustum->farPlane.normal, p_far) + frustum->farPlane.d < 0) {
        return 0; // AABB está completamente além do plano Far
    }
    
    // Teste com o plano Left
    SVECTOR p_left;
    p_left.vx = (frustum->leftPlane.normal.vx > 0) ? relMax.vx : relMin.vx;
    p_left.vy = (frustum->leftPlane.normal.vy > 0) ? relMax.vy : relMin.vy;
    p_left.vz = (frustum->leftPlane.normal.vz > 0) ? relMax.vz : relMin.vz;
    if (dotProduct(frustum->leftPlane.normal, p_left) < 0) {
        return 0; // AABB está completamente além do plano Left
    }
    
    // Teste com o plano Right
    SVECTOR p_right;
    p_right.vx = (frustum->rightPlane.normal.vx > 0) ? relMax.vx : relMin.vx;
    p_right.vy = (frustum->rightPlane.normal.vy > 0) ? relMax.vy : relMin.vy;
    p_right.vz = (frustum->rightPlane.normal.vz > 0) ? relMax.vz : relMin.vz;
    if (dotProduct(frustum->rightPlane.normal, p_right) < 0) {
        return 0; // AABB está completamente além do plano Right
    }
    
    // Se passar em todos os testes, a AABB está pelo menos parcialmente dentro do frustum
    return 1;
}

#endif