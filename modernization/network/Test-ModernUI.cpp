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
        assert(r.w==35&&r.h==35&&r.x==1648+(i%2)*39&&r.y==904+(i/2)*39);
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
    std::array<U8,0x60> background{};
    I(background.data(),0)=0x68a68c;I(background.data(),0xc)=35;I(background.data(),0x10)=35;
    SetRect(panel.data(),{1647,903,86,154});I(panel.data(),0x74)=8;
    for(int i=0;i<8;++i){auto p=children[i].data();SetRect(p,{1648+(i%2)*39,904+(i/2)*39,35,35});
        I(p,0)=0x68d7c8;*reinterpret_cast<void**>(p+0x54)=background.data();
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
    // Regression: the game rebuilds the action list by updating x/y only.
    // Enlarged 80x50 hit dimensions must never reach its 35x35 bitmap reader.
    for(int pass=0;pass<20;++pass) {
        SetRect(panel.data(),{1647,903,86,154});
        for(int i=0;i<8;++i) {
            auto child=children[i].data();
            I(child,8)=1648+(i%2)*39;I(child,0xc)=904+(i/2)*39;
            I(child,0x34)=1;
        }
        assert(DrawActions(panel.data(),nullptr,target.data(),nullptr)==7);
        assert(locks==0);
        for(int i=0;i<8;++i) {
            auto r=R(children[i].data());
            assert(r.w==80&&r.h==50);
            assert(pixels[(r.y+10)*1920+r.x+20]==0x1000+i);
        }
    }
    // Exact two-button rebuild observed in the frozen mission: new native
    // positions, but old enlarged dimensions survived list reconstruction.
    I(panel.data(),0x74)=2;SetRect(panel.data(),{1647,903,86,154});
    for(int i=0;i<2;++i){I(children[i].data(),8)=1648;I(children[i].data(),0xc)=904+i*54;}
    assert(DrawActions(panel.data(),nullptr,target.data(),nullptr)==7&&locks==0);
    // Appointment regression: an untracked control inherits a modern rectangle.
    // Pointer-keyed restoration cannot help this newly introduced button.
    std::array<U8,0x100> appointment{};
    memcpy(appointment.data(),children[0].data(),appointment.size());
    SetRect(appointment.data(),{1646,918,80,50});
    *reinterpret_cast<void**>(panel.data()+0x80)=appointment.data();
    I(panel.data(),0x74)=3;
    SetRect(panel.data(),{1647,903,86,154});
    assert(geometry.find(appointment.data())==geometry.end());
    assert(DrawActions(panel.data(),nullptr,target.data(),nullptr)==7&&locks==0);
    auto ap=R(appointment.data());
    assert(ap.x==1646&&ap.y==918&&ap.w==80&&ap.h==50);
    assert(pixels[(ap.y+10)*1920+ap.x+20]==0x1002);
    // A genuine native resize must survive restoration of the other fields.
    auto child=children[0].data();
    I(child,8)=1651;I(child,0x10)=32;
    RestoreGeometry(child);
    assert(R(child).x==1651&&R(child).y==904&&R(child).w==32&&R(child).h==35);
    assert(I(child,0x18)==32*35);
    RestoreGeometry(child);
    assert(R(child).w==32&&R(child).h==35);
    assert(Color(0xffffff)==0xffff&&Color(0)==0);
    puts("PASS: appointment/untracked button recovery, nonoverlapping native capture, list rebuilds, partial geometry changes, surface locks and panel bounds");
}
