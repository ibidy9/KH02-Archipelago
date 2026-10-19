/* Development lock/unlock policy only; not yet AP item receipt integration.
   Owner thread, epoch, original receiver and transport authority must all agree.
   Revocation disables this experimental filter; it never grants game abilities. */
extern uint32_t Kh02ActionGatePermit(uint64_t authority);
static uint64_t action_epoch,action_authority,action_blocked[4];
static uint32_t action_mask;
uint32_t Kh02ActionGateSet(uint64_t epoch,uint64_t authority,uint32_t mask) {
    if(mask>15 || !authority || !is_active(NULL) || epoch!=seen_epoch ||
       !session.context || !session.world || !session.gate.current.observation ||
       !Kh02ActionGatePermit(authority))return 2;
    action_epoch=epoch;action_authority=authority;action_mask=mask;return 1;
}
void Kh02ActionGateCounts(uint64_t *recovery,uint64_t *counter,uint64_t *guard,uint64_t *dodge) {
    if(GetCurrentThreadId()!=owner_thread)return;
    *recovery=action_blocked[0];*counter=action_blocked[1];*guard=action_blocked[2];*dodge=action_blocked[3];
}
extern uint32_t Kh02ActionOwnership(uint32_t *mask);
static uint32_t action_should_block(uint32_t hook) {
    uint32_t owned=0;
    if(hook>3 || !Kh02ActionOwnership(&owned))return 0;
    if(owned&(1u<<hook))return 0;
    diagnostic_inc(&action_blocked[hook]);return 1;
}
uint32_t Kh02ActionReceiptReady(uint64_t item) {
    uint32_t mask=0,bit=item==0x020216?8:item==0x020217?4:item==0x020218?2:item==0x020219?1:0;
    return bit && is_active(NULL) && session.context && session.world &&
        session.gate.current.observation && Kh02ActionOwnership(&mask) && (mask&bit);
}

static void action_epoch_reset(void){action_epoch=0;action_authority=0;}
