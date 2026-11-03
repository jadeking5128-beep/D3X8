#pragma once
#include <stdint.h>
#include <stddef.h>

typedef enum {
    HARP_UI_CONTAINER = 0,
    HARP_UI_ROW,
    HARP_UI_COLUMN,
    HARP_UI_STACK,
    HARP_UI_TEXT,
    HARP_UI_BUTTON,
    HARP_UI_GLASS,
    HARP_UI_SPACER
} HarpUiType;

typedef enum {
    HARP_UI_ALIGN_START = 0,
    HARP_UI_ALIGN_CENTER,
    HARP_UI_ALIGN_END,
    HARP_UI_ALIGN_STRETCH
} HarpUiAlign;

typedef struct {
    uint32_t background;
    uint32_t foreground;
    uint16_t padding;
    uint16_t gap;
    uint16_t radius;
    uint8_t opacity;
    uint8_t glass;
    uint8_t refraction;
    uint8_t blur_radius;
} HarpUiStyle;

typedef struct HarpUiNode HarpUiNode;
typedef void (*HarpUiClickFn)(HarpUiNode *node, void *userdata);

struct HarpUiNode {
    HarpUiType type;
    int x, y, w, h;
    int preferred_w, preferred_h;
    unsigned flex;
    HarpUiAlign align;
    HarpUiStyle style;
    char *text;
    HarpUiNode **children;
    size_t child_count, child_capacity;
    HarpUiClickFn on_click;
    void *userdata;
    uint8_t pressed;
    uint8_t dirty;
};

typedef struct {
    void *userdata;
    void (*fill_rect)(void *userdata, int x, int y, int w, int h,
                      int radius, uint32_t color, uint8_t opacity);
    void (*glass_rect)(void *userdata, int x, int y, int w, int h,
                       int radius, uint32_t tint, uint8_t opacity,
                       uint8_t refraction, uint8_t blur_radius);
    void (*draw_text)(void *userdata, int x, int y, int max_w,
                      uint32_t color, const char *text);
    int (*measure_text)(void *userdata, const char *text);
} HarpUiRenderer;

HarpUiNode *harp_ui_create(HarpUiType type);
void harp_ui_destroy(HarpUiNode *node);
int harp_ui_append(HarpUiNode *parent, HarpUiNode *child);
void harp_ui_set_text(HarpUiNode *node, const char *text);
void harp_ui_set_style(HarpUiNode *node, const HarpUiStyle *style);
void harp_ui_set_size(HarpUiNode *node, int width, int height);
void harp_ui_set_flex(HarpUiNode *node, unsigned flex);
void harp_ui_on_click(HarpUiNode *node, HarpUiClickFn callback, void *userdata);
void harp_ui_layout(HarpUiNode *root, int x, int y, int width, int height);
void harp_ui_render(HarpUiNode *root, const HarpUiRenderer *renderer);
int harp_ui_pointer_down(HarpUiNode *root, int x, int y);
void harp_ui_pointer_up(HarpUiNode *root);
