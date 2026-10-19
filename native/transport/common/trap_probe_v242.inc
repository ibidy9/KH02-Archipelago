/* Development-only, explicit command: no hooks, automatic queue or save writes. */
#include "trap_guards_v242.h"
typedef struct TrapRequest242 {uint64_t magic,id,epoch;uint32_t kind,reserved;} TrapRequest242;
typedef struct TrapReply242 {uint64_t magic,id,epoch,kind,status,reason,calls,hp,mp,charge,
    charge_time,badstat,duration,state,paused,context_flags,actor,last_id;} TrapReply242;
extern uint32_t Kh02TrapReceipt244(uint64_t,uint32_t);
static uint64_t trap_last242,trap_calls242,trap_next244;
#ifdef KH02_GRANT_OWNER_SELFTEST
static void *trap_mp_test242,*trap_stun_test242;
static int trap_pause_test242=-1,trap_immune_test242=-1,trap_receipt_test244=-1;
static uint64_t trap_clock_test244;
KH02_RECEIPT_API void Kh02TrapReceiptTest244(int allowed,uint64_t now) {trap_receipt_test244=allowed;trap_clock_test244=now;}
KH02_RECEIPT_API void Kh02TrapTest242(void *mp,void *stun,int paused,int immune) {
    trap_mp_test242=mp;trap_stun_test242=stun;trap_pause_test242=paused;trap_immune_test242=immune;
}
#endif
static int trap_read242(const Kh02SessionSample *s,TrapReply242 *v) {
    uint32_t hp,mp,bad,duration,charge_time,flags;unsigned char charge,state;
    uint64_t manager,object,vt;
    if(!read_local(NULL,s->provider,&vt,8) ||
       (vt!=game.base+0x3735bc8 && vt!=game.base+0x373d490) ||
       !read_local(NULL,s->provider+0x5dc,&hp,4) || !read_local(NULL,s->provider+0x5e4,&mp,4) ||
       !read_local(NULL,s->provider+0x790,&charge,1) || !read_local(NULL,s->provider+0x794,&charge_time,4) ||
       !read_local(NULL,s->provider+0x7cc,&bad,4) || !read_local(NULL,s->provider+0x7d0,&duration,4) ||
       !read_local(NULL,s->provider+0x580,&manager,8) || !popup_pointer_v111(manager) ||
       !read_local(NULL,manager+0x270,&object,8) || !popup_pointer_v111(object) ||
       !read_local(NULL,object+0x58,&state,1) || !read_local(NULL,s->context+0x348,&flags,4))return 0;
    v->actor=s->provider;v->hp=hp;v->mp=mp;v->charge=!!(charge&2);v->charge_time=charge_time;
    v->badstat=bad;v->duration=duration;v->state=state;v->context_flags=flags;
    return hp<=10000 && mp<=10000;
}
uint32_t Kh02TrapRequest242(void *raw,uint32_t size,uint32_t capacity,uint32_t *written) {
    TrapRequest242 r;TrapReply242 v={0};Kh02SessionSample s;unsigned char b[24];
    uint64_t vt,fn,now;int locked=0,immune=0,admitted;
    if(written)*written=0;
    if(!raw || !written || size!=sizeof(r) || capacity<sizeof(v) || GetCurrentThreadId()!=owner_thread)return 2;
    memcpy(&r,raw,sizeof(r));v.magic=0x32343252545041ULL;v.id=r.id;v.epoch=r.epoch;v.kind=r.kind;v.status=2;
    v.reason=1;
    if(r.magic!=v.magic || r.reserved || r.kind>2 || (r.kind && (!r.id || r.id>=0x8000000000000000ULL)) ||
       (!r.kind && r.id) || !popup_sample_v111(&s,r.epoch))goto done;
    v.reason=2;
    if(InterlockedCompareExchange(&busy,1,0))goto done;
    locked=1;
    for(uint32_t i=0;i<sizeof(trap_guards242)/sizeof(trap_guards242[0]);i++)
        if(!read_local(NULL,game.base+trap_guards242[i].rva,b,24)||memcmp(b,trap_guards242[i].bytes,24))goto done;
    v.reason=3;
    if(!trap_read242(&s,&v))goto done;
#ifdef KH02_GRANT_OWNER_SELFTEST
    if(trap_pause_test242>=0)v.paused=(uint64_t)trap_pause_test242;else
#endif
    v.paused=((unsigned char (*)(void*))(game.base+0x1eb3590))((void*)(uintptr_t)s.world);
    if(!r.kind){v.status=1;v.reason=0;goto done;}
    v.reason=4;
    if(r.id<=trap_last242 || r.id>4096)goto done;
    admitted=Kh02TrapReceipt244(r.id,r.kind);
#ifdef KH02_GRANT_OWNER_SELFTEST
    if(trap_receipt_test244>=0)admitted=trap_receipt_test244;
#endif
    if(!admitted){v.reason=12;goto done;}
    now=GetTickCount64();
#ifdef KH02_GRANT_OWNER_SELFTEST
    if(trap_clock_test244)now=trap_clock_test244;
#endif
    if(now<trap_next244){v.reason=11;goto done;}
    /* Action state is diagnostic only: movement/attacks must not starve traps. */
    v.reason=5;
    if(!v.hp || v.paused || (v.context_flags&2) || v.badstat ||
       !popup_same_v111(&s,r.epoch))goto done;
    v.reason=6;
    if(!read_local(NULL,s.provider,&vt,8) ||
       !read_local(NULL,vt+0x14b8,&fn,8) || fn!=game.base+0x327d00 ||
       !read_local(NULL,vt+0xe40,&fn,8) || fn!=game.base+0x326bb0 ||
       !read_local(NULL,vt+0xe38,&fn,8) || fn!=game.base+0x31f8c0)goto done;
    if(r.kind==1 && (!v.mp || v.charge)){v.reason=7;goto done;}
    if(r.kind==2) {
#ifdef KH02_GRANT_OWNER_SELFTEST
        if(trap_immune_test242>=0)immune=trap_immune_test242;else
#endif
        immune=((unsigned char (*)(void*,uint32_t))(game.base+0x31f8c0))((void*)(uintptr_t)s.provider,9);
        if(immune){v.reason=8;goto done;}
    }
    if(!popup_same_v111(&s,r.epoch))goto done;
    /* Consume before native entry. Never repeat an uncertain application. */
    trap_last242=r.id;trap_calls242++;trap_next244=now+15000;
    if(r.kind==1) {
        void *target=(void*)(uintptr_t)(game.base+0x327d00);
#ifdef KH02_GRANT_OWNER_SELFTEST
        if(trap_mp_test242)target=trap_mp_test242;
#endif
        ((void (*)(void*,int32_t))target)((void*)(uintptr_t)s.provider,0);
    } else {
        void *target=(void*)(uintptr_t)(game.base+0x326bb0);
#ifdef KH02_GRANT_OWNER_SELFTEST
        if(trap_stun_test242)target=trap_stun_test242;
#endif
        /* Status9 branch does not dereference the optional damage/source args. */
        ((void (*)(void*,uint32_t,float,void*,void*))target)((void*)(uintptr_t)s.provider,9,10.0f,NULL,NULL);
    }
    v.reason=9;
    if(popup_same_v111(&s,r.epoch) && trap_read242(&s,&v)){v.status=1;v.reason=0;}
done:
    v.calls=trap_calls242;v.last_id=trap_last242;
    if(locked)InterlockedExchange(&busy,0);
    memcpy(raw,&v,sizeof(v));*written=sizeof(v);return (uint32_t)v.status;
}
