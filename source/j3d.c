/*
 * j3d.c - renderer for the early J3D models of the GameCube IPL (see j3d.h)
 *
 * Reverse-engineered from the NTSC 1.2 BS2: FUN_81303898 (loader),
 * FUN_81303bec (INF1 tree), FUN_81304198 (vertex arrays), FUN_813065a0
 * (material -> GX), FUN_81304c08 / FUN_813048c4 (shape draw, env-map matrices),
 * FUN_8130509c (global setup).  Verified register-for-register against a
 * Dolphin FIFO log of the real boot animation.
 */
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <malloc.h>
#include "j3d.h"
#include "gcmath.h"

static inline u32 rd32(const u8 *p) { return (u32)p[0] << 24 | (u32)p[1] << 16 | (u32)p[2] << 8 | p[3]; }
static inline u16 rd16(const u8 *p) { return (u16)(p[0] << 8 | p[1]); }
static inline s16 rds16(const u8 *p) { return (s16)rd16(p); }
static inline float rdf(const u8 *p) { u32 v = rd32(p); float f; memcpy(&f, &v, 4); return f; }

/* MAT1 pool indices */
enum {
	P_ENTRIES, P_REMAP, P_NAMES, P_CULL, P_MATCOLOR, P_NCHAN, P_CHANINFO, P_NTEXGEN, P_TEXGEN,
	P_TEXMTX, P_TEXIDX, P_TEVORDER, P_TEVCOLOR, P_NTEV, P_TEVSTAGE, P_FOG, P_ALPHACMP, P_BLEND,
	P_ZMODE, P_POOL19, P_POOL20, P_NBT
};

/* ------------------------------------------------------------------------- */
/*  loading                                                                   */
/* ------------------------------------------------------------------------- */
static int comp_size(u32 type) { return type <= 1 ? 1 : (type <= 3 ? 2 : 4); }

static void parse_inf1(J3DModel *m)
{
	const u8 *p = m->inf1 + rd32(m->inf1 + 0x14);
	const u8 *q;
	int n = 0, parent = -1, last = -1;
	s16 *lastchild;

	for (q = p; rd16(q) != 0; q += 4)
		if (rd16(q) >= 0x10)
			n++;
	m->nodes = calloc(n ? n : 1, sizeof(J3DNode));
	m->nnodes = 0;
	/* parent link kept in a temporary array */
	lastchild = malloc(sizeof(s16) * (n ? n : 1));
	{
		s16 *par = calloc(n ? n : 1, sizeof(s16));
		int i;
		for (i = 0; i < n; i++)
			lastchild[i] = -1;
		for (q = p; rd16(q) != 0; q += 4) {
			u16 t = rd16(q), idx = rd16(q + 2);
			if (t == 0x10 || t == 0x11 || t == 0x12) {
				int k = m->nnodes++;
				J3DNode *nd = &m->nodes[k];
				nd->type = (u8)t;
				nd->index = idx;
				nd->child = -1;
				nd->next = -1;
				par[k] = (s16)parent;
				/* append as last child of parent (or as last top-level sibling) */
				if (parent >= 0) {
					if (m->nodes[parent].child < 0)
						m->nodes[parent].child = (s16)k;
					else
						m->nodes[lastchild[parent]].next = (s16)k;
					lastchild[parent] = (s16)k;
				} else if (k > 0) {
					int s = 0;
					while (m->nodes[s].next >= 0)
						s = m->nodes[s].next;
					m->nodes[s].next = (s16)k;
				}
				last = k;
			} else if (t == 1) {
				parent = last;
			} else if (t == 2) {
				parent = parent >= 0 ? par[parent] : -1;
			}
		}
		free(par);
	}
	free(lastchild);
}

