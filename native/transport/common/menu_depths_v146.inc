#include "menu_depths_v146.h"
uint32_t Kh02MenuDepthsOwnedV146(void);
typedef struct MenuAuthority146 {
    uint64_t owner,epoch,cookie;
    Kh02ReceiptOwner *receipt;
} MenuAuthority146;
static int menu_permit146(void *raw) {
    MenuAuthority146 *a=raw;uint64_t root,instance,current;
    Kh02RestoreSnapshot snapshot;
    return GetCurrentThreadId()==owner_thread && active_flag && !fault && !busy && !flight &&
        a->cookie && a->epoch && seen_epoch==a->epoch &&
        session.context==a->owner && session.gate.current.cookie==a->cookie &&
        Kh02LifecycleEpoch(life)==a->epoch && Kh02LifecycleCurrent(life)==a->receipt &&
        a->receipt && Kh02ReceiptOwnerCheck(a->receipt)==KH02_MEMBER_OK &&
        Kh02ControlledStartAuthorized() && Kh02MenuDepthsOwnedV146() &&
        read_local(NULL,game.base+0x480b5d0,&root,8) && root &&
        read_local(NULL,root+0x230,&instance,8) && instance &&
        read_local(NULL,instance+0x50,&current,8) && current==a->owner &&
        Kh02RestoreSnapshotRead(read_local,NULL,game.base,a->owner,&snapshot)==KH02_RESTORE_OK;
}
static int menu_write146(void *raw,uint64_t address,const void *value,uint32_t size) {
    SIZE_T n=0;(void)raw;
    return WriteProcessMemory(GetCurrentProcess(),(void*)(uintptr_t)address,value,size,&n) && n==size;
}
uint32_t kh02_menu_depths_v146(uint64_t *frame,uintptr_t caller) {
    MenuAuthority146 a;uint64_t map,spawn;int result;
    /* Exact Teleportation confirmation only. The native fade companion and
       story/goal routes never prepare a restart or alter these fields. */
    if(!frame || caller!=game.base+0x5ca61b || frame[3]!=1)return 0;
    if(GetCurrentThreadId()!=owner_thread)return 1;
    map=start_name(L"/Game/Levels/dw/dw_09/dw_09",1);
    spawn=start_name(L"dw_09_Lv_Start_01",1);
    if(!map || !spawn)return 1;
    if(frame[1]!=map || frame[2]!=spawn)return kh02_menu_forest_v149(frame,caller);
    a=(MenuAuthority146){frame[0],Kh02LifecycleEpoch(life),session.gate.current.cookie,Kh02LifecycleCurrent(life)};
    result=menu_prepare146(read_local,menu_write146,menu_permit146,&a,a.owner);
    Kh02OpeningTraceStart(game.base,caller,a.owner,146,(uint32_t)result);
    if(result==3){InterlockedExchange(&fault,1);InterlockedExchange(&active_flag,0);}
    return result!=1;
}
