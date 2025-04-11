// LookAt matrix code (may be implemented into libpsxgte soon)
#include "inline_c.h"
#include "lookat.h"
#define gte_OuterProduct12(r1,r2,r3)				\
				{	gte_ldopv1(r1);		\
					gte_ldopv2(r2);		\
					gte_op12();		\
					gte_stlvnl(r3); }

#define gte_OuterProduct0(r1,r2,r3)				\
				{	gte_ldopv1(r1);		\
					gte_ldopv2(r2);		\
					gte_op0();		\
					gte_stlvnl(r3); }

void crossProduct(SVECTOR *v0, SVECTOR *v1, VECTOR *out) {
	//gte_OuterProduct12(v0, v1, out);
	//return;
	out->vx = ((v0->vy*v1->vz)-(v0->vz*v1->vy))>>12;
	out->vy = ((v0->vz*v1->vx)-(v0->vx*v1->vz))>>12;
	out->vz = ((v0->vx*v1->vy)-(v0->vy*v1->vx))>>12;

}

void LookAt(VECTOR *eye, VECTOR *at, SVECTOR *up, MATRIX *mtx) {

	VECTOR taxis;
	SVECTOR zaxis;
	SVECTOR xaxis;
	SVECTOR yaxis;
	VECTOR pos;
	VECTOR vec;

	setVector(&taxis, at->vx-eye->vx, at->vy-eye->vy, at->vz-eye->vz);
	VectorNormalS(&taxis, &zaxis);
    crossProduct(&zaxis, up, &taxis);
	VectorNormalS(&taxis, &xaxis);
	crossProduct(&zaxis, &xaxis, &taxis);
	VectorNormalS(&taxis, &yaxis);

	mtx->m[0][0] = xaxis.vx;	mtx->m[1][0] = yaxis.vx;	mtx->m[2][0] = zaxis.vx;
	mtx->m[0][1] = xaxis.vy;	mtx->m[1][1] = yaxis.vy;	mtx->m[2][1] = zaxis.vy;
	mtx->m[0][2] = xaxis.vz;	mtx->m[1][2] = yaxis.vz;	mtx->m[2][2] = zaxis.vz;

	pos.vx = -eye->vx;;
	pos.vy = -eye->vy;;
	pos.vz = -eye->vz;;

	ApplyMatrixLV(mtx, &pos, &vec);
	TransMatrix(mtx, &vec);
	
}