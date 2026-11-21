"""Single source of truth for the promo intro timeline (reference frames at 30 fps,
coordinates in 640x480 EFB pixels = the original 640x480 promo frame).
gen_header() writes source/anim_data.h for the Wii build; preview.py renders the same data."""
import math

S = 49 / 95                      # screen slope of the cube's horizontal edges (isometric, EFB pixels)
CX, CY = 319.0, 209.0            # logo centre (front-top vertex of the cube)
SX, SY = 0.842, 0.760            # model units -> EFB pixels (horizontal, vertical)

# --- light streaks -------------------------------------------------------------
# line kind: 'V' vertical (param = y), 'A' descending to the right (y = off + S*x, param = x),
# 'B' rising to the right (y = off - S*x, param = x).  glint param = v*t + c (t in ref frames)
TRACKS = [  # kind, off, v, c, t_first_visible, t_last_visible
    ('B', 373.5, -93.39, 890.4, 2.0, 10.0),
    ('V', 319.0, -148.7, 1043.1, 3.5, 7.5),
    ('B', 471.5, 100.0, -525.0, 4.5, 12.5),
    ('A', 142.5, 98.36, -696.7, 6.5, 13.5),
    ('V', 224.0, -77.18, 1108.7, 7.5, 14.8),
    ('V', 414.0, -63.73, 1147.4, 10.5, 18.5),
    ('A', -53.5, -164.0, 2394.3, 10.5, 14.5),
    ('A', 44.5, 62.75, -727.0, 12.5, 22.0),
    ('B', 275.5, 108.5, -1521.8, 13.5, 19.0),
]
# the cube edges drawn by the glints: (track index, param from, param to)
EDGES = [
    (0, 319.0, 414.0),   # top face, front-right
    (1, 209.0, 307.0),   # front vertical
    (2, 319.0, 414.0),   # bottom right
    (3, 224.0, 319.0),   # bottom left
    (4, 160.0, 258.0),   # left vertical
    (5, 160.0, 258.0),   # right vertical
    (6, 319.0, 389.0),   # top face, back-right (stops at the G opening)
    (7, 224.0, 319.0),   # top face, front-left
    (8, 224.0, 319.0),   # top face, back-left
]
STREAK_FADE = (16.0, 22.0)    # all glints/lines fade out over this window
EDGE_FADE = (29.0, 34.0)      # outline fades while the solid logo slams in
LINE_BASE = 0.10              # faint full-length streak line (fraction of white)
GLINT = (52.0, 4.5, 40.0, 13.0, 0.30)   # core along/across, halo along/across (exp(-(x/s)^2)), halo strength
EDGE_HW = 0.9                 # outline half width (px)
TRAIL_HW = 0.8                # streak half width (px)

# --- logo / text ----------------------------------------------------------------
# logo looks: colour = clamp(lum(IPL logo) * A + B) on the logo's faces (fitted on the reference)
LOOK_GLASS = ((1.239, 1.431, 1.155), (-0.274, -0.183, 0.098))
# glass stage: translucent outer cube faces (top, left = +z face, right = +x face) seen where the G is open
SHELL = ((0.089, 0.300, 0.528), (0.031, 0.124, 0.261), (0.018, 0.127, 0.254))
HALF = 79.8                      # logo cube half size (model units)
LOOK_CYAN = ((0.965, 0.789, 0.422), (0.126, 0.349, 0.674))
LOOK_FINAL = ((1.004, 0.970, 0.681), (0.067, 0.091, 0.403))
GLASS = [(14.0, 0.0), (16.0, 0.31), (20.0, 0.62), (24.0, 0.91), (27.0, 1.03), (28.0, 1.04)]  # glass look intensity
# text colours (measured on the glyph cores); the glass look uses the colour at min(t, 28),
# the solid look the colour at max(t, 38)
GAMECUBE_RGB = [(16.0, (0, 0, 0)), (17.0, (.15, .23, .27)), (18.0, (.31, .44, .52)), (20.0, (.49, .68, .78)),
                (22.0, (.63, .84, .93)), (24.0, (.74, .94, .98)), (26.0, (.80, .97, .99)), (38.0, (.84, .96, .98)),
                (40.0, (.90, .98, .98)), (45.0, (.95, .97, .97))]
