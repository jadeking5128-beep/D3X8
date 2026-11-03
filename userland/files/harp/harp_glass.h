#pragma once
#include <stdint.h>

/*
 * Native Liquid Glass surface renderer for Harp.
 *
 * pixels is an opaque ARGB8888 framebuffer. The renderer snapshots only the
 * requested panel, then performs a bounded refraction/softening pass from
 * that immutable snapshot before compositing tint and optical highlights.
 */
void harp_glass_rrect(uint32_t *pixels, int screen_w, int screen_h,
                      int x, int y, int w, int h, int radius,
                      uint32_t tint, uint8_t tint_alpha,
                      uint8_t refraction, uint8_t blur_radius);
