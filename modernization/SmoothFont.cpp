// Exact-build text hook. No CRT, Windows imports, global state or allocation.
typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;
struct Rect { int x, y, w, h; };
typedef int (__thiscall *Original)(void*, void*, int, int, int, int, int, int, Rect*, int);
typedef U8* (__thiscall *Lock)(void*, int);
typedef void (__thiscall *Unlock)(void*);
typedef int (__thiscall *Advance)(void*, int, int);

#ifdef FONT_TEST
extern U8* TestAtlas;
int __fastcall TestFallback(void*, void*, void*, int, int, int, int, int, int, Rect*, int);
int __fastcall TestAdvance(void*, void*, int, int);
#define ATLAS TestAtlas
#define FALLBACK(e,t,x,y,c,f,b,m,r,i) TestFallback(e,0,t,x,y,c,f,b,m,r,i)
#define ADVANCE(e,c,i) TestAdvance(e,0,c,i)
#else
#define ATLAS ((U8*)FONT_ATLAS)
#define FALLBACK(e,t,x,y,c,f,b,m,r,i) ((Original)FONT_ORIGINAL)(e,t,x,y,c,f,b,m,r,i)
#define ADVANCE(e,c,i) ((Advance)0x6600f0)(e,c,i)
#endif

static U16 blend(U16 fg, U16 bg, unsigned alpha) {
    const unsigned inverse = 255 - alpha;
    unsigned r = (((fg >> 11) & 31) * alpha + ((bg >> 11) & 31) * inverse + 127) / 255;
    unsigned g = (((fg >> 5) & 63) * alpha + ((bg >> 5) & 63) * inverse + 127) / 255;
    unsigned b = ((fg & 31) * alpha + (bg & 31) * inverse + 127) / 255;
    return (U16)((r << 11) | (g << 5) | b);
}

// __fastcall consumes the original ECX and an unused EDX, preserving the
// original thiscall's nine stack arguments and callee stack cleanup.
extern "C" int __fastcall RenderGlyph(void* engine, void*, void* target,
    int x, int y, int character, int foreground, int background, int mode,
    Rect* clip, int fontIndex) {
    U8* surface = (U8*)target;
    int index = fontIndex == -1 ? *(int*)((U8*)engine + 0x4b0) : fontIndex;
    if (!surface || index < 0 || index > 2 || character < 32 || character > 65535 ||
        mode != 0 || surface[0x34] != 16 || surface[0x35] != 16)
        return FALLBACK(engine,target,x,y,character,foreground,background,mode,clip,fontIndex);
    U32 offset = ((U32*)ATLAS)[index * 65536 + character];
    if (!offset)
        return FALLBACK(engine,target,x,y,character,foreground,background,mode,clip,fontIndex);
    int advance = ADVANCE(engine,character,index);
    int width = *(int*)(surface + 0xc), height = *(int*)(surface + 0x10);
    if (width <= 0 || height <= 0 || width > 16384 || height > 16384)
        return FALLBACK(engine,target,x,y,character,foreground,background,mode,clip,fontIndex);
    int left = 0, top = 0, right = width, bottom = height;
    if (clip) {
        if (clip->w <= 0 || clip->h <= 0) return advance;
        if (clip->x > left) left = clip->x;
        if (clip->y > top) top = clip->y;
        if (clip->x + clip->w < right) right = clip->x + clip->w;
        if (clip->y + clip->h < bottom) bottom = clip->y + clip->h;
    }
    int glyphHeight = index == 2 ? 24 : 20;
    int sx = left > x ? left - x : 0;
    int sy = top > y ? top - y : 0;
    int ex = right - x < 24 ? right - x : 24;
    int ey = bottom - y < glyphHeight ? bottom - y : glyphHeight;
    if (sx >= ex || sy >= ey) return advance;
    U32* vtable = *(U32**)surface;
    U8* pixels = ((Lock)vtable[0x30 / 4])(target, 0);
    if (!pixels) {
        ((Unlock)vtable[0x34 / 4])(target);
        return advance;
    }
    int pitch = *(short*)(surface + 0x4e) * *(int*)(surface + 0x50);
    const U8* glyph = ATLAS + offset;
    for (int row = sy; row < ey; ++row) {
        U16* dest = (U16*)(pixels + (y + row) * pitch) + x + sx;
        const U8* alpha = glyph + row * 24 + sx;
        for (int column = sx; column < ex; ++column, ++dest, ++alpha) {
            if (*alpha) *dest = *alpha == 255 ? (U16)foreground : blend((U16)foreground, *dest, *alpha);
        }
    }
    ((Unlock)vtable[0x34 / 4])(target);
    return advance;
}

#ifndef FONT_TEST
extern "C" __declspec(naked) void FontEntry() {
    __asm jmp RenderGlyph
}
#endif
