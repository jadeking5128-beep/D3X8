#include "harp_ui.h"
#include <stdlib.h>
#include <string.h>

static char *ui_copy_string(const char *s)
{
    if (!s) s = "";
    size_t n = strlen(s);
    char *copy = (char *)malloc(n + 1);
    if (!copy) return NULL;
    memcpy(copy, s, n + 1);
    return copy;
}

HarpUiNode *harp_ui_create(HarpUiType type)
{
    HarpUiNode *node = (HarpUiNode *)calloc(1, sizeof(*node));
    if (!node) return NULL;
    node->type = type;
    node->align = HARP_UI_ALIGN_STRETCH;
    node->style.background = 0xFF20232Au;
    node->style.foreground = 0xFFF4F6FAu;
    node->style.padding = 8;
    node->style.gap = 8;
    node->style.radius = 12;
    node->style.opacity = 255;
    node->dirty = 1;
    return node;
}

void harp_ui_destroy(HarpUiNode *node)
{
    if (!node) return;
    for (size_t i = 0; i < node->child_count; i++)
        harp_ui_destroy(node->children[i]);
    free(node->children);
    free(node->text);
    free(node);
}

int harp_ui_append(HarpUiNode *parent, HarpUiNode *child)
{
    if (!parent || !child || parent == child) return 0;
    if (parent->child_count == parent->child_capacity) {
        size_t next_capacity = parent->child_capacity ? parent->child_capacity * 2 : 4;
        if (next_capacity < parent->child_capacity ||
            next_capacity > (size_t)-1 / sizeof(HarpUiNode *))
            return 0;
        HarpUiNode **next = (HarpUiNode **)realloc(
            parent->children, next_capacity * sizeof(HarpUiNode *));
        if (!next) return 0;
        parent->children = next;
        parent->child_capacity = next_capacity;
    }
    parent->children[parent->child_count++] = child;
    parent->dirty = 1;
    return 1;
}

void harp_ui_set_text(HarpUiNode *node, const char *text)
{
    if (!node) return;
    char *copy = ui_copy_string(text);
    if (!copy) return;
    free(node->text);
    node->text = copy;
    node->dirty = 1;
}

void harp_ui_set_style(HarpUiNode *node, const HarpUiStyle *style)
{
    if (!node || !style) return;
    node->style = *style;
    node->dirty = 1;
}

void harp_ui_set_size(HarpUiNode *node, int width, int height)
{
    if (!node) return;
    node->preferred_w = width > 0 ? width : 0;
    node->preferred_h = height > 0 ? height : 0;
    node->dirty = 1;
}

void harp_ui_set_flex(HarpUiNode *node, unsigned flex)
{
    if (!node) return;
    node->flex = flex;
    node->dirty = 1;
}

void harp_ui_on_click(HarpUiNode *node, HarpUiClickFn callback, void *userdata)
{
    if (!node) return;
    node->on_click = callback;
    node->userdata = userdata;
}

static int ui_min(int a, int b) { return a < b ? a : b; }