static void parse_vtx1(J3DModel *m)
{
	static const u8 attrs[13] = {GX_VA_POS, GX_VA_NRM, GX_VA_NBT, GX_VA_CLR0, GX_VA_CLR1,
	                             GX_VA_TEX0, GX_VA_TEX1, GX_VA_TEX2, GX_VA_TEX3,
	                             GX_VA_TEX4, GX_VA_TEX5, GX_VA_TEX6, GX_VA_TEX7};
	static const u8 clr_size[6] = {2, 3, 4, 2, 3, 4};
	const u8 *f = m->vtx1 + rd32(m->vtx1 + 8);
	int k;

	m->nvfmt = 0;
	for (; rd32(f) != 0xFF && m->nvfmt < 16; f += 16) {
		m->vfmt[m->nvfmt].attr = rd32(f);
		m->vfmt[m->nvfmt].cnt = rd32(f + 4);
		m->vfmt[m->nvfmt].type = rd32(f + 8);
		m->vfmt[m->nvfmt].frac = f[12];
		m->nvfmt++;
	}
	for (k = 0; k < 13; k++) {
		u32 o = rd32(m->vtx1 + 0xC + 4 * k);
		m->varray[attrs[k]] = o ? (void *)(m->vtx1 + o) : NULL;
	}
	for (k = 0; k < m->nvfmt; k++) {
		u32 a = m->vfmt[k].attr, c = m->vfmt[k].cnt, t = m->vfmt[k].type;
		if (a == GX_VA_POS)
			m->vstride[a] = (c ? 3 : 2) * comp_size(t);
		else if (a == GX_VA_NRM)
			m->vstride[a] = 3 * comp_size(t);
		else if (a == GX_VA_CLR0 || a == GX_VA_CLR1)
			m->vstride[a] = t < 6 ? clr_size[t] : 4;
		else if (a >= GX_VA_TEX0 && a <= GX_VA_TEX7)
			m->vstride[a] = (c ? 2 : 1) * comp_size(t);
	}
}

static void parse_jnt1(J3DModel *m)
{
	const u8 *e = m->jnt1 + rd32(m->jnt1 + 0xC), *remap = m->jnt1 + rd32(m->jnt1 + 0x10);
	int i, k;
	m->njoints = rd16(m->jnt1 + 8);
	m->joints = calloc(m->njoints ? m->njoints : 1, sizeof(J3DJoint));
	for (i = 0; i < m->njoints; i++) {
		const u8 *j = e + 0x40 * rd16(remap + 2 * i);
		for (k = 0; k < 3; k++) {
			m->joints[i].scale[k] = rdf(j + 4 + 4 * k);
			m->joints[i].rot[k] = rds16(j + 0x10 + 2 * k);
			m->joints[i].trans[k] = rdf(j + 0x18 + 4 * k);
		}
	}
}

static int parse_shp1(J3DModel *m)
{
	const u8 *s = m->shp1;
	const u8 *e = s + rd32(s + 0xC), *remap = s + rd32(s + 0x10);
	const u8 *vd = s + rd32(s + 0x18), *mtab = s + rd32(s + 0x1C);
	const u8 *dl = s + rd32(s + 0x20), *grp = s + rd32(s + 0x24), *pk = s + rd32(s + 0x28);
	int i;

	m->nshapes = rd16(s + 8);
	m->shapes = calloc(m->nshapes ? m->nshapes : 1, sizeof(J3DShape));
	for (i = 0; i < m->nshapes; i++) {
		const u8 *sh = e + 0x28 * rd16(remap + 2 * i);
		const u8 *g = grp + 8 * rd16(sh + 6);
		const u8 *p = pk + 8 * rd16(sh + 8);
		const u8 *d = vd + rd16(sh + 4);
		J3DShape *o = &m->shapes[i];
		(void)mtab;
		o->mtx_type = sh[0];
		o->desc = d;
		o->ndesc = 0;
		while (rd32(d + 8 * o->ndesc) != 0xFF)
			o->ndesc++;
		o->drw = rd16(g);                     /* group 0 "use matrix" */
		o->dl = (void *)(dl + rd32(p + 4));
		o->dl_size = rd32(p);
		if (((uintptr_t)o->dl & 31) || (o->dl_size & 31)) {
			/* GX_CallDispList needs 32-byte alignment (never happens with IPL data) */
			u32 sz = (o->dl_size + 31) & ~31;
			void *c = memalign(32, sz);
			if (!c)
				return -1;
			memset(c, 0, sz);
			memcpy(c, o->dl, o->dl_size);
			DCFlushRange(c, sz);
			o->dl = c;
			o->dl_size = sz;
		}
	}
	return 0;
}