NINTENDO_RGB = [(18.0, (0, 0, 0)), (19.0, (.04, .07, .09)), (21.0, (.14, .19, .23)), (23.0, (.23, .31, .36)),
                (25.0, (.30, .40, .47)), (27.0, (.33, .45, .53)), (38.0, (.65, .77, .83)), (40.0, (.71, .83, .89)),
                (45.0, (.67, .68, .69))]
GLASS_TEXT_T, SOLID_TEXT_T = 28.0, 38.0
GRADE = [(41.0, 0.0), (45.0, 1.0)]       # 0 = cyan look, 1 = natural (IPL) colours
GLOW = [(0.0, 2.0), (13.0, 2.0), (17.0, 0.8), (38.0, 0.8), (45.0, 0.12)]   # bloom strength
BLOOM_THR = 0.6                    # only what is brighter than this glows
BLOOM_TINT = (0.45, 0.75, 1.0)
# global fade to black: held until the sound has nearly died away (about -48 dB at 3.4 s),
# then 0.4 s to black (3.4-3.8 s at 29.97 fps); the clip itself fades at frames 70-82
FADE = [(101.9, 1.0), (113.9, 0.0)]
END_FRAME = 114
AUDIO_SECONDS = 4.0

GAMECUBE_BOX = (184, 346, 470, 370)      # glyph bbox incl. TM (x0, y0, x1, y1)
NINTENDO_CENTERS = [197.5, 230.5, 264.0, 299.0, 333.0, 368.5, 405.0, 440.5]
NINTENDO_Y = (320, 339)

# slam: per reference frame, the logo+text layer drawn as copies: (look, dy px, y-stretch, weight)
# look 0 = glass, 1 = solid (cyan).  Fitted on the reference frames (tools/fit_slam2.py, far weak copies pruned).
SLAM = {
    29: [(1, 60, 2.3, .425), (0, 0, 1.0, .473), (0, -48, 1.35, .238), (1, 160, 1.35, .153)],
    30: [(1, 0, 1.0, .594), (1, -160, 1.0, .263), (0, -84, 1.5, .196), (0, -108, 1.1, .127)],
    31: [(1, -20, 2.3, .469), (1, 0, 1.0, .399), (0, 112, 1.0, .187), (0, -116, 1.75, .165)],
    32: [(1, 0, 1.0, .602), (0, -156, 1.75, .147), (1, 76, 1.75, .140), (1, -28, 1.0, .147)],
    33: [(1, 8, 1.1, .450), (1, 28, 1.0, .350), (1, -8, 1.05, .238)],
    34: [(1, -4, 1.0, .650), (1, -24, 1.1, .175), (1, 12, 1.0, .163)],
    35: [(1, 4, 1.05, .716), (1, 24, 1.0, .257)],
    36: [(1, 4, 1.0, .797), (1, -16, 1.1, .162)],
    37: [(1, 4, 1.05, .429), (1, 24, 1.0, .221), (1, 0, 1.0, .351)],
    38: [(1, 0, 1.0, .806), (1, 16, 1.0, .143)],
}
SLAM_ANCHOR = 209.0


def ramp(keys, t):
    if t <= keys[0][0]: return keys[0][1]
    for (t0, v0), (t1, v1) in zip(keys, keys[1:]):
        if t <= t1:
            w = (t - t0) / (t1 - t0)
            if isinstance(v0, tuple): return tuple(a + (b - a) * w for a, b in zip(v0, v1))
            return v0 + (v1 - v0) * w
    return keys[-1][1]


def line_point(kind, off, p):
    if kind == 'V': return (off, p)
    if kind == 'A': return (p, off + S * p)
    return (p, off - S * p)


def line_dir(kind):
    if kind == 'V': return (0.0, -1.0)
    n = math.hypot(1, S)
    return (1 / n, S / n) if kind == 'A' else (1 / n, -S / n)


