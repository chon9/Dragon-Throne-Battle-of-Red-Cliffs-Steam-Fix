#define FONT_TEST
#include "SmoothFont.cpp"
static U8 atlas[3 * 65536 * 4 + 24 * 20];
U8* TestAtlas = atlas;
static U8 surface[0x70];
static U32 table[32];
static U16 pixels[64];
static int locks, unlocks, fallbacks;
static U8* __fastcall lockSurface(void*, void*, int) { ++locks; return (U8*)pixels; }
static void __fastcall unlockSurface(void*, void*) { ++unlocks; }
int __fastcall TestFallback(void*, void*, void*, int, int, int, int, int, int, Rect*, int) { ++fallbacks; return 7; }
int __fastcall TestAdvance(void*, void*, int, int) { return 18; }

extern "C" int main() {
    *(U32**)surface = table;
    table[12] = (U32)&lockSurface;
    table[13] = (U32)&unlockSurface;
    *(int*)(surface + 0xc) = 8;
    *(int*)(surface + 0x10) = 8;
    surface[0x34] = surface[0x35] = 16;
    *(short*)(surface + 0x4e) = 1;
    *(int*)(surface + 0x50) = 16;
    U32 start = 3 * 65536 * 4;
    ((U32*)atlas)[65] = start;
    atlas[start] = 255;
    atlas[start + 1] = 128;
    atlas[start + 2] = 0;
    atlas[start + 24] = 255;
    if (RenderGlyph(0,0,surface,1,1,65,0xffff,0,0,0,0) != 18) return 1;
    if (pixels[9] != 0xffff || pixels[10] != 0x8410 || pixels[11] != 0 || pixels[17] != 0xffff) return 2;
    if (locks != 1 || unlocks != 1 || fallbacks) return 3;
    for (int n = 0; n < 64; ++n) pixels[n] = 0xffff;
    Rect clip = {2,1,1,1};
    RenderGlyph(0,0,surface,1,1,65,0,0,0,&clip,0);
    if (pixels[10] != 0x7bef || pixels[9] != 0xffff || pixels[17] != 0xffff) return 4;
    for (int n = 0; n < 64; ++n) pixels[n] = 0;
    RenderGlyph(0,0,surface,-1,-1,65,0xffff,0,0,0,0);
    for (int n = 0; n < 64; ++n) if (pixels[n]) return 5;
    int prior = locks;
    RenderGlyph(0,0,surface,20,20,65,0xffff,0,0,0,0);
    if (locks != prior) return 6;
    if (RenderGlyph(0,0,surface,0,0,65,0xffff,0,2,0,0) != 7 || fallbacks != 1) return 7;
    surface[0x35] = 15;
    RenderGlyph(0,0,surface,0,0,65,0xffff,0,0,0,0);
    if (fallbacks != 2) return 8;
    surface[0x35] = 16;
    RenderGlyph(0,0,surface,0,0,66,0xffff,0,0,0,0);
    if (fallbacks != 3 || locks != unlocks) return 9;
    return 0;
}
