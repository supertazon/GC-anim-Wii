"""Block alphabet for the custom NINTENDO letters: original designs in the style of the IPL letters
(squared, rounded corners), built from rectangles, rounded rings and slanted strokes."""
from shapely.geometry import Polygon, box
from shapely.ops import unary_union
import math

B, T = -7.88, 7.05          # baseline, cap height (IPL letters)
SV, SH = 3.1, 2.75          # vertical / horizontal stroke
RO, RI = 4.3, 1.7           # outer / inner corner radius
MB0, MB1 = -1.83, 0.88      # middle bar (E)
L, R = -5.8, 5.6            # default glyph box
NSEG = 4

def rrect(x0, y0, x1, y1, corners='', ro=RO):
    """rectangle with the listed corners (bl, br, tr, tl) rounded"""
    ro = min(ro, (x1 - x0) / 2, (y1 - y0) / 2)
    pts = []
    def corner(cx, cy, a0, name, px, py):
        if name in corners and ro > 0:
            for i in range(NSEG + 1):
                a = math.radians(a0 + 90 * i / NSEG)
                pts.append((cx + ro * math.cos(a), cy + ro * math.sin(a)))
        else:
            pts.append((px, py))
    corner(x0 + ro, y0 + ro, 180, 'bl', x0, y0)
    corner(x1 - ro, y0 + ro, 270, 'br', x1, y0)
    corner(x1 - ro, y1 - ro, 0, 'tr', x1, y1)
    corner(x0 + ro, y1 - ro, 90, 'tl', x0, y1)
    return Polygon(pts)

def ring(x0, y0, x1, y1, corners='', ro=RO, ri=RI, sv=SV, sh=SH):
    return rrect(x0, y0, x1, y1, corners, ro).difference(rrect(x0 + sv, y0 + sh, x1 - sv, y1 - sh, corners, ri))

def bar(x0, y0, x1, y1):
    return box(x0, y0, x1, y1)

def diag(xt, yt, xb, yb, w=3.5):
    """slanted stroke, horizontal width w; xt/xb = left edge at top / bottom"""
    return Polygon([(xt, yt), (xt + w, yt), (xb + w, yb), (xb, yb)])

def clip(g, x0=-50, x1=50):
    return g.intersection(box(x0, B, x1, T + 0.2))

