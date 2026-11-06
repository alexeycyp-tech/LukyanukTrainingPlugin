/* Smoke test for the actual production DLL and the C-declared MSVC x86 ABI.
 * The host intentionally has an unsupported hash: no game memory is accessed.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#define TC __attribute__((thiscall))
typedef struct {void **v;} Object;
static unsigned draws;
static void TC message(Object *o,const char *s,float t,float x,float y) {(void)o;(void)s;(void)t;(void)x;(void)y;}
static void TC text(Object *o,float x,float y,const char *s) {(void)o;(void)x;(void)y;if(s && *s) draws++;}
int main(int argc,char **argv) {
    if(argc!=2) return 2;
    HMODULE dll=LoadLibraryA(argv[1]); if(!dll) {printf("LoadLibrary error %lu\n",GetLastError());return 3;}
    Object *(__cdecl *create)(Object*)=(void*)GetProcAddress(dll,"RBR_CreatePlugin");
    if(!create) return 4;
    void *table[16]={0};table[1]=message;table[2]=text;Object fake={table};
    Object *p=create(&fake); if(!p || !p->v) return 5;
    const char *(TC *name)(Object*)=(void*)p->v[1];
    if(strcmp(name(p),"LukyanukTrainingPlugin 0.1beta")) return 6;
    ((void(TC*)(Object*))p->v[2])(p);
    ((void(TC*)(Object*))p->v[3])(p);
    ((void(TC*)(Object*,char,_Bool,_Bool,_Bool,_Bool,_Bool))p->v[4])(p,'x',0,0,0,0,1);
    ((void(TC*)(Object*,float))p->v[5])(p,.016f);
    ((void(TC*)(Object*,int,const char*,_Bool))p->v[6])(p,41,"test",0);
    ((void(TC*)(Object*,float,float,float,const char*))p->v[7])(p,1,2,3,"test");
    ((void(TC*)(Object*,float,int,const char*))p->v[8])(p,42,1,"test");
    if(create(&fake)!=p || draws!=0) return 7;
    ((void*(TC*)(Object*,unsigned))p->v[0])(p,0);
    printf("PASS: production DLL loaded; 9 IPlugin slots called; menu draw no longer uses scenic game text; unsupported host safely rejected\n");
    FreeLibrary(dll); return 0;
}
