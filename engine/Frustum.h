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

VECTOR calculateForwardVector(const SVECTOR& rot) {
    VECTOR forward = {-(( isin( rot.vy )*icos( rot.vx ) )>>12), isin( -rot.vx ), (( icos( rot.vx )*icos( rot.vy ) )>>12)};
    SVECTOR out;
    VectorNormalS(&forward, &out);
    return {out.vx, out.vy, out.vz};
}

VECTOR calculateRightVector(const SVECTOR& rot) {
    VECTOR right = {icos( rot.vy ), 0, isin( rot.vy )};
    SVECTOR out;
    VectorNormalS(&right, &out);
    return {out.vx, out.vy, out.vz};
}

void setFrustumPlanes(FRUSTUM* frustum, VECTOR cam_pos, VECTOR forward, VECTOR right, int nearDist, int farDist) {
    frustum->position = { cam_pos.vx, cam_pos.vy, cam_pos.vz };
    
    frustum->direction = forward;

    frustum->farPlane.normal = { -forward.vx, 0, -forward.vz };
    frustum->farPlane.d = farDist;

    frustum->nearPlane.normal = { forward.vx, 0, forward.vz };
    frustum->nearPlane.d = nearDist;
    
    SVECTOR rotAngle = {0, -512, 0};
    MATRIX rotMatrix;
    RotMatrix(&rotAngle, &rotMatrix);
    
    VECTOR negRight = {-right.vx, -right.vy, -right.vz};
    VECTOR rotatedRight;
    ApplyMatrixLV(&rotMatrix, &negRight, &rotatedRight);
    
    SVECTOR out;
    VectorNormalS(&rotatedRight, &out);
    frustum->leftPlane.normal = { out.vx, out.vy, out.vz };
    
    frustum->leftPlane.d = (frustum->leftPlane.normal.vx * frustum->position.vx + 
                           frustum->leftPlane.normal.vy * frustum->position.vy + 
                           frustum->leftPlane.normal.vz * frustum->position.vz)>>12;

    
    rotAngle = {0, 512, 0};
    rotMatrix = {};
    RotMatrix(&rotAngle, &rotMatrix);
    
    VECTOR posRight = {right.vx, right.vy, right.vz};

    ApplyMatrixLV(&rotMatrix, &posRight, &rotatedRight);
    
    VectorNormalS(&rotatedRight, &out);

    frustum->rightPlane.normal = { out.vx, out.vy, out.vz };
    frustum->rightPlane.d = (frustum->rightPlane.normal.vx * frustum->position.vx + 
                            frustum->rightPlane.normal.vy * frustum->position.vy + 
                            frustum->rightPlane.normal.vz * frustum->position.vz)>>12;
}

int isPointInFrustum(FRUSTUM* frustum, SVECTOR point) {
    SVECTOR relPoint;
    relPoint.vx = point.vx - (frustum->position.vx>>12);
    relPoint.vy = point.vy - (frustum->position.vy>>12);
    relPoint.vz = point.vz - (frustum->position.vz>>12);
    
    int nearDistance = (frustum->nearPlane.normal.vx * relPoint.vx + 
                   frustum->nearPlane.normal.vz * relPoint.vz + 
                   frustum->nearPlane.d);

    int farDistance = (frustum->farPlane.normal.vx * relPoint.vx + 
                  frustum->farPlane.normal.vz * relPoint.vz + 
                  frustum->farPlane.d);
    
    int leftDistance = (frustum->leftPlane.normal.vx * relPoint.vx + 
                   frustum->leftPlane.normal.vz * relPoint.vz + 
                   frustum->leftPlane.d);
    
    int rightDistance = (frustum->rightPlane.normal.vx * relPoint.vx + 
                    frustum->rightPlane.normal.vz * relPoint.vz + 
                    frustum->rightPlane.d);
    
    return (nearDistance >= 0) && (farDistance <= 0) && (leftDistance <= 0) && (rightDistance <= 0);
}

int isAABBInFrustum(FRUSTUM* frustum, VECTOR min, VECTOR max) {
    SVECTOR corners[8];
    
    // Verificar se qualquer um dos cantos da caixa está dentro do frustum
    int inside = 0;
    
    // Pontos relativos à posição da câmera
    SVECTOR relMin, relMax;
    relMin.vx = min.vx - (frustum->position.vx>>12);
    relMin.vy = min.vy - (frustum->position.vy>>12);
    relMin.vz = min.vz - (frustum->position.vz>>12);
    
    relMax.vx = max.vx - (frustum->position.vx>>12);
    relMax.vy = max.vy - (frustum->position.vy>>12);
    relMax.vz = max.vz - (frustum->position.vz>>12);
    
    // Verificar cada plano do frustum
    // Plano Near
    if ((frustum->nearPlane.normal.vx * relMin.vx + frustum->nearPlane.normal.vz * relMin.vz + frustum->nearPlane.d < 0) &&
        (frustum->nearPlane.normal.vx * relMax.vx + frustum->nearPlane.normal.vz * relMax.vz + frustum->nearPlane.d < 0)) {
        return 0; // Completamente atrás do plano near
    }
    
    // Plano Far
    if ((frustum->farPlane.normal.vx * relMin.vx + frustum->farPlane.normal.vz * relMin.vz + frustum->farPlane.d > 0) &&
        (frustum->farPlane.normal.vx * relMax.vx + frustum->farPlane.normal.vz * relMax.vz + frustum->farPlane.d > 0)) {
        return 0; // Completamente além do plano far
    }
    
    // Plano Left
    if ((frustum->leftPlane.normal.vx * relMin.vx + frustum->leftPlane.normal.vz * relMin.vz + frustum->leftPlane.d > 0) &&
        (frustum->leftPlane.normal.vx * relMax.vx + frustum->leftPlane.normal.vz * relMax.vz + frustum->leftPlane.d > 0)) {
        return 0; // Completamente à direita do plano esquerdo
    }
    
    // Plano Right
    if ((frustum->rightPlane.normal.vx * relMin.vx + frustum->rightPlane.normal.vz * relMin.vz + frustum->rightPlane.d > 0) &&
        (frustum->rightPlane.normal.vx * relMax.vx + frustum->rightPlane.normal.vz * relMax.vz + frustum->rightPlane.d > 0)) {
        return 0; // Completamente à esquerda do plano direito
    }
    
    // Se chegou até aqui, ao menos parte da AABB está dentro do frustum
    return 1;
}

#endif