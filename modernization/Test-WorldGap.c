/* Execute the generated x86 hit guards, not a reimplementation of them.
 * Map the game image as data/code only; never run its entry point or imports. */
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#define CHECK(x) do { if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);return 1;} } while(0)
typedef int (__attribute__((thiscall)) *Hit)(void*);
typedef int (__attribute__((thiscall)) *PointHit)(void*,int*);
static volatile unsigned char reserved_game[0xc00000];
static void put(uintptr_t a,int v){memcpy((void*)a,&v,4);}
static int *field(unsigned char *p,int offset){return (int*)(p+offset);}
int main(int argc,char **argv){
    CHECK(argc==2);
    FILE *f=fopen(argv[1],"rb");CHECK(f);fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);
    unsigned char *raw=malloc(n);CHECK(raw);CHECK(fread(raw,1,n,f)==(size_t)n);fclose(f);
    IMAGE_DOS_HEADER *dos=(void*)raw;IMAGE_NT_HEADERS32 *nt=(void*)(raw+dos->e_lfanew);
    CHECK(nt->OptionalHeader.ImageBase==0x400000);
    /* Only the tested routines/globals are needed; do not reserve the 60MB
       font atlas, which can overlap this test process's loader allocations. */
    uintptr_t pages[]={0x560000,0x5d0000,0x8b0000,0x8e0000,0xc70000,0xc80000};
    CHECK((uintptr_t)reserved_game<0x560000 && (uintptr_t)reserved_game+sizeof(reserved_game)>=0xc90000);
    for(int i=0;i<6;++i){DWORD old;CHECK(VirtualProtect((void*)pages[i],0x10000,PAGE_EXECUTE_READWRITE,&old));}
    IMAGE_SECTION_HEADER *sec=IMAGE_FIRST_SECTION(nt);
    for(int i=0;i<nt->FileHeader.NumberOfSections;++i)for(int j=0;j<6;++j){
        uintptr_t first=0x400000+sec[i].VirtualAddress,last=first+sec[i].SizeOfRawData;
        uintptr_t a=first>pages[j]?first:pages[j],b=last<pages[j]+0x10000?last:pages[j]+0x10000;
        if(a<b)memcpy((void*)a,raw+sec[i].PointerToRawData+a-first,b-a);
    }
    /* Ensure the old cursor-dependent render wrapper really is gone. */
    CHECK(!memcmp((void*)0x5d5fd0,"\x53\x55\x56\x8b\xf1",5));
    /* Stand in only for downstream popup queries; native world bounds execute.
       Mark that accepted input reached the existing popup-check continuation. */
    unsigned char tail[]={0xc7,0x05,0,0,0,0,1,0,0,0,0x5f,0xb8,1,0,0,0,0x5e,0xc2,4,0};
    int delegated=0;uint32_t address=(uint32_t)(uintptr_t)&delegated;memcpy(tail+2,&address,4);
    memcpy((void*)0x56221c,tail,sizeof(tail));FlushInstructionCache(GetCurrentProcess(),NULL,0);
    unsigned char world[256]={0},before[256];
    *field(world,0)=0x68fda8;*field(world,8)=0;*field(world,12)=20;
    *field(world,16)=1920;*field(world,20)=1060;*field(world,24)=1920*1060;
    put(0x8e02e7,(int)(uintptr_t)world);memcpy(before,world,sizeof(world));
    Hit hit=(Hit)0x560670;PointHit point_hit=(PointHit)0x5621e0;
    int ys[]={-1,0,19,20,872,873,874,1000,1079,1080};
    int calls=0;
    for(int j=0;j<(int)(sizeof(ys)/sizeof(ys[0]));++j)for(int x=-1;x<=1920;++x){
        int y=ys[j],pt[]={x,y};put(0x8b9d60,x);put(0x8b9d64,y);
        int expected=x>=0&&x<1920&&y>=20&&y<1080&&(y<873||(x>=750&&x<1636));
        CHECK(hit(world)==expected);delegated=0;
        CHECK(point_hit(world,pt)==expected);CHECK(delegated==expected);
        CHECK(!memcmp(before,world,sizeof(world)));++calls;
    }
    /* Legacy display modes and other control classes retain rectangular hits. */
    *field(world,16)=800;*field(world,20)=580;put(0x8b9d60,799);put(0x8b9d64,599);
    int pt[]={799,599};CHECK(hit(world)==1);CHECK(point_hit(world,pt)==1);
    *field(world,16)=1920;*field(world,20)=1060;*field(world,0)=0x68e01c;
    put(0x8b9d60,1800);put(0x8b9d64,1000);CHECK(hit(world)==1);
    printf("PASS: %d boundary points through both real x86 guards; full world geometry immutable; panel hits excluded; native popup continuation and legacy controls preserved\n",calls);
    free(raw);return 0;
}
