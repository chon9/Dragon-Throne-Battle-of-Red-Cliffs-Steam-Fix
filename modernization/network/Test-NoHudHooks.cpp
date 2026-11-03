// Exercise the real shipped DLL without connecting to Steam. Even if an old
// executable advertises DTMODERN1, constructing DirectPlay must leave HUD
// vtables untouched. The old replacement DLL deliberately fails this test.
#define WIN32_LEAN_AND_MEAN
#include <initguid.h>
#include <windows.h>
#ifndef DUMMYUNIONNAME1
#define DUMMYUNIONNAME1
#endif
#include "vendor/directplay/dplay.h"
#include <cstdio>
#include <cstring>
#include <vector>
// Reserve the test addresses inside our own image before CRT heaps or DLLs can
// occupy them. This is BSS, so the executable does not contain a 9 MB payload.
static volatile unsigned char addressReserve[0x900000];
#define CHECK(x) do { if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);return 1;} } while(0)
int main(int argc,char** argv) {
    CHECK(GetModuleHandleW(nullptr)==reinterpret_cast<HMODULE>(0x400000));
    CHECK(reinterpret_cast<size_t>(addressReserve)<0x680000);
    CHECK(reinterpret_cast<size_t>(addressReserve)+sizeof(addressReserve)>0xc90000);
    auto table=reinterpret_cast<void*>(0x680000);
    *reinterpret_cast<unsigned*>(0x68e100)=0x56e4e0;
    *reinterpret_cast<unsigned*>(0x68e490)=0x57c3c0;
    memcpy(reinterpret_cast<void*>(0xc82020),"DTMODERN1",9);
    std::vector<unsigned char> before(0x10000);
    memcpy(before.data(),table,before.size());
    DWORD old=0;
    CHECK(VirtualProtect(table,0x10000,PAGE_READONLY,&old));
    HMODULE dll=LoadLibraryA(argc>1?argv[1]:"DTNet.dll");CHECK(dll);
    using Factory=HRESULT(WINAPI*)(REFCLSID,LPUNKNOWN,DWORD,REFIID,void**);
    auto create=reinterpret_cast<Factory>(GetProcAddress(dll,"CoCreateInstance"));CHECK(create);
    for(int i=0;i<10;++i) {
        IDirectPlay4* dp=nullptr;
        CHECK(create(CLSID_DirectPlay,nullptr,CLSCTX_INPROC_SERVER,IID_IDirectPlay4A,reinterpret_cast<void**>(&dp))==S_OK);
        CHECK(dp);dp->Release();
        CHECK(!memcmp(table,before.data(),before.size()));
    }
    FreeLibrary(dll);
    CHECK(VirtualProtect(table,0x10000,old,&old));
    puts("PASS: real adapter creates/releases DirectPlay without changing HUD vtables, even with the legacy modern-HUD marker");
    return 0;
}
