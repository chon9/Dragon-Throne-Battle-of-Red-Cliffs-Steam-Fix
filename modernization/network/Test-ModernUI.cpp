#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <map>
#include <cstdio>
#include <cassert>
#include "ModernUI.h"
using namespace modernui;
static std::vector<U16> pixels(1920*1080);
static int locks=0;
static int artworkCalls=0;
static std::array<uintptr_t,4> artworkVtable{};
static uintptr_t* artwork=artworkVtable.data();
static void* TestArtwork(){return &artwork;}
static constexpr U16 backing=0x3145;
static void __fastcall Blit(void*,void*,void*,int x,int y,int w,int h,int sx,int sy,int flags,void* clip){
    assert(locks==0&&flags==2&&reinterpret_cast<int*>(clip)[4]==1920*1080);
    if(artworkCalls%4==0)assert(x==1647&&y==903&&w==86&&h==154&&sx==480&&sy==44);
    else {
        assert(y==873&&h==207&&sy==0);
        assert((x==1466&&w==10&&sx==740)||(x==1476&&w==250&&sx==480)||(x==1726&&w==10&&sx==740));
    }
    for(int py=y;py<y+h;++py)for(int px=x;px<x+w;++px)pixels[py*1920+px]=backing;
    ++artworkCalls;
}
static U8* __fastcall Lock(void*,void*,int){++locks;return reinterpret_cast<U8*>(pixels.data());}
static void __fastcall Unlock(void*,void*){--locks;}
static int __fastcall FakeDraw(void* p,void*,void*,void*) {
    for(int i=0;i<I(p,0x74);++i){void* child=P(p,0x78+i*4);Rect r=R(child);
        assert(r.w==35&&r.h==35&&r.x>=1647&&r.y>=903);
        if(!I(child,0x34))continue;
        // Leave a transparent rim to catch capture of the previous frame.
        for(int y=1;y<r.h-1;++y)for(int x=1;x<r.w-1;++x)pixels[(r.y+y)*1920+r.x+x]=U16(0x1000+i);
    }return 7;
}
int main(){
    std::array<U8,0x80> target{};std::array<uintptr_t,14> v{};
    v[12]=reinterpret_cast<uintptr_t>(&Lock);v[13]=reinterpret_cast<uintptr_t>(&Unlock);
    *reinterpret_cast<uintptr_t**>(target.data())=v.data();
    I(target.data(),0xc)=1920;I(target.data(),0x10)=1080;target[0x34]=target[0x35]=16;
    *reinterpret_cast<short*>(target.data()+0x4e)=2;I(target.data(),0x50)=1920;
    std::array<U8,0x440> panel{};std::array<std::array<U8,0x100>,8> children{};
    SetRect(panel.data(),{1647,903,86,154});I(panel.data(),0x74)=8;
    for(int i=0;i<8;++i){auto p=children[i].data();SetRect(p,{1648+(i%2)*39,904+(i/2)*39,35,35});
        I(p,0x34)=1;*reinterpret_cast<void**>(panel.data()+0x78+i*4)=p;}
    OriginalActions=reinterpret_cast<ActionDraw>(&FakeDraw);
    artworkVtable[3]=reinterpret_cast<uintptr_t>(&Blit);HudArtwork=&TestArtwork;
    const int visibleCounts[]={8,2,7,0};
    for(int pass=0;pass<4;++pass){
        int visible=visibleCounts[pass];
        for(int i=0;i<8;++i)I(children[i].data(),0x34)=i<visible;
        I(panel.data(),0x438)=90;
        assert(DrawActions(panel.data(),nullptr,target.data(),nullptr)==7);
        assert(locks==0&&R(panel.data()).x==1466);
        assert(artworkCalls==(pass+1)*4&&I(panel.data(),0x438)==0);
        for(int i=0;i<visible;++i){Rect r=R(children[i].data());
            assert(r.x==1474+i%3*86&&r.y==918+i/3*50&&r.w==80&&r.h==50);
            assert(pixels[(r.y+10)*1920+r.x+20]==0x1000+i);
            assert(pixels[(r.y+1)*1920+r.x+16]==backing);
            assert(r.x>=1474&&r.x+r.w<=1726&&r.y>=917&&r.y+r.h<=1068);
        }
        // Decoration and removed buttons are never overwritten or left stale.
        for(int y=873;y<917;++y)for(int x=1466;x<1736;++x)assert(pixels[y*1920+x]==backing);
        for(int i=visible;i<9;++i)assert(pixels[(928+i/3*50)*1920+1494+i%3*86]==backing);
    }
    assert(Color(0xffffff)==0xffff&&Color(0)==0);
    puts("PASS: action reflow keeps paint and hit boxes aligned, repeated draw restores native geometry, balanced surface locks and panel bounds");
}
