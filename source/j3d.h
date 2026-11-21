/*
 * j3d.h - renderer for the early J3D models ("J3D1bmd1") of the GameCube IPL
 *
 * This mirrors the IPL's own small model renderer (BS2 "gMR*" code):
 * pre-order traversal of the INF1 hierarchy, MAT1 materials applied with the
 * same GX calls in the same order, shapes drawn from their display lists
 * with one position matrix each, env-map texture matrices recomputed per shape,
 * and the "model alpha" fade implemented as an extra TEV stage.
 */
#ifndef J3D_H
#define J3D_H

#include <gccore.h>

#define J3D_MAX_TEX 8

typedef struct {
	u8  type;        /* 0x10 joint, 0x11 material, 0x12 shape */
	u16 index;
	s16 child;       /* first child node or -1 */
	s16 next;        /* next sibling or -1 */
} J3DNode;

typedef struct {
	u8  mtx_type;    /* 0 = normal, 1 = billboard */
	u16 ndesc;
	const u8 *desc;  /* {u32 attr, u32 type} list */
	u16 drw;         /* DRW1 entry used for the position matrix */
	void *dl;        /* display list (32-byte aligned) */
	u32 dl_size;
} J3DShape;

typedef struct {
	const u8 *e;     /* the 0x98-byte MAT1 entry */
	u8  flag;
} J3DMat;

typedef struct {
	float scale[3];
	s16   rot[3];
	float trans[3];
} J3DJoint;

typedef struct J3DModel {
	u8 *data;
	u32 size;
	const u8 *inf1, *vtx1, *drw1, *jnt1, *shp1, *mat1, *tex1;

	J3DNode *nodes;
	int nnodes;

	int nvfmt;
	struct { u32 attr, cnt, type; u8 frac; } vfmt[16];
	void *varray[32];
	u8 vstride[32];

	int njoints;
	J3DJoint *joints;      /* rest pose (JNT1) */
	u16 *drw_index;
	int ndrw;

	int nshapes;
	J3DShape *shapes;

	int nmats;
	J3DMat *mats;
	const u8 *mpool[22];   /* MAT1 pools */

	int ntex;
	GXTexObj texobj[J3D_MAX_TEX];
	u8 tex_custom[J3D_MAX_TEX];  /* texture replaced by the user (custom text) */
} J3DModel;

/* per-draw state of a model instance */
typedef struct {
	J3DModel *model;
	Mtx base;              /* view * model matrix */
	Mtx *jmtx;             /* joint matrices (njoints) */
	int alpha;             /* model alpha 0..255 (255 = opaque) */
	const GXColor *matcolor0;   /* optional per-material colour 0 override (IPK1) */
	const GXColorS10 (*tevcolor)[4]; /* optional per-material TEV colour override (customisation) */
	const u8 (*tevcolor_set)[4];     /* which of the 4 TEV colours are overridden */
	int tex_color_from_ras;          /* textured "TEXC" stages take the material colour instead (custom text colour) */
	u32 shape_hide;                  /* shapes 0..31 not drawn by j3d_draw (custom letters) */
} J3DInstance;

int  j3d_load(J3DModel *m, u8 *data, u32 size);
void j3d_free(J3DModel *m);

/* replace texture i with custom image data (GX format, w x h), keeps wrap/filter */
void j3d_set_texture(J3DModel *m, int i, void *img, u16 w, u16 h, u8 fmt);
/* raw TEX1 header of texture i (BTI layout) */
const u8 *j3d_tex_header(J3DModel *m, int i);

/* MAT1 TEV colour k (0..2) of material mi; returns 0 if the material has none */
int j3d_get_tevcolor(J3DModel *m, int mi, int k, GXColorS10 *out);

/* compute joint matrices from the JNT1 rest pose (static models) */
void j3d_calc_rest(J3DInstance *inst);
/* compute joint matrices from per-joint local matrices (animated models) */
void j3d_calc_local(J3DInstance *inst, Mtx *local);

/* draw: VAT/arrays setup + hierarchy traversal */
void j3d_draw(J3DInstance *inst);
/* apply material mi of the instance's model (as j3d_draw does) */
void j3d_apply_material(J3DInstance *inst, int mi);
/* draw shape si with position matrix P; call after j3d_draw (vertex arrays set) */
void j3d_draw_shape_mtx(J3DInstance *inst, int si, Mtx P);
/* the drawn shapes' extent in view space (x0, x1, y0, y1), from their display lists
 * and the joint matrices of the last j3d_calc_*; 0 if nothing could be measured */
int  j3d_bounds(J3DInstance *inst, float b[4]);

/* the IPL's per-frame global GX setup done before the models (FUN_8130509c) */
void j3d_global_setup(void);

#endif
