#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

struct World { int x0,x1,x2,top; };
struct Case { unsigned char base,destination,patch[3]; };
int main() {
    // ModRM bytes for mov register,[esp+offset], matching each patch's live registers.
    Case cases[]={{0x6c,0x44,{0x2b,0x45,0x0c}},{0x7c,0x44,{0x2b,0x47,0x0c}},
                  {0x74,0x54,{0x2b,0x56,0x0c}},{0x74,0x44,{0x2b,0x46,0x0c}},
                  {0x44,0x7c,{0x2b,0x78,0x0c}}};
    for(auto c:cases) {
        std::vector<unsigned char> code={0x55,0x56,0x57,0x8b,c.base,0x24,0x10,
                                         0x8b,c.destination,0x24,0x14};
        code.insert(code.end(),c.patch,c.patch+3);
        if(c.destination!=0x44){code.push_back(0x8b);code.push_back(c.destination==0x54?0xc2:0xc7);}
        code.insert(code.end(),{0x5f,0x5e,0x5d,0xc3});
        void* memory=VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);assert(memory);
        memcpy(memory,code.data(),code.size());DWORD old;
        assert(VirtualProtect(memory,4096,PAGE_EXECUTE_READ,&old));FlushInstructionCache(GetCurrentProcess(),memory,code.size());
        auto run=reinterpret_cast<int(__cdecl*)(World*,int)>(memory);
        for(int top:{20,68,0,96})for(int camera:{0,32,511,2048,-32}) {
            World w{0x11223344,0x22334455,0x33445566,top};World before=w;
            int origin=run(&w,camera);
            assert(origin==camera-top&&memcmp(&w,&before,sizeof(w))==0);
            // Dynamic overlap pass and cached terrain must project the same point.
            for(int point:{0,400,1200})assert(point-origin==point-camera+top);
            if(top==20)assert(origin==camera-20); // Legacy viewport stays identical.
            if(top==68)assert((camera-20)-origin==48); // Reproduce the old mismatch.
        }
        VirtualFree(memory,0,MEM_RELEASE);
    }
    puts("PASS: all five x86 origin replacements; legacy and modern alignment; unchanged world fields.");
}