def gen_header(path):
    K = {'V': 0, 'A': 1, 'B': 2}
    o = ['/* generated by tools/anim.py - promo intro timeline (reference frames @30 fps, EFB pixels) */',
         '#ifndef ANIM_DATA_H', '#define ANIM_DATA_H', '',
         '#define A_SLOPE %.6ff' % S, '#define A_CX %.1ff' % CX, '#define A_CY %.1ff' % CY,
         '#define A_SX %.4ff' % SX, '#define A_SY %.4ff' % SY,
         '#define A_END_FRAME %d' % END_FRAME, '#define A_AUDIO_SECONDS %.2ff' % AUDIO_SECONDS,
         '#define A_LINE_BASE %.3ff' % LINE_BASE,
         '#define A_EDGE_HW %.2ff' % EDGE_HW, '#define A_TRAIL_HW %.2ff' % TRAIL_HW,
         'static const float a_glint[5] = {%.2ff, %.2ff, %.2ff, %.2ff, %.3ff};' % GLINT, '#define A_SLAM_ANCHOR %.1ff' % SLAM_ANCHOR,
         '#define A_BLOOM_THR %.2ff' % BLOOM_THR,
         'static const float a_bloom_tint[3] = {%.3ff, %.3ff, %.3ff};' % BLOOM_TINT,
         'typedef struct { float t, v; } AKey;', 'typedef struct { float t, v[3]; } AKey3;',
         'typedef struct { int kind; float off, v, c, t0, t1; } ATrack;',
         'typedef struct { int track; float p0, p1; } AEdge;',
         'typedef struct { int look; float dy, sy, w; } ALayer;', '']
    o.append('static const ATrack a_tracks[%d] = {' % len(TRACKS))
    for k, off, v, c, t0, t1 in TRACKS:
        o.append('\t{%d, %.2ff, %.3ff, %.2ff, %.2ff, %.2ff},' % (K[k], off, v, c, t0, t1))
    o.append('};')
    o.append('static const AEdge a_edges[%d] = {' % len(EDGES))
    for t, a, b in EDGES: o.append('\t{%d, %.1ff, %.1ff},' % (t, a, b))
    o.append('};')
    def keys(name, ks):
        o.append('static const AKey %s[%d] = {%s};' % (name, len(ks), ', '.join('{%.2ff, %.4ff}' % kv for kv in ks)))
    keys('a_glass', GLASS)
    for nm, ks in (('a_gamecube_rgb', GAMECUBE_RGB), ('a_nintendo_rgb', NINTENDO_RGB)):
        o.append('static const AKey3 %s[%d] = {%s};' % (nm, len(ks), ', '.join('{%.2ff, {%.3ff, %.3ff, %.3ff}}' % ((t,) + tuple(c)) for t, c in ks)))
    o.append('#define A_GLASS_TEXT_T %.1ff' % GLASS_TEXT_T); o.append('#define A_SOLID_TEXT_T %.1ff' % SOLID_TEXT_T)
    keys('a_grade', GRADE); keys('a_glow', GLOW); keys('a_fade', FADE)
    keys('a_streak_fade', [(STREAK_FADE[0], 1.0), (STREAK_FADE[1], 0.0)])
    keys('a_edge_fade', [(EDGE_FADE[0], 1.0), (EDGE_FADE[1], 0.0)])
    for nm, (a, b) in (('glass', LOOK_GLASS), ('cyan', LOOK_CYAN), ('final', LOOK_FINAL)):
        o.append('static const float a_look_%s[2][3] = {{%.3ff, %.3ff, %.3ff}, {%.3ff, %.3ff, %.3ff}};' % ((nm,) + tuple(a) + tuple(b)))
    o.append('static const float a_shell[3][3] = {%s};' % ', '.join('{%.3ff, %.3ff, %.3ff}' % c for c in SHELL))
    o.append('#define A_HALF %.2ff' % HALF)
    o.append('static const float a_gamecube_box[4] = {%.1ff, %.1ff, %.1ff, %.1ff};' % GAMECUBE_BOX)
    o.append('static const float a_nintendo_x[8] = {%s};' % ', '.join('%.1ff' % x for x in NINTENDO_CENTERS))
    o.append('static const float a_nintendo_y[2] = {%.1ff, %.1ff};' % NINTENDO_Y)
    first = min(SLAM); last = max(SLAM)
    o.append('#define A_SLAM_FIRST %d' % first); o.append('#define A_SLAM_LAST %d' % last)
    o.append('#define A_SLAM_MAXL 5')
    o.append('static const ALayer a_slam[%d][A_SLAM_MAXL] = {' % (last - first + 1))
    for f in range(first, last + 1):
        ls = SLAM[f] + [(0, 0, 1, 0)] * (5 - len(SLAM[f]))
        o.append('\t{' + ', '.join('{%d, %.1ff, %.2ff, %.3ff}' % l for l in ls) + '},')
    o.append('};'); o.append(''); o.append('#endif')
    open(path, 'w').write('\n'.join(o) + '\n')


if __name__ == '__main__':
    import sys
    gen_header(sys.argv[1] if len(sys.argv) > 1 else 'anim_data.h')
