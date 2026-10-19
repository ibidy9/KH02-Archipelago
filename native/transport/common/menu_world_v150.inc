#include "menu_world_v150.h"
uint32_t Kh02MenuWorldOwnedV150(uint32_t);
typedef struct MenuToken150 {uint64_t owner,epoch,cookie;uint32_t world,armed;} MenuToken150;
typedef struct MenuAuthority150 {
    uint64_t owner,epoch,cookie;Kh02ReceiptOwner *receipt;uint32_t world,command;
} MenuAuthority150;
static MenuToken150 menu_token150;
static int menu_root150(uint64_t owner) {
    uint64_t root,instance,current;
    return read_local(NULL,game.base+0x480b5d0,&root,8) && root &&
        read_local(NULL,root+0x230,&instance,8) && instance &&
        read_local(NULL,instance+0x50,&current,8) && current==owner;
}
static unsigned menu_permit_reason157;
static int menu_permit150(void *raw) {
    MenuAuthority150 *a=raw;Kh02RestoreSnapshot snapshot;menu_permit_reason157=0;
#define MP157(value,reason) do {if(!(value)){menu_permit_reason157=(reason);return 0;}}while(0)
    MP157(GetCurrentThreadId()==owner_thread,1);MP157(active_flag,2);MP157(!fault,3);MP157(!busy,4);
    MP157(flight==(LONG)a->command,5);MP157(a->cookie,6);MP157(a->epoch,7);MP157(seen_epoch==a->epoch,8);
    MP157(session.context==a->owner,9);MP157(session.gate.current.cookie==a->cookie,10);
    MP157(Kh02LifecycleEpoch(life)==a->epoch,11);MP157(Kh02LifecycleCurrent(life)==a->receipt,12);
    MP157(a->receipt,13);MP157(Kh02ReceiptOwnerCheck(a->receipt)==KH02_MEMBER_OK,14);
    MP157(Kh02ControlledStartAuthorized(),15);MP157(Kh02MenuWorldOwnedV150(a->world),16);
    MP157(menu_root150(a->owner),17);
    MP157(Kh02RestoreSnapshotRead(read_local,NULL,game.base,a->owner,&snapshot)==KH02_RESTORE_OK,18);
#undef MP157
    return 1;
}
static int menu_write150(void *raw,uint64_t address,const void *value,uint32_t size) {
    SIZE_T n=0;(void)raw;
    return WriteProcessMemory(GetCurrentProcess(),(void*)(uintptr_t)address,value,size,&n) && n==size;
}
static void menu_result150(uint64_t owner,uintptr_t caller,uint32_t result) {
    Kh02OpeningTraceStart(game.base,caller,owner,150,result);
    if(result==3){InterlockedExchange(&fault,1);InterlockedExchange(&active_flag,0);}
}
#include "menu_music_v219.inc"
static uint32_t kh02_menu_world_v150(uint64_t *frame,uintptr_t caller) {
    static const wchar_t *maps[]={L"/Game/Levels/dw/dw_01/dw_01",L"/Game/Levels/dw/dw_59/dw_59"};
    static const wchar_t *spawns[]={L"dw_01_Lv_Start_01",L"dw_59_Lv_Start_01"};
    if(!frame || caller!=game.base+0x5ca61b || frame[3]!=1)return 0;
    if(GetCurrentThreadId()!=owner_thread)return 1;
    menu_token150.armed=0;
    for(unsigned world=0;world<2;world++) {
        uint64_t map=start_name(maps[world],1),spawn=start_name(spawns[world],1);
        if(!map || !spawn)return 1;
        if(frame[1]!=map || frame[2]!=spawn)continue;
        MenuAuthority150 a={frame[0],Kh02LifecycleEpoch(life),session.gate.current.cookie,Kh02LifecycleCurrent(life),world,0};
        int result=menu_prepare150(read_local,menu_write150,menu_permit150,&a,a.owner,world,0);
        if(result!=1) {
            /* Existing fixed trace only: retain exact transaction/permit
             * refusal reasons before returning to the native confirmation. */
            Kh02OpeningTraceStart(game.base,caller,a.owner,201,menu_reason157);
            Kh02OpeningTraceStart(game.base,caller,a.owner,202,menu_permit_reason157);
        }
        if(result==1) {
            unsigned audio=menu_music219(game.base,world,result,menu_permit150(&a));
            Kh02OpeningTraceStart(game.base,caller,a.owner,219,audio);
        }
        if(result==1)menu_token150=(MenuToken150){a.owner,a.epoch,a.cookie,world,1};
        menu_result150(a.owner,caller,(uint32_t)result);return result!=1;
    }
    return 0;
}
/* Read the exact supported owner's native cinematic predicate, without calling
 * an engine function or installing another hook. Static audit v161 pins these. */