static void ui_layout_node(HarpUiNode *node, int x, int y, int w, int h)
{
    if (!node) return;
    node->x = x; node->y = y;
    node->w = w > 0 ? w : 0; node->h = h > 0 ? h : 0;

    int pad = node->style.padding;
    int ix = x + pad, iy = y + pad;
    int iw = w - pad * 2, ih = h - pad * 2;
    if (iw < 0) iw = 0;
    if (ih < 0) ih = 0;

    if (node->type != HARP_UI_ROW && node->type != HARP_UI_COLUMN &&
        node->type != HARP_UI_STACK && node->type != HARP_UI_CONTAINER &&
        node->type != HARP_UI_GLASS)
        return;

    if (node->type == HARP_UI_STACK || node->type == HARP_UI_CONTAINER ||
        node->type == HARP_UI_GLASS) {
        for (size_t i = 0; i < node->child_count; i++) {
            HarpUiNode *child = node->children[i];
            int cw = child->preferred_w > 0 ? ui_min(child->preferred_w, iw) : iw;
            int ch = child->preferred_h > 0 ? ui_min(child->preferred_h, ih) : ih;
            int cx = ix, cy = iy;
            if (child->align == HARP_UI_ALIGN_CENTER) {
                cx += (iw - cw) / 2; cy += (ih - ch) / 2;
            } else if (child->align == HARP_UI_ALIGN_END) {
                cx += iw - cw; cy += ih - ch;
            }
            ui_layout_node(child, cx, cy, cw, ch);
        }
        return;
    }

    int row = node->type == HARP_UI_ROW;
    int main_available = row ? iw : ih;
    int cross_available = row ? ih : iw;
    int gaps = node->child_count > 1 ? (int)(node->child_count - 1) * node->style.gap : 0;
    int fixed = 0;
    unsigned flex_total = 0;
    for (size_t i = 0; i < node->child_count; i++) {
        HarpUiNode *child = node->children[i];
        int basis = row ? child->preferred_w : child->preferred_h;
        if (child->flex) flex_total += child->flex;
        else fixed += basis > 0 ? basis : 0;
    }
    int remaining = main_available - gaps - fixed;
    if (remaining < 0) remaining = 0;
    int cursor = row ? ix : iy;

    for (size_t i = 0; i < node->child_count; i++) {
        HarpUiNode *child = node->children[i];
        int basis = row ? child->preferred_w : child->preferred_h;
        int main_size = child->flex && flex_total
            ? (int)(((unsigned long long)remaining * child->flex) / flex_total)
            : (basis > 0 ? basis : 0);
        if (main_size > main_available) main_size = main_available;
        int cross_basis = row ? child->preferred_h : child->preferred_w;
        int cross_size = cross_basis > 0 ? ui_min(cross_basis, cross_available) : cross_available;
        int cross_offset = 0;
        if (child->align == HARP_UI_ALIGN_CENTER)
            cross_offset = (cross_available - cross_size) / 2;
        else if (child->align == HARP_UI_ALIGN_END)
            cross_offset = cross_available - cross_size;

        if (row)
            ui_layout_node(child, cursor, iy + cross_offset, main_size, cross_size);
        else
            ui_layout_node(child, ix + cross_offset, cursor, cross_size, main_size);
        cursor += main_size + node->style.gap;
    }
}

void harp_ui_layout(HarpUiNode *root, int x, int y, int width, int height)
{
    ui_layout_node(root, x, y, width, height);
}

static void ui_render_node(HarpUiNode *node, const HarpUiRenderer *renderer)
{
    if (!node || !renderer) return;
    if (node->w <= 0 || node->h <= 0) return;

    if (node->type == HARP_UI_GLASS && renderer->glass_rect) {
        renderer->glass_rect(renderer->userdata, node->x, node->y,
                             node->w, node->h, node->style.radius,
                             node->style.background, node->style.opacity,
                             node->style.refraction, node->style.blur_radius);
    } else if ((node->type == HARP_UI_CONTAINER || node->type == HARP_UI_BUTTON) &&
               renderer->fill_rect) {
        uint32_t color = node->style.background;
        if (node->type == HARP_UI_BUTTON && node->pressed)
            color = 0xFF477FCBu;
        renderer->fill_rect(renderer->userdata, node->x, node->y,
                            node->w, node->h, node->style.radius,
                            color, node->style.opacity);
    }

    if (node->text && renderer->draw_text) {
        int tx = node->x + node->style.padding;
        int ty = node->y + (node->h - 16) / 2;
        int max_w = node->w - node->style.padding * 2;
        if (max_w < 0) max_w = 0;
        if (node->type == HARP_UI_TEXT) ty = node->y + node->style.padding;
        renderer->draw_text(renderer->userdata, tx, ty, max_w,
                            node->style.foreground, node->text);
    }

    for (size_t i = 0; i < node->child_count; i++)
        ui_render_node(node->children[i], renderer);
    node->dirty = 0;
}

void harp_ui_render(HarpUiNode *root, const HarpUiRenderer *renderer)
{
    ui_render_node(root, renderer);
}

static int ui_hit(HarpUiNode *node, int x, int y)
{
    return node && x >= node->x && y >= node->y &&
           x < node->x + node->w && y < node->y + node->h;
}

static int ui_pointer_node(HarpUiNode *node, int x, int y)
{
    if (!ui_hit(node, x, y)) return 0;
    for (size_t i = node->child_count; i > 0; i--)
        if (ui_pointer_node(node->children[i - 1], x, y)) return 1;
    if (node->type == HARP_UI_BUTTON) {
        node->pressed = 1;
        node->dirty = 1;
        if (node->on_click) node->on_click(node, node->userdata);
        return 1;
    }
    return node->type != HARP_UI_SPACER;
}

int harp_ui_pointer_down(HarpUiNode *root, int x, int y)
{
    return ui_pointer_node(root, x, y);
}

static void ui_release_node(HarpUiNode *node)
{
    if (!node) return;
    node->pressed = 0;
    for (size_t i = 0; i < node->child_count; i++)
        ui_release_node(node->children[i]);
}

void harp_ui_pointer_up(HarpUiNode *root)
{
    ui_release_node(root);
}
