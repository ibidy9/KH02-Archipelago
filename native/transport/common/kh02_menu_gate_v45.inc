/* AP-derived UI locking; no game data, costs or queue record writes. */
#include "kh02_menu_gate_v45.h"
static Kh02MenuObservation menu_observation;
uint64_t kh02_menu_observe(void *hud,uint32_t command,void *row,void *ret,uint64_t result) {
    static const uint32_t commands[7]={0x33,0x34,0x37,0x38,0x3b,0x3c,0x47};
    static const uint32_t callers[7]={0x57aaff,0x589821,0x589a9c,0x59b6e0,0x59bd9a,0x59e55c,0x59eb4c};
    uint64_t controller=0,actor=0,vt=0,hvt=0,again=0,bytes=0;
    uint32_t tail=0,reason=0,ci=7,si=7;
    InterlockedIncrement(&flight);magic_inc(&menu_observation.raw);
    if(GetCurrentThreadId()!=owner_thread){reason=1;goto done;}
    if(!is_active(NULL)){reason=2;goto done;}
    if(!session.context || !session.world || !session.gate.current.observation){reason=3;goto done;}
    for(uint32_t i=0;i<7;i++)if((uintptr_t)ret==game.base+callers[i]){ci=i;break;}
    if(ci==7){reason=4;goto done;}
    for(uint32_t i=0;i<7;i++)if(command==commands[i]){si=i;break;}
    if(si==7){reason=5;goto done;}
    if(!diagnostic_pointer((uintptr_t)hud) ||
       !read_local(NULL,(uintptr_t)hud,&hvt,8) ||
       !read_local(NULL,(uintptr_t)hud+0x3b0,&controller,8) || !diagnostic_pointer(controller) ||
       !read_local(NULL,controller+0x3e8,&actor,8) || actor!=session.gate.current.provider ||
       !diagnostic_pointer(actor) || !read_local(NULL,actor,&vt,8) ||
       (vt!=game.base+0x3735bc8 && vt!=game.base+0x373d490)) {reason=6;goto done;}
    /* HUD vtable is recorded, not guessed. Its exact identity needs live proof. */
    if(row) {
        if((uintptr_t)row<0x10000 || (uintptr_t)row>0x7fffffffffffULL-12 || ((uintptr_t)row&3) ||
           !read_local(NULL,(uintptr_t)row,&bytes,8) ||
           !read_local(NULL,(uintptr_t)row+8,&tail,4) || (bytes&255)!=command){reason=7;goto done;}
    } else if(ci!=0 && ci!=4){reason=7;goto done;}
    if(!read_local(NULL,controller+0x3e8,&again,8) || again!=actor ||
       !read_local(NULL,(uintptr_t)hud+0x3b0,&again,8) || again!=controller){reason=8;goto done;}
    menu_observation.epoch=seen_epoch;
    menu_observation.last_hud=(uintptr_t)hud;menu_observation.last_vtable=hvt;
    menu_observation.last_actor=actor;menu_observation.last_caller=callers[ci];
    menu_observation.last_command=command;menu_observation.last_row=(uintptr_t)row;
    menu_observation.last_result=result&255;menu_observation.last_row_bytes=bytes;
    menu_observation.last_row_tail=tail;
    magic_inc(&menu_observation.accepted[ci][si]);
    if(result&255)magic_inc(&menu_observation.native_true[ci][si]);
    /* Only observed display/insertion/shortcut execution paths may change.
     * Selection helpers and unknown command categories retain native behavior. */
    uint32_t owned=0;
    int grand=si==1 || si==3 || si==5;
    if((result&255) && ((grand && (ci==0 || ci==4)) ||
                       (!grand && (ci==3 || ci==5 || ci==6))) &&
       Kh02MagicOwnership(&owned) && !(owned&(1u<<si))) {
        magic_inc(&menu_observation.blocked[ci][si]);result&=~(uint64_t)255;
    }
done:
    magic_inc(&menu_observation.reasons[reason]);InterlockedDecrement(&flight);return result;
}
void Kh02MenuObservationSnapshot(Kh02MenuObservation *out) {
    if(out)memcpy(out,&menu_observation,sizeof(*out));
}
