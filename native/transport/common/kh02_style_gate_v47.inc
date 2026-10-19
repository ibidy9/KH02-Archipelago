/* Independent style request gate. Does not alter meter, records or existing UI. */
#include "kh02_style_gate_v47.h"
static Kh02StyleGate style_gate;
uint32_t Kh02StyleReceiptReady(uint64_t item) {
    uint32_t owned=0;
    return item==0x020208 && is_active(NULL) && session.context && session.world &&
        session.gate.current.observation && Kh02StyleOwnership(&owned) && owned;
}
uint32_t kh02_style_request_gate(void *receiver,uint32_t command,void *caller) {
    uint32_t reason=0,index=2,owned=0,blocked=0;
    uint64_t controller=(uintptr_t)receiver,actor=0,vt=0,cvt=0,fn=0,again=0;
    unsigned char kind=0;
    InterlockedIncrement(&flight);magic_inc(&style_gate.raw);
    if(GetCurrentThreadId()!=owner_thread){reason=1;goto done;}
    if(!is_active(NULL) || !session.context || !session.world ||
       !session.gate.current.observation){reason=2;goto done;}
    if(command==0x70)index=0;else if(command==0x86)index=1;
    else {reason=3;goto done;}
    if(!diagnostic_pointer(controller) || !read_local(NULL,controller,&cvt,8) ||
       (cvt!=game.base+0x37305d0 && cvt!=game.base+0x3734480) ||
       !read_local(NULL,cvt+0x1330,&fn,8) || fn!=game.base+0x503760 ||
       !read_local(NULL,controller+0x3e8,&actor,8) || actor!=session.gate.current.provider ||
       !diagnostic_pointer(actor) || !read_local(NULL,actor,&vt,8) ||
       (vt!=game.base+0x3735bc8 && vt!=game.base+0x373d490) ||
       !read_local(NULL,actor+0x5d0,&kind,1) || kind!=6){reason=4;goto done;}
    if(!read_local(NULL,controller+0x3e8,&again,8) || again!=actor ||
       !read_local(NULL,controller,&again,8) || again!=cvt){reason=5;goto done;}
    if(!Kh02StyleOwnership(&owned)){reason=6;goto done;}
    style_gate.epoch=seen_epoch;style_gate.controller=controller;style_gate.actor=actor;
    style_gate.command=command;style_gate.caller=(uintptr_t)caller;style_gate.owned=owned;
    magic_inc(&style_gate.accepted[index]);
    if(!owned){magic_inc(&style_gate.blocked[index]);blocked=1;}
done:
    magic_inc(&style_gate.reasons[reason]);InterlockedDecrement(&flight);return blocked;
}
void Kh02StyleGateSnapshot(Kh02StyleGate *out) {
    if(out)memcpy(out,&style_gate,sizeof(*out));
}
