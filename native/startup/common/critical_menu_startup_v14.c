/* v14: capability 0x20 exposes Critical; 0x10 is carryover. See docs/critical-real-gate-2026-09-16.md. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <string.h>
#include "critical_menu_startup.h"
#define TARGET_RVA 0x560560u
#define PATCH_SIZE 15u
#define FLAGS_OFFSET 0x578u
typedef void (__fastcall *Refresh)(void *object);
static const unsigned char expected[PATCH_SIZE]={0x48,0x8b,0xc4,0x48,0x83,0xec,0x38,0x48,0x89,0x58,0x08,0x48,0x8d,0x50,0xe8};
static unsigned char original[PATCH_SIZE],patch[PATCH_SIZE],*target;static void *trampoline;
static Refresh original_refresh;static Kh02StartupCriticalPermit permit_fn;static void *permit_context;
static Kh02StartupCriticalEvidence *telemetry;static volatile LONG installed,enabled,flight;
static void absolute_jump(unsigned char *at,const void *destination){uint64_t value=(uint64_t)(uintptr_t)destination;at[0]=0xff;at[1]=0x25;memset(at+2,0,4);memcpy(at+6,&value,8);}
static void __fastcall refresh(void *object){uint32_t before,after;InterlockedIncrement(&flight);original_refresh(object);
    if(telemetry)InterlockedIncrement(&telemetry->calls);
    if(object && InterlockedCompareExchange(&enabled,0,0) && permit_fn && permit_fn(permit_context)){
        volatile LONG *flags=(volatile LONG*)((unsigned char*)object+FLAGS_OFFSET);
        before=(uint32_t)InterlockedCompareExchange(flags,0,0);after=(uint32_t)InterlockedOr(flags,0x20)|0x20;
        if(telemetry){InterlockedExchange(&telemetry->last_before,(LONG)before);InterlockedExchange(&telemetry->last_after,(LONG)after);InterlockedIncrement(&telemetry->permitted);if(!(before&0x20))InterlockedIncrement(&telemetry->changed);}
    }InterlockedDecrement(&flight);
}
static uint32_t install_at(unsigned char *candidate,Kh02StartupCriticalPermit permit,void *context,Kh02StartupCriticalEvidence *evidence){unsigned char *copy;DWORD old,ignored;
    if(!candidate||!permit||!evidence||InterlockedCompareExchange(&installed,0,0)||memcmp(candidate,expected,sizeof(expected)))return 0;
    copy=(unsigned char*)VirtualAlloc(NULL,PATCH_SIZE+14,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);if(!copy)return 0;
    target=candidate;memcpy(original,target,PATCH_SIZE);memcpy(copy,original,PATCH_SIZE);absolute_jump(copy+PATCH_SIZE,target+PATCH_SIZE);
    memset(patch,0x90,sizeof(patch));absolute_jump(patch,refresh);trampoline=copy;original_refresh=(Refresh)copy;permit_fn=permit;permit_context=context;telemetry=evidence;
    if(!VirtualProtect(target,PATCH_SIZE,PAGE_EXECUTE_READWRITE,&old)){trampoline=NULL;original_refresh=NULL;permit_fn=NULL;telemetry=NULL;VirtualFree(copy,0,MEM_RELEASE);return 0;}
    memcpy(target,patch,PATCH_SIZE);FlushInstructionCache(GetCurrentProcess(),target,PATCH_SIZE);VirtualProtect(target,PATCH_SIZE,old,&ignored);
    InterlockedExchange(&installed,1);InterlockedExchange(&enabled,1);InterlockedExchange(&evidence->installed,1);return 1;
}
uint32_t Kh02StartupCriticalInstall(unsigned char *image,Kh02StartupCriticalPermit permit,void *context,Kh02StartupCriticalEvidence *evidence){IMAGE_DOS_HEADER *dos;IMAGE_NT_HEADERS64 *nt;
    if(!image)return 0;dos=(IMAGE_DOS_HEADER*)image;if(dos->e_magic!=IMAGE_DOS_SIGNATURE)return 0;nt=(IMAGE_NT_HEADERS64*)(image+dos->e_lfanew);
    if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->OptionalHeader.SizeOfImage!=0x04f5f000u)return 0;return install_at(image+TARGET_RVA,permit,context,evidence);
}
uint32_t Kh02StartupCriticalRemove(void){DWORD old,ignored;unsigned spins;InterlockedExchange(&enabled,0);if(!InterlockedCompareExchange(&installed,0,0))return 1;
    if(!target||memcmp(target,patch,PATCH_SIZE)||!VirtualProtect(target,PATCH_SIZE,PAGE_EXECUTE_READWRITE,&old))return 0;
    memcpy(target,original,PATCH_SIZE);FlushInstructionCache(GetCurrentProcess(),target,PATCH_SIZE);VirtualProtect(target,PATCH_SIZE,old,&ignored);InterlockedExchange(&installed,0);if(telemetry)InterlockedExchange(&telemetry->installed,0);
    for(spins=0;spins<10000&&InterlockedCompareExchange(&flight,0,0);spins++)Sleep(0);if(InterlockedCompareExchange(&flight,0,0))return 0;
    VirtualFree(trampoline,0,MEM_RELEASE);target=NULL;trampoline=NULL;original_refresh=NULL;permit_fn=NULL;permit_context=NULL;telemetry=NULL;return 1;
}
#ifdef KH02_STARTUP_CRITICAL_SELFTEST
static uint32_t test_allowed;static uint32_t test_permit(void *unused){(void)unused;return test_allowed;}
__declspec(dllexport) uint32_t Kh02StartupCriticalTestInstall(void *candidate,uint32_t allowed,Kh02StartupCriticalEvidence *evidence){test_allowed=allowed;return install_at(candidate,test_permit,NULL,evidence);}
__declspec(dllexport) void Kh02StartupCriticalTestPermit(uint32_t allowed){test_allowed=allowed;}
__declspec(dllexport) uint32_t Kh02StartupCriticalTestRemove(void){return Kh02StartupCriticalRemove();}
#endif
