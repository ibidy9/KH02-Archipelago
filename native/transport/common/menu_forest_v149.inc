#include "menu_forest_v149.h"
#include "menu_return_v215.h"
#include "menu_ww_return_v231.h"
#include "menu_local_return_v235.h"
uint32_t Kh02MenuForestOwnedV149(void);
typedef struct MenuAuthority149 {
    uint64_t owner,epoch,cookie;
    Kh02ReceiptOwner *receipt;
} MenuAuthority149;
static int menu_permit149(void *raw) {
    MenuAuthority149 *a=raw;uint64_t root,instance,current;
    Kh02RestoreSnapshot snapshot;
    return GetCurrentThreadId()==owner_thread && active_flag && !fault && !busy && !flight &&
        a->cookie && a->epoch && seen_epoch==a->epoch &&
        session.context==a->owner && session.gate.current.cookie==a->cookie &&
        Kh02LifecycleEpoch(life)==a->epoch && Kh02LifecycleCurrent(life)==a->receipt &&
        a->receipt && Kh02ReceiptOwnerCheck(a->receipt)==KH02_MEMBER_OK &&
        Kh02ControlledStartAuthorized() && Kh02MenuForestOwnedV149() &&
        read_local(NULL,game.base+0x480b5d0,&root,8) && root &&
        read_local(NULL,root+0x230,&instance,8) && instance &&
        read_local(NULL,instance+0x50,&current,8) && current==a->owner &&
        Kh02RestoreSnapshotRead(read_local,NULL,game.base,a->owner,&snapshot)==KH02_RESTORE_OK;
}
static int menu_write149(void *raw,uint64_t address,const void *value,uint32_t size) {
    SIZE_T n=0;(void)raw;
    return WriteProcessMemory(GetCurrentProcess(),(void*)(uintptr_t)address,value,size,&n) && n==size;
}
static int menu_focused214(uint64_t *frame,uintptr_t caller) {
    unsigned world,mode;uint64_t state=frame[0]+0x3330;
    static const wchar_t *local_maps[]={L"/Game/Levels/dw/dw_02/dw_02",L"/Game/Levels/dw/dw_09/dw_09"};
    static const wchar_t *local_spawns[]={L"dw_02_Lv_Save_01",L"dw_09_Lv_Save_01"};
    for(unsigned j=0;j<2;j++)if(frame[1]==start_name(local_maps[j],1) && frame[2]==start_name(local_spawns[j],1)) {
        uint32_t bit;if(!menu_save_bit214(local_spawns[j],&bit))return 1;
        unsigned w=j?3:0;
        MenuAuthority150 a={frame[0],Kh02LifecycleEpoch(life),session.gate.current.cookie,Kh02LifecycleCurrent(life),w,0};
        int result=menu_local_return235(read_local,menu_write150,menu_permit150,&a,frame[0],bit,w);
        Kh02OpeningTraceStart(game.base,caller,frame[0],235,(w<<16)|(unsigned)result);
        if(result==3){InterlockedExchange(&fault,1);InterlockedExchange(&active_flag,0);}
        return result!=1;
    }
    if(frame[1]==start_name(L"/Game/Levels/dw/dw_03/dw_03",1) &&
       frame[2]==start_name(L"dw_03_Lv_Save_01",1)) {
        uint32_t bit;
        if(!menu_save_bit214(L"dw_03_Lv_Save_01",&bit))return 1;
        MenuAuthority150 authority={frame[0],Kh02LifecycleEpoch(life),session.gate.current.cookie,
            Kh02LifecycleCurrent(life),1,0};
        int result=menu_ww_return231(read_local,menu_write150,menu_permit150,&authority,frame[0],bit);
        Kh02OpeningTraceStart(game.base,caller,frame[0],231,(unsigned)result);
        if(result==3){InterlockedExchange(&fault,1);InterlockedExchange(&active_flag,0);}
        return result!=1;
    }
    if(frame[1]==start_name(L"/Game/Levels/dw/dw_08/dw_08",1)) {
        static const wchar_t *names[]={L"dw_08_Lv_Save_01",L"dw_08_Lv_Save_02",L"dw_08_Lv_Save_03"};
        for(unsigned selected=0;selected<3;selected++) {
            if(frame[2]!=start_name(names[selected],1))continue;
            uint32_t bits[3];
            for(unsigned i=0;i<3;i++)if(!menu_save_bit214(names[i],&bits[i]))return 1;
            MenuAuthority150 authority={frame[0],Kh02LifecycleEpoch(life),session.gate.current.cookie,
                Kh02LifecycleCurrent(life),2,0};
            int result=menu_return215(read_local,menu_write150,menu_permit150,&authority,frame[0],selected,bits);
            Kh02OpeningTraceStart(game.base,caller,frame[0],215,(selected<<16)|(unsigned)result);
            if(result==3){InterlockedExchange(&fault,1);InterlockedExchange(&active_flag,0);}
            return result!=1;
        }
    }
    if(frame[1]==start_name(L"/Game/Levels/dw/dw_08/dw_08",1) &&
       frame[2]==start_name(L"dw_08_Lv_Autosave_06",1)) {
        world=2;mode=1;
        if(!menu_forest_ready214(state,1,0))return 1;
    } else if(frame[1]==start_name(L"/Game/Levels/dw/dw_07/dw_07",1) &&
              frame[2]==start_name(L"dw_07_Lv_Start_01",1)) {
        world=1;mode=2;
        if(!menu_phantom_ready214(state))return 1;
    } else if(frame[1]==start_name(L"/Game/Levels/dw/dw_02/dw_02",1) &&
              frame[2]==start_name(L"dw_02_Lv_Retry_01",1)) {
        world=0;mode=3;
        if(!menu_boss_ready239(state,4))return 1;
    } else if(frame[1]==start_name(L"/Game/Levels/dw/dw_08/dw_08",1) &&
              frame[2]==start_name(L"dw_08_Lv_Retry_02",1)) {
        world=2;mode=4;
        if(!menu_boss_ready239(state,32))return 1;
    } else return kh02_menu_world_v150(frame,caller);
    MenuAuthority150 a={frame[0],Kh02LifecycleEpoch(life),session.gate.current.cookie,
        Kh02LifecycleCurrent(life),world,0};
    int result=menu_prepare214(read_local,menu_write150,menu_permit150,&a,a.owner,mode,0);
    Kh02OpeningTraceStart(game.base,caller,a.owner,214,(mode<<16)|(unsigned)result);
    if(result==3){InterlockedExchange(&fault,1);InterlockedExchange(&active_flag,0);}
    return result!=1;
}
uint32_t kh02_menu_forest_v149(uint64_t *frame,uintptr_t caller) {
    MenuAuthority149 a;uint64_t map,spawn;int result;
    /* Exact Teleportation confirmation only. The native fade companion and
       story/goal routes never prepare a restart or alter these fields. */
    if(!frame || caller!=game.base+0x5ca61b || frame[3]!=1)return 0;
    if(GetCurrentThreadId()!=owner_thread)return 1;
    map=start_name(L"/Game/Levels/dw/dw_08/dw_08",1);
    spawn=start_name(L"dw_08_Lv_Start_01",1);
    if(!map || !spawn)return 1;
    if(frame[1]!=map || frame[2]!=spawn)return menu_focused214(frame,caller);
    a=(MenuAuthority149){frame[0],Kh02LifecycleEpoch(life),session.gate.current.cookie,Kh02LifecycleCurrent(life)};
    uint32_t first,rocky,end;
    if(!menu_save_bit214(L"dw_08_Lv_Save_01",&first) ||
       !menu_save_bit214(L"dw_08_Lv_Save_02",&rocky) ||
       !menu_save_bit214(L"dw_08_Lv_Save_03",&end) ||
       first==rocky || first==end || rocky==end)return 1;
    result=menu_prepare214(read_local,menu_write149,menu_permit149,&a,a.owner,0,rocky|end);
    Kh02OpeningTraceStart(game.base,caller,a.owner,149,(uint32_t)result);
    if(result==3){InterlockedExchange(&fault,1);InterlockedExchange(&active_flag,0);}
    return result!=1;
}