static int menu_castle_ready161(uint64_t owner) {
    static const unsigned char prefix[]={0x8b,0x81,0x48,0x03,0,0,0xd1,0xe8,0x24,1,0xc3};
    uint64_t vt,fn;uint32_t story,flags;char map[32];unsigned char bytes[sizeof(prefix)];
    return read_local(NULL,owner,&vt,8) && vt==game.base+0x36df228 &&
        read_local(NULL,vt+0x270,&fn,8) && fn==game.base+0x44f2f0 &&
        read_local(NULL,fn,bytes,sizeof(bytes)) && !memcmp(bytes,prefix,sizeof(bytes)) &&
        read_local(NULL,owner+0x3338,&story,4) && story==110 &&
        read_local(NULL,owner+0x384c,map,32) && !memcmp(map,"/Game/Levels/dw/dw_02/dw_02",26) &&
        read_local(NULL,owner+0x348,&flags,4) && !(flags&2);
}
static int menu_permit161(void *raw) {
    MenuAuthority150 *a=raw;
    return menu_permit150(raw) && menu_castle_ready161(a->owner);
}
static uint32_t menu_castle_stage161(int automatic) {
    char map[32];MenuToken150 t=menu_token150;
    if(!t.armed || t.world!=0 || t.epoch!=Kh02LifecycleEpoch(life) ||
       t.cookie!=session.gate.current.cookie || !read_local(NULL,t.owner+0x384c,map,32) ||
       memcmp(map,"/Game/Levels/dw/dw_02/dw_02",26))return 2;
    MenuAuthority150 a={t.owner,t.epoch,t.cookie,Kh02LifecycleCurrent(life),0,1};
    int result=menu_prepare150(read_local,menu_write150,automatic?menu_permit161:menu_permit150,&a,t.owner,CASTLE_GEARS150,1);
    Kh02OpeningTraceStart(game.base,game.base+0x5ca61b,t.owner,157,menu_reason157);
    Kh02OpeningTraceStart(game.base,game.base+0x5ca61b,t.owner,159,menu_permit_reason157);
    if(menu_before_valid157)for(unsigned i=0;i<2;i++) {
        uint64_t packed=0;memcpy(&packed,menu_before157.switches[i],6);
        packed|=(uint64_t)menu_before157.treasure1[i]<<48;
        Kh02OpeningTraceRemote(game.base,t.owner,0,packed,157+i);
    }
    if(result || automatic)menu_token150.armed=0;
    menu_result150(t.owner,game.base+0x5ca61b,(uint32_t)(result+10));
    if(result==3){InterlockedExchange(&fault,1);InterlockedExchange(&active_flag,0);}
    return result==1?1:result==3?5:2;
}
/* Retained development action9; production timing never calls the client. */
static uint32_t menu_castle_stage150(void) {return menu_castle_stage161(0);}
static void menu_castle_auto161(void) {
    MenuToken150 t=menu_token150;
    if(GetCurrentThreadId()!=owner_thread || !t.armed || t.world!=0)return;
    if(t.epoch!=Kh02LifecycleEpoch(life) || t.cookie!=session.gate.current.cookie) {
        menu_token150.armed=0;return;
    }
    MenuAuthority150 a={t.owner,t.epoch,t.cookie,Kh02LifecycleCurrent(life),0,1};
    if(!menu_permit161(&a))return;
    uint32_t result=menu_castle_stage161(1);
    Kh02OpeningTraceStart(game.base,game.base+0x5ca61b,t.owner,161,result);
}
static void menu_ww_event150(void *context,uint64_t event) {
    MenuToken150 t=menu_token150;uint32_t story;char map[32];unsigned char prefix[16];
    if(GetCurrentThreadId()!=owner_thread || !t.armed || t.world!=1)return;
    if(!active_flag || fault || busy || flight!=1 || t.epoch!=Kh02LifecycleEpoch(life) ||
       t.cookie!=session.gate.current.cookie || !Kh02ControlledStartAuthorized()) {
        menu_token150.armed=0;return;
    }
    uint64_t expected=start_name(L"dw_50_gameflow_Evt_cutscene_dw50_dw015_Movie",0);
    if(!expected || event!=expected)return;
    menu_token150.armed=0;
    if(!menu_root150(t.owner) || !Kh02MenuWorldOwnedV150(1) ||
       !read_local(NULL,t.owner+0x3338,&story,4) || story!=200 ||
       !read_local(NULL,t.owner+0x384c,map,32) ||
       memcmp(map,"/Game/Levels/dw/dw_59/dw_59",26) ||
       !read_local(NULL,game.base+0x459540,prefix,16) || memcmp(prefix,ww_close_prefix,16))return;
    Kh02OpeningTraceRemote(game.base,t.owner,(uintptr_t)context,event,150);
    ((void (__fastcall *)(void))(game.base+0x459540))();
}

#ifdef KH02_GRANT_OWNER_SELFTEST
KH02_RECEIPT_API void Kh02MenuWorldEventTestV150(uint64_t event) {
    InterlockedIncrement(&flight);menu_ww_event150(NULL,event);InterlockedDecrement(&flight);
}
KH02_RECEIPT_API uint32_t Kh02MenuCastleStageTestV150(void) {
    uint32_t r;InterlockedIncrement(&flight);r=menu_castle_stage150();InterlockedDecrement(&flight);return r;
}
#endif
