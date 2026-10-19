/* Additive session build: sample precise UTC against process creation time. */
/* First empty-folder title experiment. Filename only; no receipt/gameplay writes. */
#if defined(KH02_FILENAME_SELFTEST) && defined(KH02_FILENAME_LIVE)
#error Test exports must not be present in a live build
#endif
#if !defined(KH02_FILENAME_SELFTEST) && !defined(KH02_FILENAME_LIVE)
#error Explicit build mode required
#endif
#define DllMain Kh02ForwarderDllMain
#include "forwarder.c"
#undef DllMain
#include "startup_input.h"
#include "resume_authorization.h"
#include "provider_install.h"
#include "filename_evidence.h"
#include <string.h>
#include <wchar.h>
__declspec(dllexport) FilenameEvidence Kh02FilenameEvidence={.magic=0x4b484649,.version=1,.size=sizeof(FilenameEvidence)};
static Kh02StartupGate gate;
static Kh02ProviderInstall installation;
static HANDLE intent_handle,exe_handle;
static Kh02ResumeAuthorization resume;
static unsigned char *image_base;
static Kh02PathFormat native_format;
#ifdef KH02_FRESH_STORAGE_AUTH
static unsigned char fresh_setup_digest[32];
static volatile LONG fresh_storage_ready;
static uint32_t critical_permit(void *unused) {
    (void)unused;
    return gate.ready && gate.mode==KH02_LAB_AP &&
        InterlockedCompareExchange(&fresh_storage_ready,0,0)!=0;
}
/* The preparer uses SHA256(immutable setup bytes) as this launch's nonce.
   The transport must independently hash its owned copy before calling here.
   This authorizes storage membership only, never delivery/network activity. */
__declspec(dllexport) unsigned Kh02ResumeStorageCheck(uint64_t created,
    const unsigned char binding[32],const unsigned char digest[32]) {
    unsigned char b[32],d[32];SIZE_T got=0;
    if(!binding || !digest || !InterlockedCompareExchange(&fresh_storage_ready,0,0) ||
       !ReadProcessMemory(GetCurrentProcess(),binding,b,32,&got) || got!=32 ||
       !ReadProcessMemory(GetCurrentProcess(),digest,d,32,&got) || got!=32)return 0;
    return created==gate.created && gate.ready && gate.mode==KH02_LAB_AP &&
        installation.status==1 && !InterlockedCompareExchange(&Kh02FilenameEvidence.fault,0,0) &&
        !memcmp(b,gate.route.binding,32) && !memcmp(d,fresh_setup_digest,32);
}
#endif
__declspec(dllexport) const unsigned char Kh02FilenameSupportedBuild[32]={0xa5,0x94,0x5a,0xf5,0x04,0x10,0x47,0xf5,0x4d,0xb6,0x7d,0x76,0x67,0x39,0x0f,0x4d,0xc9,0x0e,0xee,0x59,0xd1,0xc0,0x64,0x59,0x43,0x4d,0xda,0xb5,0x7d,0x0f,0x33,0x9c};
static BOOL read_exact(const void *p,void *out,SIZE_T n) {
    SIZE_T got=0;return ReadProcessMemory(GetCurrentProcess(),p,out,n,&got)&&got==n;
}
/* An unsupported internal call has no native FString failure return. Never
   fabricate one or fall through to vanilla. The title-only experimental build
   raises a noncontinuable error; it does not close/launch another process. */
