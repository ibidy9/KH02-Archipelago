#include "kh02_stat_popup_v111.h"
static Kh02StatPopupQueueV111 popup_queue_v111;
static uint32_t popup_servicing_v111;
static uint64_t popup_waits_v111;
static HANDLE popup_mapping_v111;
static uint64_t *popup_diag_v111;
extern uint32_t Kh02StatPopupNativeV111(uint32_t,void (*)(void));
extern uint32_t Kh02StatPopupOwnerStatusV111(void);
static uint64_t popup_cookie_v111(void) {
    return harness_life && harness_life->branch?harness_life->branch->scope.cookie:0;
}
static void popup_publish_v111(void) {
    if(!popup_diag_v111 && !popup_mapping_v111) {
        wchar_t name[96];
        _snwprintf_s(name,96,_TRUNCATE,L"Local\\KH02APStatPopupV111-%lu",GetCurrentProcessId());
        popup_mapping_v111=CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,128,name);
        if(popup_mapping_v111 && GetLastError()!=ERROR_ALREADY_EXISTS)
            popup_diag_v111=MapViewOfFile(popup_mapping_v111,FILE_MAP_ALL_ACCESS,0,0,128);
    }
    if(!popup_diag_v111)return;
    InterlockedIncrement64((LONG64*)&popup_diag_v111[1]);
    popup_diag_v111[0]=0x313131505453;popup_diag_v111[2]=128;
    popup_diag_v111[3]=popup_queue_v111.epoch;popup_diag_v111[4]=popup_queue_v111.cookie;
    popup_diag_v111[5]=popup_queue_v111.count;popup_diag_v111[6]=popup_queue_v111.admitted;
    popup_diag_v111[7]=popup_queue_v111.submitted;popup_diag_v111[8]=popup_queue_v111.stale;
    popup_diag_v111[9]=popup_queue_v111.overflow;popup_diag_v111[10]=popup_queue_v111.failed;
    popup_diag_v111[11]=popup_waits_v111;popup_diag_v111[12]=Kh02StatPopupOwnerStatusV111();
    MemoryBarrier();InterlockedIncrement64((LONG64*)&popup_diag_v111[1]);
}
static void popup_consume_v111(void){Kh02StatPopupConsumeV111(&popup_queue_v111);}
void Kh02StatPopupServiceV111(void) {
    uint32_t kind;
    if(GetCurrentThreadId()!=harness_thread || popup_servicing_v111 || !Kh02ControlledStartAuthorized())return;
    kind=Kh02StatPopupPeekV111(&popup_queue_v111,Kh02LifecycleEpoch(harness_life),popup_cookie_v111());
    if(kind) {
        popup_servicing_v111=1;
        if(!Kh02StatPopupNativeV111(kind,popup_consume_v111))popup_waits_v111++;
        popup_servicing_v111=0;
    }
    popup_publish_v111();
}
