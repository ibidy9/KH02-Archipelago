#ifndef KH02_RESTORE_HOOK_CANDIDATE_H
#define KH02_RESTORE_HOOK_CANDIDATE_H
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "kh02_combined_route_candidate.h"

/* Side-by-side save observer only. Existing proven hook owners are unchanged.
   Caller must serialize installation on the proven owner thread and enforce
   whole-call flight/fault refusal before restore. No live export or launcher. */
typedef struct Kh02RestoreHook {
    unsigned char *target,*trampoline;
    RUNTIME_FUNCTION *unwind_table;
    unsigned char original[32],patch[32];
    DWORD owner_thread,protection;
    uint32_t size,prepared,registered,owned;
} Kh02RestoreHook;
enum { KH02_SAVE_LOAD,KH02_SAVE_STAGE,KH02_SAVE_COPY,KH02_SAVE_CAPTURE,KH02_SAVE_REFRESH,KH02_SAVE_QUEUE,KH02_SAVE_RESTORE };
uint32_t Kh02RestoreHookPrepare(Kh02RestoreHook*,unsigned char*,uint32_t,void*);
uint32_t Kh02RestoreHookPublish(Kh02RestoreHook*);
uint32_t Kh02RestoreHookRestore(Kh02RestoreHook*);
#ifdef KH02_RESTORE_HOOK_SELFTEST
extern uint32_t Kh02RestoreHookTestFailRegistration,Kh02RestoreHookTestPublishCountdown;
#endif
#endif