def G(c):
    l, r = L, R
    if c == 'A':
        g = ring(l, B - 5, r, T, 'tl tr').intersection(box(l, B, r, T))
        g = g.union(bar(l, -2.6, r, 0.15))
    elif c == 'B':
        g = unary_union([bar(l, B, l + SV, T),
                         ring(l, MB0, r - 0.4, T, 'tr br', ro=3.2),
                         ring(l, B, r, MB1, 'tr br', ro=3.2)])
    elif c == 'C':
        g = ring(l, B, r, T, 'bl br tr tl').difference(box(0.6, B + SH, r + 1, T - SH))
    elif c == 'D':
        g = ring(l, B, r + 0.25, T, 'tr br')
    elif c == 'E':
        g = ring(l, B, r, T, 'tl bl').difference(box(l + SV, B + SH, r + 1, T - SH)).union(bar(l, MB0, r, MB1))
    elif c == 'F':
        g = ring(l, B - 5, r, T, 'tl').intersection(box(l, B, r, T)).difference(box(l + SV, B - 1, r + 1, T - SH)).union(bar(l, MB0, r - 0.6, MB1))
    elif c == 'G':
        g = ring(l, B, r, T, 'bl br tr tl').difference(box(0.6, MB1, r + 1, T - SH)).union(bar(0.2, MB0, r, MB1))
    elif c == 'H':
        g = unary_union([bar(l, B, l + SV, T), bar(r - SV, B, r, T), bar(l, MB0, r, MB1)])
    elif c == 'I':
        g = bar(-1.79, B, 1.79, T)
    elif c == 'J':
        g = ring(l, B, r, T + 5, 'bl br').intersection(box(l, B, r, T)).difference(box(l - 1, -2.6, l + SV + 0.01, T + 1))
    elif c == 'K':
        g = unary_union([bar(l, B, l + SV, T),
                         clip(diag(r - 3.9, T, l + SV - 0.6, -1.6, 3.9)),
                         clip(diag(-1.5, -0.3, r - 3.6, B, 3.6))]).intersection(box(l, B, r, T))
    elif c == 'L':
        g = ring(l, B, r, T + 5, 'bl').intersection(box(l, B, r, T)).difference(box(l + SV, B + SH, r + 1, T + 1))
    elif c == 'M':
        l, r = -6.9, 6.7
        g = unary_union([bar(l, B, l + SV, T), bar(r - SV, B, r, T),
                         diag(l, T, -1.85, -3.0, 3.7), diag(r - 3.7, T, -1.85, -3.0, 3.7)]).intersection(box(l, B, r, T))
    elif c == 'N':
        g = unary_union([bar(-5.91, B, -2.88, T), bar(2.21, B, 5.25, T),
                         Polygon([(-5.91, T), (-1.86, T), (5.25, B), (2.18, B)])]).intersection(box(-5.91, B, 5.25, T))
    elif c == 'O':
        g = ring(-5.94, -7.99, 5.94, 7.16, 'bl br tr tl', ri=RI + 0.3)
    elif c == 'P':
        g = bar(l, B, l + SV, T).union(ring(l, MB0, r, T, 'tr br', ro=3.6))
    elif c == 'Q':
        g = ring(-5.94, -7.99, 5.94, 7.16, 'bl br tr tl', ri=RI + 0.3).union(clip(diag(0.2, -2.4, 3.4, B, 3.3)))
    elif c == 'R':
        g = unary_union([bar(l, B, l + SV, T), ring(l, MB0, r, T, 'tr br', ro=3.6),
                         clip(diag(-0.6, MB1 - 0.1, r - 3.6, B, 3.6))]).intersection(box(l, B, r, T))
    elif c == 'S':
        top = ring(l, MB0, r, T, 'tl bl', ro=3.4).difference(box(l + SV, MB1, r + 1, T - SH))
        bot = ring(l, B, r, MB1, 'tr br', ro=3.4).difference(box(l - 1, B + SH, r - SV, MB0))
        g = top.union(bot)
    elif c == 'T':
        g = bar(-1.69, B, 1.70, 4.09).union(bar(-6.08, 4.09, 6.16, 7.04))
    elif c == 'U':
        g = ring(l, B, r, T + 5, 'bl br').intersection(box(l, B, r, T))
    elif c == 'V':
        l, r = -6.2, 6.0
        g = unary_union([diag(l, T, -1.95, B, 3.8), diag(r - 3.8, T, -1.95, B, 3.8)]).intersection(box(l, B, r, T))
    elif c == 'W':
        l, r = -6.9, 6.7
        g = unary_union([bar(l, B, l + SV, T), bar(r - SV, B, r, T),
                         diag(-1.85, 2.2, l, B, 3.7), diag(-1.85, 2.2, r - 3.7, B, 3.7)]).intersection(box(l, B, r, T))
    elif c == 'X':
        g = unary_union([diag(l, T, r - 3.8, B, 3.8), diag(r - 3.8, T, l, B, 3.8)]).intersection(box(l, B, r, T))
    elif c == 'Y':
        g = unary_union([diag(l, T, -1.8, -1.2, 3.6), diag(r - 3.6, T, -1.8, -1.2, 3.6),
                         bar(-1.75, B, 1.75, -0.5)]).intersection(box(l, B, r, T))
    elif c == 'Z':
        g = unary_union([bar(l, T - SH, r, T), bar(l, B, r, B + SH),
                         diag(r - 3.9, T - SH + 0.01, l, B + SH - 0.01, 3.9)])
    else:
        raise KeyError(c)
    return g
