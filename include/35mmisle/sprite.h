#ifndef ISLE_SPRITE_H
#define ISLE_SPRITE_H

#include <stddef.h>
#include <stdint.h>

/* Native C views, not serialized layouts. Texel and palette bytes remain in
 * canonical N64 byte order. A resident asset owner must outlive every copied
 * view and renderer use; copying a view does not transfer ownership. Mutable
 * placement/tint lives in the game object's copy, not the immutable asset.
 */
typedef struct IsleBitmap {
    int16_t width;
    int16_t width_img;
    int16_t s;
    int16_t t;
    const uint8_t* buf;
    size_t byte_count;
    int16_t actualHeight;
    int16_t LUToffset;
} IsleBitmap;

typedef struct IsleSprite {
    int16_t x;
    int16_t y;
    int16_t width;
    int16_t height;
    float scalex;
    float scaley;
    int16_t expx;
    int16_t expy;
    uint16_t attr;
    int16_t zdepth;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t alpha;
    int16_t startTLUT;
    int16_t nTLUT;
    const uint8_t* LUT;
    size_t LUT_bytes;
    int16_t istart;
    int16_t istep;
    int16_t nbitmaps;
    int16_t ndisplist;
    int16_t bmheight;
    int16_t bmHreal;
    uint8_t bmfmt;
    uint8_t bmsiz;
    const IsleBitmap* bitmap;
    int16_t frac_s;
    int16_t frac_t;
} IsleSprite;

/* Original attribute values are format/interface facts. No SDK implementation
 * or pointer-bearing wire structure is imported by these declarations.
 */
enum {
    ISLE_SP_TRANSPARENT = 0x0001,
    ISLE_SP_CUTOUT = 0x0002,
    ISLE_SP_HIDDEN = 0x0004,
    ISLE_SP_Z = 0x0008,
    ISLE_SP_SCALE = 0x0010,
    ISLE_SP_FASTCOPY = 0x0020,
    ISLE_SP_OVERLAP = 0x0040,
    ISLE_SP_TEXSHIFT = 0x0080,
    ISLE_SP_FRACPOS = 0x0100,
    ISLE_SP_TEXSHUF = 0x0200,
    ISLE_SP_EXTERN = 0x0400
};

/* Complete native scalar operations on a caller-owned mutable view. The
 * caller retains the view's real owner/lifetime; these do not validate handles,
 * allocate, copy a sprite, or extend bitmap/palette ownership. */
static inline void isle_sprite_move(IsleSprite* sprite, int32_t x, int32_t y) {
    /* Decode signed low lanes explicitly: out-of-range signed casts are not
     * portable, and subtracting in int32_t before narrowing cannot overflow. */
    uint32_t low_x = (uint32_t)x & UINT32_C(0xffff);
    uint32_t low_y = (uint32_t)y & UINT32_C(0xffff);
    sprite->x = (int16_t)(low_x <= INT16_MAX ? (int32_t)low_x : (int32_t)low_x - 65536);
    sprite->y = (int16_t)(low_y <= INT16_MAX ? (int32_t)low_y : (int32_t)low_y - 65536);
}

static inline void isle_sprite_scale(IsleSprite* sprite, float x, float y) {
    sprite->scalex = x;
    sprite->scaley = y;
    sprite->attr |= ISLE_SP_SCALE;
}

static inline void isle_sprite_color(IsleSprite* sprite, uint8_t r, uint8_t g,
    uint8_t b, uint8_t a) {
    sprite->red = r;
    sprite->green = g;
    sprite->blue = b;
    sprite->alpha = a;
}

static inline void isle_sprite_set_attribute(IsleSprite* sprite, uint16_t attributes) {
    sprite->attr |= attributes;
}

static inline void isle_sprite_clear_attribute(IsleSprite* sprite, uint16_t attributes) {
    sprite->attr &= (uint16_t)~attributes;
}

#endif
