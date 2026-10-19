/* Native UI only. No stat/inventory/save mutation and no new hook. */
#include "kh02_stat_popup_guards_v111.h"
extern void Kh02StatPopupServiceV111(void);
static uint32_t popup_owner_status_v111;
uint32_t Kh02StatPopupOwnerStatusV111(void){return popup_owner_status_v111;}
static int popup_sample_v111(Kh02SessionSample *s,uint64_t epoch) {
    return GetCurrentThreadId()==owner_thread && is_active(NULL) &&
        Kh02ControlledStartAuthorized() && epoch && epoch==Kh02LifecycleEpoch(life) &&
        !session.stopped && !session.uncertain && !session.gate.busy &&
        Kh02OwnerReadSample(read_local,NULL,game.base,game.cache,session.gate.current.generation,s)==KH02_GATE_OK &&
        s->phase==3 && !s->title_flags && !s->pending_map && !(s->travel_flags&1) && !s->world_flag &&
        s->context==session.context && s->world==session.world && s->provider==session.gate.current.provider;
}
static int popup_same_v111(const Kh02SessionSample *s,uint64_t epoch) {
    Kh02SessionSample again;
    return popup_sample_v111(&again,epoch) && !memcmp(s,&again,sizeof(again));
}
static int popup_pointer_v111(uint64_t p) {return p>=0x10000 && p<0x7fffffff0000ULL;}
static int popup_vtable_v111(uint64_t obj,uint64_t *vt) {
    return popup_pointer_v111(obj) && read_local(NULL,obj,vt,8) &&
        *vt>=game.base+0x3599000 && *vt<game.base+0x4483b24;
}
uint32_t Kh02StatPopupNativeV111(uint32_t kind,void (*consume)(void)) {
    typedef void *(*Getter)(void*);
    typedef void (*Notify)(void*,uint32_t,const void*);
    Kh02SessionSample s;uint64_t epoch,vt,fn,controller,ui,manager,array,child;
    int32_t counts[2];unsigned char bytes[24];uint32_t present=0;
    struct {uint32_t kind,amount;} pair={kind,kind==4?5u:10u};
    struct {void *data;int32_t count,capacity;} list={&pair,1,1};
    popup_owner_status_v111=1;
    if(!consume || (kind!=4 && kind!=5) || GetCurrentThreadId()!=owner_thread)return 0;
    epoch=Kh02LifecycleEpoch(life);
    if(!popup_sample_v111(&s,epoch) || !popup_ready273(read_local,NULL,game.base,s.context) ||
       InterlockedCompareExchange(&busy,1,0))return 0;
    popup_owner_status_v111=2;
    for(uint32_t i=0;i<sizeof(popup_guards_v111)/sizeof(popup_guards_v111[0]);i++) {
        if(!read_local(NULL,game.base+popup_guards_v111[i].rva,bytes,24) ||
           memcmp(bytes,popup_guards_v111[i].bytes,24))goto done;
    }
    popup_owner_status_v111=3;
    if(!popup_vtable_v111(s.provider,&vt) || !read_local(NULL,vt+0x928,&fn,8) || fn!=game.base+0x31e730 ||
       !popup_same_v111(&s,epoch))goto done;
    controller=(uintptr_t)((Getter)(uintptr_t)(game.base+0x31e730))((void*)(uintptr_t)s.provider);
    popup_owner_status_v111=4;
    if(!popup_pointer_v111(controller) || !popup_same_v111(&s,epoch))goto done;
    ui=(uintptr_t)((Getter)(uintptr_t)(game.base+0x4f7e50))((void*)(uintptr_t)controller);
    popup_owner_status_v111=5;
    if(!popup_pointer_v111(ui) || !popup_same_v111(&s,epoch) ||
       !read_local(NULL,ui+0x90,&manager,8) || !popup_vtable_v111(manager,&vt) ||
       !read_local(NULL,manager+0x1a0,&array,8) || !popup_pointer_v111(array) ||
       !read_local(NULL,manager+0x1a8,counts,8) || counts[0]<=0 || counts[0]>16 ||
       counts[1]<counts[0] || counts[1]>16)goto done;
    popup_owner_status_v111=6;
    for(int32_t i=0;i<counts[0];i++) {
        if(!read_local(NULL,array+(uint64_t)i*8,&child,8))goto done;
        if(!child)continue;
        if(!popup_vtable_v111(child,&vt))goto done;
        for(uint32_t off=0x268;off<=0x278;off+=0x10) {
            if(!read_local(NULL,vt+off,&fn,8) || fn<game.base+0x1000 || fn>=game.base+0x359883f)goto done;
        }
        present++;
    }
    if(!present || !popup_same_v111(&s,epoch))goto done;
    /* Native copies the pair array synchronously. Its void return is not an ack.
       Consume BEFORE entering UI code; no retry after an uncertain call. */
    consume();popup_owner_status_v111=7;
    ((Notify)(uintptr_t)(game.base+0x56f530))((void*)(uintptr_t)ui,6,&list);
    popup_owner_status_v111=8;
done:
    InterlockedExchange(&busy,0);return popup_owner_status_v111==8;
}
