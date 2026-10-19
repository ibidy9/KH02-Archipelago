#include "kh02_magic_observation_v38.h"
uint32_t Kh02GrantOwnerMagicObservation(Kh02MagicObservation *out) {
    static const uint32_t commands[7]={0x33,0x34,0x37,0x38,0x3b,0x3c,0x47};
    static const uint32_t flags[7]={0,0,0,0,5,5,1};
    static const uint32_t effects[7]={1301,1304,1320,1325,1342,1343,1422};
    static const uint32_t voices[7]={1,1,2,2,4,4,3};
    uint64_t again=0,vt=0;
    if(!out)return 2;
    memset(out,0,sizeof(*out));out->status=2;
    if(!is_active(NULL) || !session.context || !session.world || !session.gate.current.observation)return 2;
    out->epoch=seen_epoch;out->provider=session.gate.current.provider;
    out->status=3;
    if(!diagnostic_pointer(out->provider) || !read_local(NULL,out->provider,&vt,8) ||
       (vt!=game.base+0x3735bc8 && vt!=game.base+0x373d490) ||
       !read_local(NULL,out->provider+0x578,&out->equipment,8) || !diagnostic_pointer(out->equipment))return 3;
    out->status=4;
    if(!read_local(NULL,out->equipment,&out->equipment_vtable,8) || out->equipment_vtable!=game.base+0x37a9810 ||
       !read_local(NULL,out->equipment+0x270,&out->magic,8) || !diagnostic_pointer(out->magic) ||
       !read_local(NULL,out->equipment+0x280,&out->replacement,8))return 4;
    out->status=5;
    if(!read_local(NULL,out->magic,&out->magic_vtable,8) || out->magic_vtable!=game.base+0x3750bc0 ||
       !read_local(NULL,out->magic+0x48,&out->records,8) || !diagnostic_pointer(out->records) ||
       !read_local(NULL,out->magic+0x50,&out->count,4) || !read_local(NULL,out->magic+0x54,&out->capacity,4) ||
       out->count!=7 || out->capacity<7 || out->capacity>256)return 5;
    out->status=6;
    if(!read_local(NULL,out->records,out->rows,sizeof(out->rows)))return 6;
    for(uint32_t i=0;i<7;i++) {
        Kh02MagicRow *r=&out->rows[i];
        if((r->command!=commands[i] && r->command!=0) || r->flags!=flags[i] ||
           r->effect!=effects[i] || r->voice!=voices[i] ||
           !diagnostic_pointer(r->projectile) || r->trailing)return 6;
    }
    out->status=7;
    if(!read_local(NULL,out->provider+0x578,&again,8) || again!=out->equipment ||
       !read_local(NULL,out->equipment+0x270,&again,8) || again!=out->magic ||
       !read_local(NULL,out->magic+0x48,&again,8) || again!=out->records)return 7;
    out->status=1;return 1;
}