static _Noreturn void contract_fault(unsigned code) {
    InterlockedExchange(&Kh02FilenameEvidence.fault,(LONG)code);
    ULONG_PTR detail=code;RaiseException(0xe0420201,EXCEPTION_NONCONTINUABLE,1,&detail);
    __builtin_trap();
}
static Kh02PathText *filename(void *owner,Kh02PathText *out,const wchar_t *name) {
    uintptr_t caller=(uintptr_t)__builtin_return_address(0)-(uintptr_t)image_base;
    InterlockedIncrement(&Kh02FilenameEvidence.flight);
    if(caller!=0x52ce6f && caller!=0x5330ca && caller!=0x537bec && caller!=0x527068)contract_fault(1);
    if(owner!=image_base+0x4485688 || !out)contract_fault(2);
    wchar_t stable[12];
    if(!read_exact(name,stable,sizeof(stable)) ||
       (memcmp(stable,L"KH0.2BbS_WW",sizeof(stable)) && memcmp(stable,L"KH0.2BbS_JP",sizeof(stable))))contract_fault(3);
    LONG at=InterlockedIncrement(&Kh02FilenameEvidence.count)-1;
    FilenameEvent *event=at>=0 && at<32?&Kh02FilenameEvidence.events[at]:NULL;
    if(event){event->thread=GetCurrentThreadId();event->caller=caller;memcpy(event->name,stable,sizeof(stable));}
    else InterlockedIncrement(&Kh02FilenameEvidence.overflow);
    Kh02PathText *returned=NULL;
    unsigned result=Kh02StartupPath(&gate,gate.created,gate.route.binding,owner,out,stable,NULL,native_format,&returned);
    if(result!=KH02_PATH_REDIRECTED || returned!=out)contract_fault(4);
    Kh02PathText value;
    if(!read_exact(returned,&value,sizeof(value)) || value.count<=0 || value.count>256 || value.count>value.capacity)contract_fault(5);
    if(event) {
        if(!read_exact(value.data,event->path,(SIZE_T)value.count*sizeof(wchar_t)) || event->path[value.count-1])contract_fault(6);
        InterlockedExchange(&event->complete,1);
    }
    InterlockedDecrement(&Kh02FilenameEvidence.flight);return returned;
}
static BOOL install_filename(void *unused) {
    (void)unused;
    native_format=(Kh02PathFormat)(image_base+0xaa4cc0);
    unsigned char actual[15];
    const unsigned char prefix[15]={0x4c,0x8b,0xdc,0x49,0x89,0x53,0x10,0x4d,0x89,0x43,0x18,0x4d,0x89,0x4b,0x20};
    if(!read_exact((void*)native_format,actual,sizeof(actual)) || memcmp(actual,prefix,sizeof(prefix)))return FALSE;
    void *replacements[5]={image_base+0x52ce40,image_base+0x537440,image_base+0x533070,image_base+0x527030,filename};
    if(!Kh02StartupCriticalInstall(image_base,critical_permit,NULL,&Kh02FilenameEvidence.critical_menu))return FALSE;
    BOOL ok=Kh02InstallProvider(&installation,image_base,replacements,NULL);
    if(!ok && !Kh02StartupCriticalRemove())InterlockedExchange(&Kh02FilenameEvidence.fault,7);
    Kh02FilenameEvidence.install_status=installation.status;return ok;
}
static uint64_t time_value(FILETIME t){return ((uint64_t)t.dwHighDateTime<<32)|t.dwLowDateTime;}
static BOOL boot(HINSTANCE module,unsigned char *base) {
    if(Kh02FilenameEvidence.mode)return FALSE;
    Kh02FilenameEvidence.mode=KH02_LAB_BLOCKED;image_base=base;
    wchar_t path[1024],exe[1024];FILETIME created,exit_time,kernel,user,now;
    DWORD n=GetModuleFileNameW(module,path,1024);if(!n || n>=1024)return FALSE;
    wchar_t *slash=wcsrchr(path,L'\\');if(!slash || slash-path>980)return FALSE;
    wcscpy(slash+1,L"kh02-ap-ready.bin");
    if(!GetProcessTimes(GetCurrentProcess(),&created,&exit_time,&kernel,&user))return FALSE;
    GetSystemTimePreciseAsFileTime(&now);Kh02StartupRequest request;
    unsigned intent=Kh02ReadStartupInput(path,&request,&intent_handle);BOOL ok;
    if(intent==KH02_LAB_NO_INTENT) {
        ok=Kh02StartupBegin(&gate,intent,NULL,time_value(created),time_value(now),NULL,NULL,NULL);
    }else {
        if(intent!=KH02_LAB_AP_INTENT)return FALSE;
        n=GetModuleFileNameW(NULL,exe,1024);if(!n || n>=1024)return FALSE;
        unsigned char build[32];if(!Kh02HashStartupFile(exe,build,&exe_handle))return FALSE;
#ifdef KH02_FILENAME_LIVE
        if(memcmp(build,Kh02FilenameSupportedBuild,32))return FALSE;
#else
        (void)Kh02FilenameSupportedBuild;
#endif
        wcscpy(slash+1,L"kh02-ap-resume.bin");
        unsigned resume_mode=Kh02ReadResumeAuthorization(path,&request,&resume);
        if(resume_mode==KH02_LAB_INTENT_ERROR)return FALSE;
        ok=Kh02StartupBeginChecked(&gate,intent,&request,time_value(created),time_value(now),build,install_filename,NULL,
            resume_mode==KH02_LAB_AP_INTENT?Kh02CheckResumeRoot:NULL,&resume);
        Kh02ReleaseResumeFile(&resume);
#ifdef KH02_FRESH_STORAGE_AUTH
        /* Only an exact-file authenticated resume grants this candidate storage authority. */
        if(ok && resume_mode==KH02_LAB_AP_INTENT && gate.mode==KH02_LAB_AP) {
            memcpy(fresh_setup_digest,request.nonce,32);
            InterlockedExchange(&fresh_storage_ready,1);
        }
#endif
    }
    Kh02FilenameEvidence.startup_error=gate.error;
    if(ok) {
        Kh02FilenameEvidence.mode=gate.mode;Kh02FilenameEvidence.created=gate.created;
        memcpy(Kh02FilenameEvidence.binding,gate.route.binding,32);
        memcpy(Kh02FilenameEvidence.root,gate.route.root,sizeof(gate.route.root));
    }
    return ok;
}
__declspec(dllexport) unsigned Kh02FilenameMode(void){return Kh02FilenameEvidence.mode;}
#ifdef KH02_FILENAME_SELFTEST
__declspec(dllexport) BOOL Kh02FilenameTestBoot(HINSTANCE module,unsigned char *base){return boot(module,base);}
#endif
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID reserved) {
    (void)reserved;
#ifdef KH02_FILENAME_LIVE
    if(reason==DLL_PROCESS_ATTACH)return boot(module,(unsigned char*)GetModuleHandleW(NULL));
#else
    (void)module;(void)reason;
#endif
    return TRUE;
}