static void init_texture(J3DModel *m, int i, void *img, u16 w, u16 h, u8 fmt)
{
	const u8 *t = m->tex1 + rd32(m->tex1 + 0xC) + 0x20 * i;
	GX_InitTexObj(&m->texobj[i], img, w, h, fmt, t[6], t[7], t[0x10] ? GX_TRUE : GX_FALSE);
	GX_InitTexObjLOD(&m->texobj[i], t[0x14], t[0x15],
	                 0.125f * (float)(s8)t[0x16], 0.125f * (float)(s8)t[0x17],
	                 0.01f * (float)rds16(t + 0x1A), t[0x12], t[0x11], t[0x13]);
}

static void parse_tex1(J3DModel *m)
{
	const u8 *h = m->tex1 + rd32(m->tex1 + 0xC);
	int i;
	m->ntex = rd16(m->tex1 + 8);
	if (m->ntex > J3D_MAX_TEX)
		m->ntex = J3D_MAX_TEX;
	for (i = 0; i < m->ntex; i++) {
		const u8 *t = h + 0x20 * i;
		init_texture(m, i, (void *)(t + rd32(t + 0x1C)), rd16(t + 2), rd16(t + 4), t[0]);
		m->tex_custom[i] = 0;
	}
}

static void parse_mat1(J3DModel *m)
{
	const u8 *s = m->mat1;
	int i;
	for (i = 0; i < 22; i++) {
		u32 o = rd32(s + 0xC + 4 * i);
		m->mpool[i] = o ? s + o : NULL;
	}
	m->nmats = rd16(s + 8);
	m->mats = calloc(m->nmats ? m->nmats : 1, sizeof(J3DMat));
	for (i = 0; i < m->nmats; i++) {
		m->mats[i].e = m->mpool[P_ENTRIES] + 0x98 * rd16(m->mpool[P_REMAP] + 2 * i);
		m->mats[i].flag = m->mats[i].e[0];
	}
}

int j3d_load(J3DModel *m, u8 *data, u32 size)
{
	u32 p = 0x20;
	memset(m, 0, sizeof(*m));
	if (memcmp(data, "J3D1bmd1", 8))
		return -1;
	m->data = data;
	m->size = size;
	while (p + 8 < size) {
		const u8 *s = data + p;
		u32 sz = rd32(s + 4);
		if (!sz)
			break;
		if (!memcmp(s, "INF1", 4)) m->inf1 = s;
		else if (!memcmp(s, "VTX1", 4)) m->vtx1 = s;
		else if (!memcmp(s, "DRW1", 4)) m->drw1 = s;
		else if (!memcmp(s, "JNT1", 4)) m->jnt1 = s;
		else if (!memcmp(s, "SHP1", 4)) m->shp1 = s;
		else if (!memcmp(s, "MAT1", 4)) m->mat1 = s;
		else if (!memcmp(s, "TEX1", 4)) m->tex1 = s;
		p += (sz + 31) & ~31;
	}
	if (!m->inf1 || !m->vtx1 || !m->drw1 || !m->jnt1 || !m->shp1 || !m->mat1 || !m->tex1)
		return -2;

	parse_inf1(m);
	parse_vtx1(m);
	parse_jnt1(m);
	m->ndrw = rd16(m->drw1 + 8);
	m->drw_index = malloc(2 * (m->ndrw ? m->ndrw : 1));
	{
		int i;
		const u8 *d = m->drw1 + rd32(m->drw1 + 0x10);
		for (i = 0; i < m->ndrw; i++)
			m->drw_index[i] = rd16(d + 2 * i);
	}
	if (parse_shp1(m))
		return -3;
	parse_tex1(m);
	parse_mat1(m);
	DCFlushRange(data, (size + 31) & ~31);
	return 0;
}

void j3d_free(J3DModel *m)
{
	int i;
	for (i = 0; i < m->nshapes; i++) {
		u8 *dl = m->shapes[i].dl;
		if (dl < m->data || dl >= m->data + m->size)
			free(dl);
	}
	free(m->nodes);
	free(m->joints);
	free(m->drw_index);
	free(m->shapes);
	free(m->mats);
	memset(m, 0, sizeof(*m));
}

