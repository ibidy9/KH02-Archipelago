/* Observe four static style-command candidates; preserve every v45 result. */
#define kh02_menu_observe kh02_menu_observe_v45
#include "kh02_menu_gate_v45.inc"
#undef kh02_menu_observe
#include "kh02_situation_observer_v46.h"
static Kh02SituationObservation situation_observation;
uint64_t kh02_menu_observe(void *hud,uint32_t command,void *row,void *ret,uint64_t result) {
    static const uint32_t ids[4]={0x70,0x71,0x86,0x87};
    static const uint32_t callers[7]={0x57aaff,0x589821,0x589a9c,0x59b6e0,0x59bd9a,0x59e55c,0x59eb4c};
    uint32_t si=4,ci=7,reason=0;
    uint64_t controller=0,actor=0,vt=0,cvt=0,again=0;
    unsigned char kind=0,first=0,second=0,row_id=0;
    Kh02InventorySnapshot inventory;
    InterlockedIncrement(&flight);magic_inc(&situation_observation.raw);
    if(GetCurrentThreadId()!=owner_thread){reason=1;goto done;}
    if(!is_active(NULL) || !session.context || !session.world ||
       !session.gate.current.observation){reason=2;goto done;}
    for(uint32_t i=0;i<7;i++)if((uintptr_t)ret==game.base+callers[i]){ci=i;break;}
    if(ci==7){reason=3;goto done;}
    for(uint32_t i=0;i<4;i++)if(command==ids[i]){si=i;break;}
    if(si==4){reason=4;goto done;}
    if(!diagnostic_pointer((uintptr_t)hud) ||
       !read_local(NULL,(uintptr_t)hud+0x3b0,&controller,8) || !diagnostic_pointer(controller) ||
       !read_local(NULL,controller,&cvt,8) ||
       !read_local(NULL,controller+0x3e8,&actor,8) || actor!=session.gate.current.provider ||
       !diagnostic_pointer(actor) || !read_local(NULL,actor,&vt,8) ||
       (vt!=game.base+0x3735bc8 && vt!=game.base+0x373d490) ||
       !read_local(NULL,actor+0x5d0,&kind,1)){reason=5;goto done;}
    if(row) {
        if((uintptr_t)row<0x10000 || (uintptr_t)row>0x7fffffffffffULL-12 || ((uintptr_t)row&3) ||
           !read_local(NULL,(uintptr_t)row,&row_id,1) || row_id!=command){reason=6;goto done;}
    } else if(ci!=0 && ci!=4){reason=6;goto done;}
    if(Kh02ReadInventory(read_local,NULL,session.world,&inventory)!=KH02_GATE_OK ||
       inventory.primary<0x10f8c || inventory.reference<0x10f8c ||
       !read_local(NULL,inventory.primary-0xf8c,&first,1) ||
       !read_local(NULL,inventory.reference-0xf8c,&second,1) ||
       !read_local(NULL,controller+0x3e8,&again,8) || again!=actor ||
       !read_local(NULL,(uintptr_t)hud+0x3b0,&again,8) || again!=controller){reason=7;goto done;}
    situation_observation.epoch=seen_epoch;
    situation_observation.hud=(uintptr_t)hud;situation_observation.controller=controller;
    situation_observation.controller_vtable=cvt;situation_observation.actor=actor;
    situation_observation.character_kind=kind;situation_observation.command=command;
    situation_observation.caller=callers[ci];situation_observation.row=(uintptr_t)row;
    situation_observation.result=result;situation_observation.ability_primary=first;
    situation_observation.ability_reference=second;situation_observation.context=session.context;
    situation_observation.world=session.world;situation_observation.inventory_owner=inventory.owner;
    magic_inc(&situation_observation.accepted[ci][si]);
    if(result&255)magic_inc(&situation_observation.native_true[ci][si]);
done:
    magic_inc(&situation_observation.reasons[reason]);InterlockedDecrement(&flight);
    return kh02_menu_observe_v45(hud,command,row,ret,result);
}
void Kh02SituationObservationSnapshot(Kh02SituationObservation *out) {
    if(out)memcpy(out,&situation_observation,sizeof(*out));
}
