#include "harp_glass.h"
#include <stdlib.h>
#include <string.h>

static uint32_t *glass_snapshot;
static size_t glass_snapshot_capacity;

static uint32_t glass_blend(uint32_t dst, uint32_t src, unsigned alpha)
{
    unsigned inv = 255u - alpha;
    unsigned sr = (src >> 16) & 255u, sg = (src >> 8) & 255u, sb = src & 255u;
    unsigned dr = (dst >> 16) & 255u, dg = (dst >> 8) & 255u, db = dst & 255u;
    return 0xFF000000u |
           (((sr * alpha + dr * inv) / 255u) << 16) |
           (((sg * alpha + dg * inv) / 255u) << 8) |
           ((sb * alpha + db * inv) / 255u);
}

static unsigned glass_channel(uint32_t p, unsigned shift)
{
    return (p >> shift) & 255u;
}

static int glass_clamp(int v, int lo, int hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

static int glass_inside(int x, int y, int w, int h, int r)
{
    if (r <= 0) return 1;
    int cx = x < r ? r - 1 : (x >= w - r ? w - r : -1);
    int cy = y < r ? r - 1 : (y >= h - r ? h - r : -1);
    if (cx < 0 || cy < 0) return 1;
    int dx = x - cx, dy = y - cy;
    return dx * dx + dy * dy <= r * r;
}

static uint32_t glass_sample(const uint32_t *src, int w, int h,
                             int x, int y, int blur)
{
    x = glass_clamp(x, 0, w - 1);
    y = glass_clamp(y, 0, h - 1);
    if (blur <= 0) return src[(size_t)y * (size_t)w + (size_t)x];

    int l = glass_clamp(x - blur, 0, w - 1);
    int rr = glass_clamp(x + blur, 0, w - 1);
    int t = glass_clamp(y - blur, 0, h - 1);
    int b = glass_clamp(y + blur, 0, h - 1);
    uint32_t p0 = src[(size_t)y * (size_t)w + (size_t)x];
    uint32_t p1 = src[(size_t)y * (size_t)w + (size_t)l];
    uint32_t p2 = src[(size_t)y * (size_t)w + (size_t)rr];
    uint32_t p3 = src[(size_t)t * (size_t)w + (size_t)x];
    uint32_t p4 = src[(size_t)b * (size_t)w + (size_t)x];
    unsigned r = glass_channel(p0,16) + glass_channel(p1,16) +
                 glass_channel(p2,16) + glass_channel(p3,16) + glass_channel(p4,16);
    unsigned g = glass_channel(p0,8) + glass_channel(p1,8) +
                 glass_channel(p2,8) + glass_channel(p3,8) + glass_channel(p4,8);
    unsigned bl = glass_channel(p0,0) + glass_channel(p1,0) +
                  glass_channel(p2,0) + glass_channel(p3,0) + glass_channel(p4,0);
    return 0xFF000000u | ((r / 5u) << 16) | ((g / 5u) << 8) | (bl / 5u);
}

void harp_glass_rrect(uint32_t *pixels, int screen_w, int screen_h,
                      int x, int y, int w, int h, int radius,
                      uint32_t tint, uint8_t tint_alpha,
                      uint8_t refraction, uint8_t blur_radius)
{
    if (!pixels || screen_w <= 0 || screen_h <= 0 || w <= 0 || h <= 0)
        return;

    if (radius < 0) radius = 0;
    if (radius > w / 2) radius = w / 2;
    if (radius > h / 2) radius = h / 2;

    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + w > screen_w ? screen_w : x + w;
    int y1 = y + h > screen_h ? screen_h : y + h;
    if (x0 >= x1 || y0 >= y1) return;

    int rw = x1 - x0, rh = y1 - y0;
    size_t count = (size_t)rw * (size_t)rh;
    if (count > (size_t)-1 / sizeof(uint32_t)) return;
    if (count > glass_snapshot_capacity) {
        uint32_t *next = (uint32_t *)realloc(glass_snapshot, count * sizeof(uint32_t));
        if (!next) {
            /* Allocation pressure: preserve a legible translucent fallback. */
            for (int py = y0; py < y1; py++)
                for (int px = x0; px < x1; px++) {
                    if (!glass_inside(px - x, py - y, w, h, radius)) continue;
                    size_t at = (size_t)py * (size_t)screen_w + (size_t)px;
                    pixels[at] = glass_blend(pixels[at], tint, tint_alpha);
                }
            return;
        }
        glass_snapshot = next;
        glass_snapshot_capacity = count;
    }

    for (int py = 0; py < rh; py++)
        memcpy(glass_snapshot + (size_t)py * (size_t)rw,
               pixels + (size_t)(y0 + py) * (size_t)screen_w + (size_t)x0,
               (size_t)rw * sizeof(uint32_t));

    int blur = blur_radius > 3 ? 3 : blur_radius;
    int refract = refraction > 12 ? 12 : refraction;
    int rim = refract * 2 + 5;
    if (rim < 5) rim = 5;

    for (int py = y0; py < y1; py++) {
        int ly = py - y;
        for (int px = x0; px < x1; px++) {
            int lx = px - x;
            if (!glass_inside(lx, ly, w, h, radius)) continue;

            int edge = lx;
            if (w - 1 - lx < edge) edge = w - 1 - lx;
            if (ly < edge) edge = ly;
            if (h - 1 - ly < edge) edge = h - 1 - ly;
            int edge_weight = rim - edge;
            if (edge_weight < 0) edge_weight = 0;
            if (edge_weight > rim) edge_weight = rim;

            int vx = lx * 2 - w;
            int vy = ly * 2 - h;
            int ox = 0, oy = 0;
            if (refract && edge_weight) {
                ox = (vx * refract * edge_weight) / (w * rim);
                oy = (vy * refract * edge_weight) / (h * rim);
            }

            int sx = px - x0 - ox;
            int sy = py - y0 - oy;
            uint32_t sample = glass_sample(glass_snapshot, rw, rh, sx, sy, blur);
            unsigned tint_mix = tint_alpha;
            if (tint_mix > 210) tint_mix = 210;
            uint32_t out = glass_blend(sample, tint, tint_mix);

            /* A soft top-facing reflection and a restrained lower edge. */
            int top_alpha = 0;
            if (ly < 3) top_alpha = 38 - ly * 8;
            else if (ly < rim) top_alpha = (rim - ly) * 5 / rim;
            if (top_alpha > 0)
                out = glass_blend(out, 0xFFFFFFFFu, (unsigned)top_alpha);

            if (edge <= 0)
                out = glass_blend(out, 0xFFFFFFFFu, 74);
            else if (edge == 1)
                out = glass_blend(out, 0xFFFFFFFFu, 24);

            if (ly >= h - 2 && edge <= 2)
                out = glass_blend(out, 0xFF18202Cu, 26);

            pixels[(size_t)py * (size_t)screen_w + (size_t)px] = out;
        }
    }
}