const u8 *j3d_tex_header(J3DModel *m, int i)
{
	return m->tex1 + rd32(m->tex1 + 0xC) + 0x20 * i;
}

void j3d_set_texture(J3DModel *m, int i, void *img, u16 w, u16 h, u8 fmt)
{
	if (i < 0 || i >= m->ntex)
		return;
	init_texture(m, i, img, w, h, fmt);
	m->tex_custom[i] = 1;
}

/* ------------------------------------------------------------------------- */
/*  joint matrices                                                            */
/* ------------------------------------------------------------------------- */
static void calc_node(J3DInstance *inst, int n, Mtx cur, Mtx *local)
{
	J3DModel *m = inst->model;
	while (n >= 0) {
		const J3DNode *nd = &m->nodes[n];
		Mtx c;
		gm_copy(cur, c);
		if (nd->type == 0x10 && nd->index < m->njoints) {
			Mtx l;
			if (local) {
				gm_copy(local[nd->index], l);
			} else {
				const J3DJoint *j = &m->joints[nd->index];
				gm_euler(l, j->trans[0], j->trans[1], j->trans[2], j->rot[0], j->rot[1], j->rot[2]);
				if (j->scale[0] != 1.0f || j->scale[1] != 1.0f || j->scale[2] != 1.0f)
					gm_scalecols(l, j->scale[0], j->scale[1], j->scale[2]);
			}
			gm_concat(cur, l, c);
			gm_copy(c, inst->jmtx[nd->index]);
		}
		if (nd->child >= 0)
			calc_node(inst, nd->child, c, local);
		n = nd->next;
	}
}

void j3d_calc_rest(J3DInstance *inst)
{
	calc_node(inst, 0, inst->base, NULL);
}

void j3d_calc_local(J3DInstance *inst, Mtx *local)
{
	calc_node(inst, 0, inst->base, local);
}

/* ------------------------------------------------------------------------- */
/*  drawing                                                                   */
/* ------------------------------------------------------------------------- */
static const u8 *pool8(J3DModel *m, int pool, u8 idx, int size)
{
	return (idx == 0xFF || !m->mpool[pool]) ? NULL : m->mpool[pool] + size * idx;
}

static const u8 *pool16(J3DModel *m, int pool, u16 idx, int size)
{
	return (idx == 0xFFFF || !m->mpool[pool]) ? NULL : m->mpool[pool] + size * idx;
}

/* material currently applied (the IPL keeps it in a global, not reset between models) */
static const J3DMat *cur_mat;
static J3DModel *cur_model;

