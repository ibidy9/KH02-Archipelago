/* Candidate native defeat operation. No new hook, save write or receipt mutation. */
#include "deathlink_guards_v249.h"
typedef struct DeathReply249 {uint64_t magic,id,epoch,kind,status,reason,calls,hp,mp,state,
    paused,context_flags,actor,last_id,native_result,reserved;} DeathReply249;
static uint64_t death_last249,death_calls249;
#ifdef KH02_GRANT_OWNER_SELFTEST
static void *death_hp_test249,*death_die_test249;
KH02_RECEIPT_API void Kh02DeathTest249(void *hp,void *die) {death_hp_test249=hp;death_die_test249=die;}
#endif
uint32_t Kh02DeathRequest249(void *raw,uint32_t size,uint32_t capacity,uint32_t *written) {
    TrapRequest242 r;DeathReply249 v={0};TrapReply242 t={0};Kh02SessionSample s;
    uint64_t vt,subvt,fn;unsigned char b[24];int locked=0;
    if(written)*written=0;
    if(!raw || !written || size!=sizeof(r) || capacity<sizeof(v) || GetCurrentThreadId()!=owner_thread)return 2;
    memcpy(&r,raw,sizeof(r));v.magic=0x393432444c5041ULL;v.id=r.id;v.epoch=r.epoch;v.kind=r.kind;v.status=2;v.reason=1;
    if(r.magic!=v.magic || r.reserved || r.kind>1 ||
       (r.kind && (!r.id || r.id>=0x8000000000000000ULL)) || (!r.kind && r.id) ||
       !popup_sample_v111(&s,r.epoch))goto done;
    v.reason=2;if(InterlockedCompareExchange(&busy,1,0))goto done;locked=1;
    for(uint32_t i=0;i<sizeof(death_guards249)/sizeof(death_guards249[0]);i++)
        if(!read_local(NULL,(death_guards249[i].rva==0x519200 && entries[24].owned ? (uint64_t)(uintptr_t)death_trampoline250 : game.base+death_guards249[i].rva),b,(death_guards249[i].rva==0x519200 && entries[24].owned ? 18:24))||memcmp(b,death_guards249[i].bytes,(death_guards249[i].rva==0x519200 && entries[24].owned ? 18:24)))goto done;
    v.reason=3;if(!trap_read242(&s,&t))goto done;
    v.hp=t.hp;v.mp=t.mp;v.state=t.state;v.context_flags=t.context_flags;v.actor=t.actor;
#ifdef KH02_GRANT_OWNER_SELFTEST
    if(trap_pause_test242>=0)v.paused=(uint64_t)trap_pause_test242;else
#endif
    v.paused=((unsigned char (*)(void*))(game.base+0x1eb3590))((void*)(uintptr_t)s.world);
    if(!r.kind){v.status=1;v.reason=0;goto done;}
    v.reason=4;if(r.id<=death_last249)goto done;
    v.reason=5;
    if(!v.hp || v.paused || (v.context_flags&2) || v.state==0x33 || v.state==0x5e || t.badstat ||
       !popup_same_v111(&s,r.epoch))goto done;
    v.reason=6;
    if(!read_local(NULL,s.provider,&vt,8) ||
       !read_local(NULL,vt+0x14a0,&fn,8) || fn!=game.base+0x327c80 ||
       !read_local(NULL,s.provider+0x450,&subvt,8) ||
       (subvt!=game.base+0x3737f60 && subvt!=game.base+0x373f828) ||
       !read_local(NULL,subvt+0x368,&fn,8) || fn!=game.base+0x510f20 ||
       !read_local(NULL,subvt+0x370,&fn,8) || fn!=game.base+0x519200)goto done;
    if(!popup_same_v111(&s,r.epoch))goto done;
    /* Consume BEFORE either call: an uncertain result must never kill twice. */
    death_last249=r.id;death_calls249++;
    {
        void *hp=(void*)(uintptr_t)(game.base+0x327c80),*die=(void*)(uintptr_t)(game.base+0x519200);
        unsigned char damage[40]={0};float direction[3]={0,0,0};uint16_t native_tail=2;
        /* Same default damage record tail as native TakeDamage; no attacker. */
        memcpy(damage+0x24,&native_tail,2);
#ifdef KH02_GRANT_OWNER_SELFTEST
        if(death_hp_test249)hp=death_hp_test249;if(death_die_test249)die=death_die_test249;
#endif
        ((void (*)(void*,int32_t))hp)((void*)(uintptr_t)s.provider,0);
        death_remote250++;
        v.native_result=((unsigned char (*)(void*,const void*,float,const void*,const void*,void*))die)(
            (void*)(uintptr_t)(s.provider+0x450),damage,0.0f,direction,direction,NULL);
        death_remote250--;
    }
    v.reason=7;
    if(popup_same_v111(&s,r.epoch) && trap_read242(&s,&t)){
        v.hp=t.hp;v.mp=t.mp;v.state=t.state;v.status=1;v.reason=0;
    }
done:
    v.calls=death_calls249;v.last_id=death_last249;
    if(locked)InterlockedExchange(&busy,0);
    memcpy(raw,&v,sizeof(v));*written=sizeof(v);return (uint32_t)v.status;
}
