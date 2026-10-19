#include "menu_order_plan_v213.h"
void *kh02_menu_order_trampoline213,*kh02_menu_seen_resume213;
static void *menu_seen_unused213;
extern void kh02_menu_order_thunk213(void),kh02_menu_seen_thunk213(void);
static const unsigned char menu_order_prefix213[18]={0x33,0xd2,0x4c,0x89,0x65,0xa7,0x48,0x8d,0x4d,0xa7,0x48,0xc7,0x45,0xaf,8,0,0,0};
/* Filled from exact-build file audit, including the relative call bytes. */
static const unsigned char menu_seen_prefix213[14]={0x8b,0xd7,0x48,0x8d,0x88,0x18,0x3d,0,0,0xe8,0x96,0xa3,0xe5,0xff};
typedef struct MenuOrder213 {
    uint64_t epoch,cookie,state,widget,array;
    uint32_t next,count,pending,discovered,valid,applied;
    uint32_t ids[18],sorted[18];
    uint64_t maps[18],spawns[18];
} MenuOrder213;
static MenuOrder213 menu_order213;
static uint32_t menu_trace_count213;
static int menu_writable213(uint64_t address,size_t size) {
    MEMORY_BASIC_INFORMATION info;
    return address>=0x10000 && address<0x7fffffffffffULL-size &&
        VirtualQuery((void*)(uintptr_t)address,&info,sizeof(info))==sizeof(info) &&
        info.State==MEM_COMMIT && !(info.Protect&(PAGE_GUARD|PAGE_NOACCESS)) &&
        (info.Protect&(PAGE_READWRITE|PAGE_WRITECOPY)) &&
        address+size<=(uintptr_t)info.BaseAddress+info.RegionSize;
}
static int menu_authority213(void) {
    Kh02ReceiptOwner *r=Kh02LifecycleCurrent(life);
    return menu_scope151() && session.context && session.gate.current.cookie &&
        seen_epoch==Kh02LifecycleEpoch(life) && r &&
        Kh02ReceiptOwnerCheck(r)==KH02_MEMBER_OK && menu_root150(session.context);
}
static void menu_begin213(uint64_t row,uint64_t state,uint32_t index,uint32_t shown) {
    MenuOrder213 *m=&menu_order213;unsigned world,kind;
    static const unsigned worlds[18]={0,1,2,2,2,3,0,1,2,3,1,2,0,1,2,3,0,2};
    static const unsigned kinds[18]={3,3,3,0,0,3,1,1,1,1,0,0,2,2,2,2,0,0};
    if(!index) {
        memset(m,0,sizeof(*m));m->valid=1;m->state=state;
        m->epoch=Kh02LifecycleEpoch(life);m->cookie=session.gate.current.cookie;
    }
    if(!m->valid || index!=m->next || index>=18 || state!=m->state ||
       !menu_authority213() || m->epoch!=Kh02LifecycleEpoch(life) ||
       m->cookie!=session.gate.current.cookie || !menu_classify151(row,&world,&kind) ||
       world!=worlds[index] || kind!=kinds[index]) {m->valid=0;return;}
    /* Forest rows share a world/kind; verify their distinct route identities. */
    uint64_t spawn;
    if(!read_local(NULL,row+8,&spawn,8) ||
       (index==3 && spawn!=start_name(L"dw_08_Lv_Save_02",1)) ||
       (index==4 && spawn!=start_name(L"dw_08_Lv_Save_03",1)) ||
       (index==11 && spawn!=start_name(L"dw_08_Lv_Autosave_06",1)) ||
       (index==16 && spawn!=start_name(L"dw_02_Lv_Retry_01",1)) ||
       (index==17 && spawn!=start_name(L"dw_08_Lv_Retry_02",1))) {m->valid=0;return;}
    m->pending=index;m->discovered=shown;m->next++;
}
static void menu_end213(uint64_t row,uint32_t shown) {
    MenuOrder213 *m=&menu_order213;
    if(!m->valid || !m->next || m->count>=18)return;
    if(m->discovered || shown) {
        uint32_t i=m->count;
        if(!read_local(NULL,row+8,&m->spawns[i],8) || !read_local(NULL,row+16,&m->maps[i],8)) {m->valid=0;return;}
        m->ids[i]=m->pending;m->count++;
    }
}
/* Runs once after native visibility/row construction, before UI publication. */
void kh02_menu_order_capture213(uint64_t widget) {
    MenuOrder213 *m=&menu_order213;
    struct {uint64_t data;int32_t count,capacity;} a;
    unsigned char before[18*0x38],after[18*0x38];int32_t focus,new_focus=-1;
    uint32_t trace=menu_trace_count213++<6;
    if(trace)Kh02OpeningTraceStart(game.base,game.base+0x5d1b6a,session.context,232,
        (m->valid<<24)|(m->next<<16)|(m->count<<8)|m->applied);
    if(!menu_authority213() || !m->valid || m->next!=18 || !m->count || m->applied ||
       m->epoch!=Kh02LifecycleEpoch(life) || m->cookie!=session.gate.current.cookie ||
       !read_local(NULL,widget+0x2d8,&a,sizeof(a)) || a.count!=(int32_t)m->count ||
       a.capacity<a.count || a.capacity>64 || a.data<0x10000 || (a.data&7) ||
       !read_local(NULL,widget+0x310,&focus,4) ||
       !read_local(NULL,a.data,before,m->count*0x38) ||
       !menu_writable213(a.data,m->count*0x38) || !menu_writable213(widget+0x310,4))return;
    for(uint32_t i=0;i<m->count;i++) {
        uint64_t map,spawn;memcpy(&map,before+i*0x38+0x10,8);memcpy(&spawn,before+i*0x38+0x18,8);
        if(map!=m->maps[i] || spawn!=m->spawns[i])return;
    }
    if(!menu_plan213(before,m->ids,m->count,focus,after,m->sorted,&new_focus))return;
    /* Same owner thread, no native calls or allocator activity between reads
     * and commit. Metadata and row ownership remain unchanged. */
    memcpy((void*)(uintptr_t)a.data,after,m->count*0x38);
    memcpy((void*)(uintptr_t)(widget+0x310),&new_focus,4);
    m->widget=widget;m->array=a.data;m->applied=1;
    if(trace) {
        Kh02OpeningTraceStart(game.base,game.base+0x5d1b6a,session.context,230,(m->count<<16)|(uint32_t)new_focus);
        for(uint32_t i=0;i<m->count;i++)
            Kh02OpeningTraceStart(game.base,game.base+0x5d1b6a,session.context,239,(i<<16)|m->sorted[i]);
    }
}
/* Original callback's only persistent write: mark the hovered entry seen.
 * The UI and native travel callback keep using the sorted display index. */
void kh02_menu_seen_capture213(uint64_t widget,uint64_t field,uint32_t index) {
    MenuOrder213 *m=&menu_order213;uint64_t array;int32_t count;
    if(m->applied && widget==m->widget) {
        if(!menu_authority213() || m->epoch!=Kh02LifecycleEpoch(life) ||
           m->cookie!=session.gate.current.cookie || field!=m->state-0x2ff8+0x3d18 ||
           index>=m->count || !read_local(NULL,widget+0x2d8,&array,8) || array!=m->array ||
           !read_local(NULL,widget+0x2e0,&count,4) || count!=(int32_t)m->count)return;
        index=m->sorted[index];
    }
    ((void (__fastcall *)(uint64_t,uint32_t))(game.base+0x42e8c0))(field,index);
}