static void apply_material(J3DInstance *inst, int mi)
{
	static const u8 chan_ids[4] = {GX_COLOR0, GX_ALPHA0, GX_COLOR1, GX_ALPHA1};
	J3DModel *m = inst->model;
	const J3DMat *mat = &m->mats[mi];
	const u8 *e = mat->e, *p;
	int k, ntev = 0;

	cur_mat = mat;
	cur_model = m;

	if ((p = pool8(m, P_CULL, e[1], 4)))
		GX_SetCullMode(p[3]);
	if ((p = pool8(m, P_NCHAN, e[2], 1)))
		GX_SetNumChans(p[0]);
	for (k = 0; k < 2; k++) {
		if ((p = pool16(m, P_MATCOLOR, rd16(e + 8 + 2 * k), 4))) {
			GXColor c = {p[0], p[1], p[2], p[3]};
			if (k == 0 && inst->matcolor0)
				c = inst->matcolor0[mi];
			GX_SetChanMatColor(k ? GX_COLOR1A1 : GX_COLOR0A0, c);
		}
	}
	for (k = 0; k < 4; k++)
		if ((p = pool16(m, P_CHANINFO, rd16(e + 0xC + 2 * k), 8)))
			GX_SetChanCtrl(chan_ids[k], p[0], GX_SRC_REG, p[1], p[2], p[3], p[4]);
	if ((p = pool8(m, P_NTEXGEN, e[3], 1))) {
		int n = p[0];
		GX_SetNumTexGens(n);
		for (k = 0; k < n && k < 8; k++) {
			const u8 *g = pool16(m, P_TEXGEN, rd16(e + 0x14 + 2 * k), 4);
			if (g)
				GX_SetTexCoordGen2(GX_TEXCOORD0 + k, g[0], g[1], g[2], GX_FALSE, GX_DTTIDENTITY);
		}
	}
	for (k = 0; k < 10; k++) {
		const u8 *t = pool16(m, P_TEXMTX, rd16(e + 0x24 + 2 * k), 0x34);
		if (t) {
			Mtx tm;
			int r, c;
			for (r = 0; r < 3; r++)
				for (c = 0; c < 4; c++)
					tm[r][c] = rdf(t + 16 * r + 4 * c);
			GX_LoadTexMtxImm(tm, GX_TEXMTX0 + 3 * k, t[0x30]);
		}
	}
	if ((p = pool8(m, P_NTEV, e[4], 1))) {
		ntev = p[0];
		GX_SetNumTevStages(ntev);
	}
	for (k = 0; k < 4; k++) {
		GXColorS10 c;
		int have = 0;
		if ((p = pool16(m, P_TEVCOLOR, rd16(e + 0x68 + 2 * k), 8))) {
			c.r = rds16(p); c.g = rds16(p + 2); c.b = rds16(p + 4); c.a = rds16(p + 6);
			have = 1;
		}
		if (inst->tevcolor && inst->tevcolor_set && inst->tevcolor_set[mi][k]) {
			c = inst->tevcolor[mi][k];
			have = 1;
		}
		if (have && k < 3)
			GX_SetTevColorS10(GX_TEVREG0 + k, c);
	}
	for (k = 0; k < 8; k++) {
		const u8 *s = pool16(m, P_TEVSTAGE, rd16(e + 0x70 + 2 * k), 0x14);
		const u8 *o = pool16(m, P_TEVORDER, rd16(e + 0x48 + 2 * k), 4);
		u16 ti = rd16(e + 0x38 + 2 * k);
		if (s) {
			GX_SetTevKColorSel(GX_TEVSTAGE0 + k, GX_TEV_KCSEL_1_4);
			GX_SetTevKAlphaSel(GX_TEVSTAGE0 + k, GX_TEV_KASEL_1);
			if (s[0] < 5) {
				GX_SetTevOp(GX_TEVSTAGE0 + k, s[0]);
			} else {
				if (inst->tex_color_from_ras && s[1] == GX_CC_ZERO && s[2] == GX_CC_TEXC &&
				    s[3] == GX_CC_ONE && s[4] == GX_CC_ZERO)
					/* custom text colour: colour from the material, shape from the texture alpha */
					GX_SetTevColorIn(GX_TEVSTAGE0 + k, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC);
				else
					GX_SetTevColorIn(GX_TEVSTAGE0 + k, s[1], s[2], s[3], s[4]);
				GX_SetTevColorOp(GX_TEVSTAGE0 + k, s[5], s[6], s[7], s[8], s[9]);
				GX_SetTevAlphaIn(GX_TEVSTAGE0 + k, s[10], s[11], s[12], s[13]);
				GX_SetTevAlphaOp(GX_TEVSTAGE0 + k, s[14], s[15], s[16], s[17], s[18]);
			}
		}
		if (o)
			GX_SetTevOrder(GX_TEVSTAGE0 + k, o[0], o[1], o[2]);
		else
			GX_SetTevOrder(GX_TEVSTAGE0 + k, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLORNULL);
		if (ti != 0xFFFF && m->mpool[P_TEXIDX]) {
			u16 t = rd16(m->mpool[P_TEXIDX] + 2 * ti);
			if (t < m->ntex)
				GX_LoadTexObj(&m->texobj[t], GX_TEXMAP0 + k);
		}
	}

	/* model alpha (FUN_81306c04 / tail of FUN_813065a0) */
	if (inst->alpha == 255) {
		GXColor k0 = {0, 0, 0, 255};
		GX_SetTevKColor(GX_KCOLOR0, k0);
		if (mat->flag & 1) {
			GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
			GX_SetZCompLoc(GX_TRUE);
		} else {
			GX_SetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
			GX_SetZCompLoc(GX_FALSE);
		}
	} else {
		GXColor k0 = {0, 0, 0, (u8)inst->alpha};
		GX_SetNumTevStages(ntev + 1);
		GX_SetTevKColor(GX_KCOLOR0, k0);
		GX_SetTevKAlphaSel(GX_TEVSTAGE0 + ntev, GX_TEV_KASEL_K0_A);
		GX_SetTevColorIn(GX_TEVSTAGE0 + ntev, GX_CC_ZERO, GX_CC_ONE, GX_CC_CPREV, GX_CC_ZERO);
		GX_SetTevColorOp(GX_TEVSTAGE0 + ntev, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
		GX_SetTevAlphaIn(GX_TEVSTAGE0 + ntev, GX_CA_ZERO, GX_CA_KONST, GX_CA_APREV, GX_CA_ZERO);
		GX_SetTevAlphaOp(GX_TEVSTAGE0 + ntev, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
		GX_SetTevOrder(GX_TEVSTAGE0 + ntev, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		GX_SetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
		GX_SetZCompLoc(GX_FALSE);
	}
}

static void draw_shape(J3DInstance *inst, int si)
{
	J3DModel *m = inst->model;
	const J3DShape *sh = &m->shapes[si];
	Mtx P, N;
	int k;

	GX_ClearVtxDesc();
	for (k = 0; k < sh->ndesc; k++)
		GX_SetVtxDesc(rd32(sh->desc + 8 * k), rd32(sh->desc + 8 * k + 4));

	gm_copy(inst->jmtx[m->drw_index[sh->drw]], P);
	if (sh->mtx_type == 1) {               /* billboard (not used by the boot models) */
		int r;
		for (r = 0; r < 3; r++) {
			float l = sqrtf(P[0][r] * P[0][r] + P[1][r] * P[1][r] + P[2][r] * P[2][r]);
			P[0][r] = P[1][r] = P[2][r] = 0.0f;
			P[r][r] = l;
		}
	}
	guMtxInvXpose(P, N);
	GX_LoadPosMtxImm(P, GX_PNMTX0);
	GX_LoadNrmMtxImm(N, GX_PNMTX0);

	/* env-mapped texture matrices: MAT1 matrix * rotation part of P (FUN_813048c4) */
	if (cur_mat && cur_model == m) {
		const u8 *e = cur_mat->e, *p;
		u16 done = 0;
		int n = 0;
		if ((p = pool8(m, P_NTEXGEN, e[3], 1)))
			n = p[0];
		for (k = 0; k < n && k < 8; k++) {
			const u8 *g = pool16(m, P_TEXGEN, rd16(e + 0x14 + 2 * k), 4);
			int tk;
			const u8 *t;
			if (!g || g[1] != GX_TG_NRM || g[2] < GX_TEXMTX0)
				continue;
			tk = (g[2] - GX_TEXMTX0) / 3;
			if (tk > 9 || (done & (1 << tk)))
				continue;
			done |= 1 << tk;
			if ((t = pool16(m, P_TEXMTX, rd16(e + 0x24 + 2 * tk), 0x34))) {
				Mtx tm, R, T;
				int r, c;
				for (r = 0; r < 3; r++)
					for (c = 0; c < 4; c++)
						tm[r][c] = rdf(t + 16 * r + 4 * c);
				gm_copy(P, R);
				R[0][3] = R[1][3] = R[2][3] = 0.0f;
				gm_concat(tm, R, T);
				GX_LoadTexMtxImm(T, GX_TEXMTX0 + 3 * tk, t[0x30]);
			}
		}
	}
	GX_CallDispList(sh->dl, sh->dl_size);
}

static void draw_nodes(J3DInstance *inst, int n)
{
	J3DModel *m = inst->model;
	while (n >= 0) {
		const J3DNode *nd = &m->nodes[n];
		if (nd->type == 0x11 && nd->index < m->nmats)
			apply_material(inst, nd->index);
		else if (nd->type == 0x12 && nd->index < m->nshapes &&
		         !(nd->index < 32 && (inst->shape_hide & (1u << nd->index))))
			draw_shape(inst, nd->index);
		if (nd->child >= 0)
			draw_nodes(inst, nd->child);
		n = nd->next;
	}
}

void j3d_draw(J3DInstance *inst)
{
	J3DModel *m = inst->model;
	int k;

	/* FUN_81304198: vertex format and arrays */
	for (k = 0; k < m->nvfmt; k++) {
		u32 a = m->vfmt[k].attr;
		GX_SetVtxAttrFmt(GX_VTXFMT0, a, m->vfmt[k].cnt, m->vfmt[k].type, m->vfmt[k].frac);
		if (a < 32 && m->varray[a])
			GX_SetArray(a, m->varray[a], m->vstride[a]);
	}
	GX_InvVtxCache();
	draw_nodes(inst, 0);
}

void j3d_apply_material(J3DInstance *inst, int mi)
{
	if (mi >= 0 && mi < inst->model->nmats)
		apply_material(inst, mi);
}

/* one position (attribute GX_VA_POS, index idx) of the model, in model units */
static int pos_at(const J3DModel *m, u32 idx, float out[3])
{
	int k, cnt = 1, type = 4, frac = 0, n;
	const u8 *p;
	float sc;
	for (k = 0; k < m->nvfmt; k++)
		if (m->vfmt[k].attr == GX_VA_POS) {
			cnt = (int)m->vfmt[k].cnt; type = (int)m->vfmt[k].type; frac = m->vfmt[k].frac;
		}
	if (!m->varray[GX_VA_POS] || !m->vstride[GX_VA_POS])
		return 0;
	p = (const u8 *)m->varray[GX_VA_POS] + idx * m->vstride[GX_VA_POS];
	n = cnt ? 3 : 2;
	sc = 1.0f / (float)(1 << frac);
	out[2] = 0.0f;
	for (k = 0; k < n; k++) {
		switch (type) {
		case 0: out[k] = p[k] * sc; break;
		case 1: out[k] = (s8)p[k] * sc; break;
		case 2: out[k] = (u16)((p[2 * k] << 8) | p[2 * k + 1]) * sc; break;
		case 3: out[k] = (s16)((p[2 * k] << 8) | p[2 * k + 1]) * sc; break;
		default: out[k] = rdf(p + 4 * k); break;
		}
	}
	return 1;
}

int j3d_bounds(J3DInstance *inst, float b[4])
{
	J3DModel *m = inst->model;
	int si, any = 0;
	b[0] = b[2] = 1e9f;
	b[1] = b[3] = -1e9f;
	for (si = 0; si < m->nshapes; si++) {
		const J3DShape *sh = &m->shapes[si];
		const u8 *d = (const u8 *)sh->dl, *e = d + sh->dl_size;
		MtxP P;
		if (si < 32 && (inst->shape_hide & (1u << si)))
			continue;
		P = inst->jmtx[m->drw_index[sh->drw]];
		while (d < e) {
			u8 cmd = *d++;
			if (cmd == 0x00)
				continue;                              /* NOP */
			if ((cmd & 0xf8) == 0x20 || (cmd & 0xf8) == 0x28 || (cmd & 0xf8) == 0x30 || (cmd & 0xf8) == 0x38) {
				d += 4;                                /* indexed XF loads (matrices) */
				continue;
			}
			if (cmd == 0x08) { d += 5; continue; }     /* CP register */
			if (cmd == 0x10) {                         /* XF registers */
				int nw = ((d[0] << 8) | d[1]) + 1;
				d += 4 + 4 * nw;
				continue;
			}
			if (cmd >= 0x80 && cmd < 0xc0) {          /* primitives */
				int nv = (d[0] << 8) | d[1], v, k;
				d += 2;
				for (v = 0; v < nv && d < e; v++) {
					u32 pidx = 0xffffffffu;
					for (k = 0; k < sh->ndesc; k++) {
						u32 a = rd32(sh->desc + 8 * k), t = rd32(sh->desc + 8 * k + 4);
						u32 val = 0;
						if (t == 0)
							continue;
						if (t == 1) {
							if (a <= GX_VA_TEX7MTXIDX) { d += 1; continue; }
							return any;                    /* direct data: not measured */
						}
						if (t == 2) val = *d++;
						else { val = (d[0] << 8) | d[1]; d += 2; }
						if (a == GX_VA_POS)
							pidx = val;
					}
					if (pidx != 0xffffffffu) {
						float q[3], o[3];
						if (!pos_at(m, pidx, q))
							continue;
						guVecMultiply(P, (guVector *)q, (guVector *)o);
						if (o[0] < b[0]) b[0] = o[0];
						if (o[0] > b[1]) b[1] = o[0];
						if (o[1] < b[2]) b[2] = o[1];
						if (o[1] > b[3]) b[3] = o[1];
						any = 1;
					}
				}
				continue;
			}
			break;                                     /* unknown: stop this list */
		}
	}
	return any;
}

void j3d_draw_shape_mtx(J3DInstance *inst, int si, Mtx P)
{
	J3DModel *m = inst->model;
	const J3DShape *sh;
	Mtx N;
	int k;
	if (si < 0 || si >= m->nshapes)
		return;
	sh = &m->shapes[si];
	GX_ClearVtxDesc();
	for (k = 0; k < sh->ndesc; k++)
		GX_SetVtxDesc(rd32(sh->desc + 8 * k), rd32(sh->desc + 8 * k + 4));
	guMtxInvXpose(P, N);
	GX_LoadPosMtxImm(P, GX_PNMTX0);
	GX_LoadNrmMtxImm(N, GX_PNMTX0);
	GX_CallDispList(sh->dl, sh->dl_size);
}

void j3d_global_setup(void)
{
	Mtx I;
	int i;
	GXColor white = {255, 255, 255, 255};

	gm_ident(I);
	GX_SetCullMode(GX_CULL_BACK);
	GX_SetCoPlanar(GX_FALSE);
	GX_SetColorUpdate(GX_TRUE);
	GX_SetAlphaUpdate(GX_TRUE);
	GX_SetDither(GX_TRUE);
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR1, GX_CLR_RGBA, GX_RGBA8, 0);
	for (i = 0; i < 8; i++)
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0 + i, GX_TEX_ST, GX_S16, 7);
	for (i = 0; i < 16; i++)
		GX_SetTevOrder(GX_TEVSTAGE0 + i, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLORNULL);
	GX_SetTevOp(GX_TEVSTAGE0, GX_REPLACE);
	for (i = 0; i < 10; i++) {
		GX_LoadPosMtxImm(I, GX_PNMTX0 + 3 * i);
		GX_LoadNrmMtxImm(I, GX_PNMTX0 + 3 * i);
		GX_LoadTexMtxImm(I, GX_TEXMTX0 + 3 * i, GX_MTX2x4);
	}
	GX_SetChanMatColor(GX_COLOR0A0, white);
	GX_SetChanMatColor(GX_COLOR1A1, white);
	GX_SetNumChans(1);
	GX_SetChanCtrl(GX_COLOR0A0, GX_DISABLE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_CLAMP, GX_AF_NONE);
	GX_SetChanCtrl(GX_COLOR1A1, GX_DISABLE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_CLAMP, GX_AF_NONE);
	GX_SetNumTexGens(0);
	for (i = 0; i < 8; i++)
		GX_SetTexCoordGen2(GX_TEXCOORD0 + i, GX_TG_MTX2x4, GX_TG_TEX0 + i, GX_IDENTITY, GX_FALSE, GX_DTTIDENTITY);
	GX_ClearVtxDesc();
	GX_InvVtxCache();
	GX_InvalidateTexAll();
	GX_SetAlphaCompare(GX_ALWAYS, 0, GX_AOP_OR, GX_ALWAYS, 0);
	GX_SetCurrentMtx(GX_PNMTX0);
}

int j3d_get_tevcolor(J3DModel *m, int mi, int k, GXColorS10 *out)
{
	const u8 *p;
	if (mi < 0 || mi >= m->nmats || k < 0 || k > 3)
		return 0;
	p = pool16(m, P_TEVCOLOR, rd16(m->mats[mi].e + 0x68 + 2 * k), 8);
	if (!p)
		return 0;
	out->r = rds16(p); out->g = rds16(p + 2); out->b = rds16(p + 4); out->a = rds16(p + 6);
	return 1;
}
