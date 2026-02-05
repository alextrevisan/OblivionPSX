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

int32_t gte_dot_product_psn00bsdk_restricted(VECTOR vec_s0_12, SVECTOR vec_s16_0) {
    unsigned long v0xy_packed;
    long v0z_val;

    // Load the S16.0 vector (vec_s16_0) into GTE vector register V0.
    // V0X and V0Y are packed into one long for gte_ldv0XY_reg.
    // GTE data register 0 (cop2r0, VXY0): VX0 in lower 16 bits, VY0 in upper 16 bits.
    // GTE data register 1 (cop2r1, VZ0): VZ0.
    // These macros are from gtemisc.h.
    v0xy_packed = ( ( (unsigned long)( (unsigned short)vec_s16_0.vy ) ) << 16) |
                  (unsigned long)( (unsigned short)vec_s16_0.vx );
    v0z_val = (long)vec_s16_0.vz;

   // gte_ldv0(&vec_s16_0);
    gte_ldv0XY_reg(v0xy_packed); // Load V0X and V0Y components
    gte_ldv0Z_reg(v0z_val);      // Load V0Z component

    // Load the S0.12 vector (vec_s0_12) components into GTE IR1, IR2, IR3 registers.
    // These macros are from gtemisc.h and load a long (short is sign-extended).
    gte_ldir1((long)vec_s0_12.vx); // IR1 = vec_s0_12->vx (S0.12)
    gte_ldir2((long)vec_s0_12.vy); // IR2 = vec_s0_12->vy (S0.12)
    gte_ldir3((long)vec_s0_12.vz); // IR3 = vec_s0_12->vz (S0.12)

    // Execute the Normal Color Single (NCS) GTE command.
    // gte_ncs() is used by macros in gtemac.h (e.g., gte_NormalColor) and is assumed
    // to be a defined primitive GTE command macro in the PSn00bSDK environment.
    // The NCS command (opcode 0x1E, typically 0x018001E) calculates:
    // MAC1 = (IR1*V0.vx + IR2*V0.vy + IR3*V0.vz)
    // With shift factor (sf) = 0, the result S0.12 * S16.0 = S16.12 is stored in MAC1.
    gte_ncs();

    // Retrieve the dot product result from GTE register MAC1.
    // gte_getmac1() is from gtemisc.h and returns a long.
    // The result is in S16.12 format.
    return gte_getmac1();
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