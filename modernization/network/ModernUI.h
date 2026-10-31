#pragma once
// Opt-in presentation prototype for the exact native executable marked DTMODERN1.
// No game economy, orders, multiplayer protocol or simulation data is modified.
#include <array>
#include <string>
#include <vector>
namespace modernui {
using U8=unsigned char; using U16=unsigned short; using U32=unsigned int;
inline int& I(void* p,int o) { return *reinterpret_cast<int*>(static_cast<U8*>(p)+o); }
inline void* P(void* p,int o) { return *reinterpret_cast<void**>(static_cast<U8*>(p)+o); }
struct Rect { int x,y,w,h; };
inline Rect R(void* p) { return *reinterpret_cast<Rect*>(static_cast<U8*>(p)+8); }
inline bool Equal(Rect a,Rect b) { return a.x==b.x&&a.y==b.y&&a.w==b.w&&a.h==b.h; }
inline void SetRect(void* p,Rect r) { *reinterpret_cast<Rect*>(static_cast<U8*>(p)+8)=r; I(p,0x18)=r.w*r.h; }
using TopDraw=int(__thiscall*)(void*,void*);
using ActionDraw=int(__thiscall*)(void*,void*,void*);
static TopDraw OriginalTop=reinterpret_cast<TopDraw>(0x56e4e0);
static ActionDraw OriginalActions=reinterpret_cast<ActionDraw>(0x57c3c0);
struct Surface {
    void* object=nullptr; U8* pixels=nullptr; int width=0,height=0,pitch=0;
    explicit Surface(void* p):object(p) {
        if(!p || static_cast<U8*>(p)[0x34]!=16 || static_cast<U8*>(p)[0x35]!=16)return;
        width=I(p,0xc);height=I(p,0x10);
        if(width<1024||width>3840||height<768||height>2160)return;
        auto v=*reinterpret_cast<uintptr_t**>(p);
        pixels=reinterpret_cast<U8*(__thiscall*)(void*,int)>(v[12])(p,0);
        pitch=*reinterpret_cast<short*>(static_cast<U8*>(p)+0x4e)*I(p,0x50);
        if(!pixels) reinterpret_cast<void(__thiscall*)(void*)>(v[13])(p);
    }
    ~Surface() { if(pixels) { auto v=*reinterpret_cast<uintptr_t**>(object);reinterpret_cast<void(__thiscall*)(void*)>(v[13])(object); } }
    U16* Row(int y) {return reinterpret_cast<U16*>(pixels+y*pitch);}
    void Fill(Rect r,U16 color) {
        if(!pixels)return;
        for(int y=std::max(0,r.y);y<std::min(height,r.y+r.h);++y)
            for(int x=std::max(0,r.x);x<std::min(width,r.x+r.w);++x)Row(y)[x]=color;
    }
};
inline U16 Color(unsigned rgb) {return U16(((rgb>>19)&31)<<11|((rgb>>10)&63)<<5|((rgb>>3)&31));}
struct TextCanvas {
    HDC dc=nullptr; HBITMAP bitmap=nullptr; HGDIOBJ oldBitmap=nullptr,oldFont=nullptr;
    HFONT font=nullptr; U32* pixels=nullptr; int width=0;
    ~TextCanvas(){Release();}
    void Release(){if(dc){SelectObject(dc,oldFont);SelectObject(dc,oldBitmap);DeleteObject(font);DeleteObject(bitmap);DeleteDC(dc);}dc=nullptr;}
    bool Ensure(int w) {
        if(dc&&w==width)return true;
        Release(); width=w;dc=CreateCompatibleDC(nullptr);
        BITMAPINFO b{};b.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);b.bmiHeader.biWidth=w;b.bmiHeader.biHeight=-68;
        b.bmiHeader.biPlanes=1;b.bmiHeader.biBitCount=32;b.bmiHeader.biCompression=BI_RGB;
        bitmap=CreateDIBSection(dc,&b,DIB_RGB_COLORS,reinterpret_cast<void**>(&pixels),nullptr,0);
        if(!bitmap){DeleteDC(dc);dc=nullptr;return false;}
        oldBitmap=SelectObject(dc,bitmap);
        font=CreateFontW(-22,0,0,0,FW_MEDIUM,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Microsoft JhengHei");
        oldFont=SelectObject(dc,font);SetBkMode(dc,TRANSPARENT);return true;
    }
    void Text(int x,int w,const std::wstring& text,COLORREF color=RGB(242,231,198)) {
        RECT r{x,0,x+w,68};SetTextColor(dc,color);DrawTextW(dc,text.c_str(),int(text.size()),&r,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
    }
};
static TextCanvas canvas;
inline bool Mission(void* target) {return target&&I(target,0xc)==1920&&I(target,0x10)==1080;}
inline std::wstring Value(void* top,int slot,int offset=0xa21) {
    void* control=P(top,slot);
    return control&&I(control,0x34)?std::to_wstring(I(control,offset)):L"--";
}
struct Geometry {Rect original{},modern{};};
static std::map<void*,Geometry> geometry;
static void RestoreGeometry(void* p) {
    auto it=geometry.find(p);
    if(it!=geometry.end()&&Equal(R(p),it->second.modern))SetRect(p,it->second.original);
}
static void ModernGeometry(void* p,Rect r) {
    geometry[p]={R(p),r};SetRect(p,r);
}
static void* resourceIcons[6]{};
static void LoadResourceIcons() {
    // The old popup owns a lazily loaded icon array. Keep a separate retained
    // set so opening/closing either popup cannot delete the persistent icons.
    for(int i=0;i<6;++i)if(!resourceIcons[i]) {
        void* resource=reinterpret_cast<void*(__thiscall*)(void*,int)>(0x4bcc30)(reinterpret_cast<void*>(0x76bd48),0x20e4+i);
        if(resource)reinterpret_cast<int(__cdecl*)(void*,void**)>(0x642130)(resource,&resourceIcons[i]);
    }
}
static void ScalePixels(Surface& surface,const std::vector<U16>& pixels,int sw,int sh,Rect dest) {
    for(int y=0;y<dest.h;++y)for(int x=0;x<dest.w;++x)
        surface.Row(dest.y+y)[dest.x+x]=pixels[(y*sh/dest.h)*sw+x*sw/dest.w];
}
static int __fastcall DrawTop(void* top,void*,void* target) {
    if(!Mission(target)||!canvas.Ensure(1920))return OriginalTop(top,target);
    const int utilitySlots[]={0x84,0x94,0x9c};
    for(int slot:utilitySlots)if(P(top,slot))RestoreGeometry(P(top,slot));
    // The stock popup is redundant now that its resources are always visible.
    if(I(top,0x15d)==1){I(top,0x15d)=0;I(top,0x161)=0;}
    I(top,0x38)=1;
    for(int slot:utilitySlots)if(P(top,slot))I(P(top,slot),0x38)=1;
    int result=OriginalTop(top,target);
    if(P(top,0x98))I(P(top,0x98),0x34)=0;
    void* world=*reinterpret_cast<void**>(0x8e02e7);
    if(!world)return result;
    int index=I(world,0x4e),count=*reinterpret_cast<int*>(0x771308);
    auto territory=*reinterpret_cast<U8**>(0x77130c);
    if(!territory)return result;
    territory+=(index>=0&&index<count)?index*98981:0;
    int cityIndex=I(territory,0x624);
    U8* city=nullptr;
    if(cityIndex>=0&&cityIndex<*reinterpret_cast<int*>(0x8b57f5))city=reinterpret_cast<U8**>(0x8b7905)[cityIndex];
    if(city && city[0x1b1]!=*reinterpret_cast<int*>(0x8b57ed))city=nullptr;
    auto name=reinterpret_cast<wchar_t*>(static_cast<U8*>(top)+0x1a9);
    bool chinese=name[0]>127;
    for(int i=0;i<1920*68;++i)canvas.pixels[i]=0x252b2e;
    canvas.Text(16,170,std::wstring(name,wcsnlen_s(name,100)),RGB(255,212,112));
    canvas.Text(200,175,(chinese?L"人口 ":L"Pop. ")+Value(top,0xa8));
    canvas.Text(380,135,(chinese?L"人數 ":L"People ")+Value(top,0xb0)+L"/"+Value(top,0xb0,0xa25));
    canvas.Text(520,140,(chinese?L"金錢 ":L"Gold ")+Value(top,0x90),RGB(255,212,112));
    const int start=680,cell=174;
    for(int i=0;i<6;++i) {
        int x=start+i*cell;
        for(int y=18;y<50;++y)canvas.pixels[y*1920+x-8]=0x616460;
        auto value=city?std::to_wstring(*reinterpret_cast<U16*>(city+0x30f+2*i)):L"--";
        canvas.Text(x+42,cell-48,value);
    }
    GdiFlush();
    {
        Surface surface(target);if(!surface.pixels)return result;
        std::array<std::vector<U16>,3> utilityPixels;
        std::array<Rect,3> old{};
        for(int i=0;i<3;++i)if(P(top,utilitySlots[i])) {
            auto control=P(top,utilitySlots[i]);old[i]=R(control);auto r=old[i];
            if(r.w<1||r.h<1||r.w>48||r.h>48||r.x<0||r.y<0||r.x+r.w>1920||r.y+r.h>68)continue;
            utilityPixels[i].resize(r.w*r.h);
            for(int y=0;y<r.h;++y)for(int x=0;x<r.w;++x)utilityPixels[i][y*r.w+x]=surface.Row(r.y+y)[r.x+x];
        }
        for(int y=0;y<68;++y)for(int x=0;x<1920;++x)surface.Row(y)[x]=Color(canvas.pixels[y*1920+x]);
        surface.Fill({0,67,1920,1},Color(0xa88e52));
        for(int i=0;i<3;++i)if(!utilityPixels[i].empty()) {
            Rect hit{1744+i*56,12,48,44};surface.Fill(hit,Color(0x434b4d));
            ScalePixels(surface,utilityPixels[i],old[i].w,old[i].h,{hit.x+6,hit.y+10,36,24});
            ModernGeometry(P(top,utilitySlots[i]),hit);
        }
    }
    LoadResourceIcons();
    for(int i=0;i<6;++i)if(resourceIcons[i]) {
        auto icon=resourceIcons[i];auto v=*reinterpret_cast<uintptr_t**>(icon);int x=start+i*cell+5;
        reinterpret_cast<void(__thiscall*)(void*,void*,int,int,int,void*)>(v[4])(icon,target,x,21,0,nullptr);
        Surface surface(target);
        int w=I(icon,0xc),h=I(icon,0x10);
        if(surface.pixels&&w>0&&h>0&&w<=28&&h<=28){
            std::vector<U16> pixels(w*h);
            for(int y=0;y<h;++y)for(int sx=0;sx<w;++sx)pixels[y*w+sx]=surface.Row(21+y)[x+sx];
            ScalePixels(surface,pixels,w,h,{x,21,28,28});
        }
    }
    return result;
}
static void* NativeHudArtwork(){void* hud=*reinterpret_cast<void**>(0x8e02e3);return hud?P(hud,0x50):nullptr;}
static void* (*HudArtwork)()=&NativeHudArtwork;
static void PanelArtwork(void* artwork,void* target,Rect dest,int sx,int sy) {
    if(!artwork)return;
    auto v=*reinterpret_cast<uintptr_t**>(artwork);
    using Blit=void(__thiscall*)(void*,void*,int,int,int,int,int,int,int,void*);
    // Native rectangles include a cached area after x/y/width/height.
    int bounds[]={0,0,1920,1080,1920*1080};
    reinterpret_cast<Blit>(v[3])(artwork,target,dest.x,dest.y,dest.w,dest.h,sx,sy,2,bounds);
}
static int __fastcall DrawActions(void* panel,void*,void* target,void* clip) {
    if(!Mission(target))return OriginalActions(panel,target,clip);
    RestoreGeometry(panel);
    int count=I(panel,0x74);
    if(count<0||count>32)return OriginalActions(panel,target,clip);
    for(int i=0;i<count;++i){void* child=P(panel,0x78+i*4);if(child)RestoreGeometry(child);}
    Rect native=R(panel);
    if(native.x<1500 || native.w!=86 || native.h!=154)return OriginalActions(panel,target,clip);
    void* artwork=HudArtwork();
    // Transparent native button pixels must start on clean backing, not on a
    // previously enlarged icon left in this shared part of the render target.
    PanelArtwork(artwork,target,native,480,44);
    // Preserve the original control rendering and command handling, then enlarge
    // their visible pixels and hit rectangles together. Each icon remains the
    // actual enabled/disabled/hover state drawn by the game.
    I(panel,0x438)=0; // Retire the old sliding curtain; it overwrites captured icons during selection changes.
    I(panel,0x38)=1;
    for(int i=0;i<count;++i){void* child=P(panel,0x78+i*4);if(child)I(child,0x38)=1;}
    int result=OriginalActions(panel,target,clip);
    struct Button {void* object;Rect original;std::vector<U16> pixels;};
    std::vector<Button> buttons;
    {
    Surface surface(target);if(!surface.pixels)return result;
    for(int i=0;i<count;++i) {
        void* child=P(panel,0x78+i*4);if(!child||!I(child,0x34))continue;
        Rect r=R(child);
        if(r.w<1||r.h<1||r.w>64||r.h>64||r.x<0||r.y<0||r.x+r.w>1920||r.y+r.h>1080)continue;
        Button b{child,r,{}};b.pixels.resize(r.w*r.h);
        for(int y=0;y<r.h;++y)for(int x=0;x<r.w;++x)b.pixels[y*r.w+x]=surface.Row(r.y+y)[r.x+x];
        buttons.push_back(std::move(b));
    }
    }
    if(buttons.size()>9)return result;
    // Compose the widened command panel from the game's own original artwork.
    PanelArtwork(artwork,target,{1466,873,10,207},740,0);
    PanelArtwork(artwork,target,{1476,873,250,207},480,0);
    PanelArtwork(artwork,target,{1726,873,10,207},740,0);
    Surface surface(target);if(!surface.pixels)return result;
    for(size_t i=0;i<buttons.size();++i) {
        // The original artwork has a 44px header and a bottom frame. Fit all
        // three rows inside its parchment area, including their click targets.
        auto& b=buttons[i];int x=1474+int(i%3)*86,y=918+int(i/3)*50;
        Rect hit{x,y,80,50};
        int size=48,dx=x+16,dy=y+1;
        for(int sy=0;sy<size;++sy)for(int sx=0;sx<size;++sx)
            surface.Row(dy+sy)[dx+sx]=b.pixels[(sy*b.original.h/size)*b.original.w+sx*b.original.w/size];
        ModernGeometry(b.object,hit);
    }
    ModernGeometry(panel,{1466,873,270,207});
    return result;
}
static void Install() {
    static bool done=false;if(done)return;
    if(GetModuleHandleW(nullptr)!=(HMODULE)0x400000)return;
    MEMORY_BASIC_INFORMATION info{};
    if(!VirtualQuery(reinterpret_cast<void*>(0xc82020),&info,sizeof(info))||info.State!=MEM_COMMIT)return;
    if(memcmp(reinterpret_cast<void*>(0xc82020),"DTMODERN1",9))return;
    struct Patch {uintptr_t slot,expected,replacement;};
    Patch patches[]={{0x68e100,0x56e4e0,reinterpret_cast<uintptr_t>(&DrawTop)},
                     {0x68e490,0x57c3c0,reinterpret_cast<uintptr_t>(&DrawActions)}};
    for(auto p:patches)if(*reinterpret_cast<uintptr_t*>(p.slot)!=p.expected)return;
    for(auto p:patches){DWORD old=0;if(!VirtualProtect(reinterpret_cast<void*>(p.slot),4,PAGE_READWRITE,&old))return;
        *reinterpret_cast<uintptr_t*>(p.slot)=p.replacement;DWORD ignored=0;VirtualProtect(reinterpret_cast<void*>(p.slot),4,old,&ignored);}
    done=true;
}
}
