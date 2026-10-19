/* History-derived projection, through the existing game-thread owner only. */
extern uint32_t Kh02MovementOwnership209(uint32_t*);
#include "movement_projection_v209.inc"
static uint32_t movement_reconcile209(void) {
    uint32_t mask=0,status;Kh02MeterProjection result;
    if(GetCurrentThreadId()!=owner_thread || !is_active(NULL) ||
       !session.context || !session.world || !session.gate.current.observation ||
       InterlockedCompareExchange(&busy,0,0) || !Kh02MovementOwnership209(&mask))return 0;
    status=Kh02MovementProject209(read_local,meter_write,NULL,session.world,session.context,mask,&result);
    if(status==KH02_GATE_UNCERTAIN) {InterlockedExchange(&fault,1);InterlockedExchange(&active_flag,0);}
    return status==KH02_GATE_OK;
}
uint32_t Kh02MovementReceiptReady209(uint64_t item) {
    uint32_t mask=0,bit=item==0x020205?1:item==0x020206?2:item==0x020207?16:0;
    return bit && Kh02MovementOwnership209(&mask) && (mask&bit) && movement_reconcile209();
}
