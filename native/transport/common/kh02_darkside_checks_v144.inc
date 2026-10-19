/* Included after the proven event row definition. No game/save writes.
 * One bit per confirmed group, retained for the immutable prepared cookie.
 * Python's existing seed-scoped journal/AP acknowledgements own persistence. */
static uint64_t darkside_cookie_v144,darkside_hits_v144;
static uint32_t darkside_mask_v144,darkside_reason_v144;
static int darkside_source_v144(uint64_t context) {
    uint64_t root=0,instance=0,current=0,table=0;char map[32];
    return context && !(context&7) &&
        read_local(NULL,game.base+0x480b5d0,&root,8) && root &&
        read_local(NULL,root+0x230,&instance,8) && instance &&
        read_local(NULL,instance+0x50,&current,8) && current==context &&
        read_local(NULL,context,&table,8) && table==game.base+0x36df228 &&
        read_local(NULL,context+0x384c,map,sizeof(map)) &&
        !memcmp(map,"/Game/Levels/dw/dw_08/dw_08",26);
}
static void darkside_complete_v144(const WipeoutRowV143 *r) {
    uint64_t cookie,epoch;Kh02ReceiptOwner *receipt;
    if(GetCurrentThreadId()!=owner_thread || r->thread!=owner_thread ||
       (r->match!=1 && r->match!=2 && r->match!=4))return;
    darkside_hits_v144++;darkside_reason_v144=1;
    cookie=session.gate.current.cookie;epoch=Kh02LifecycleEpoch(life);
    receipt=Kh02LifecycleCurrent(life);
    if(!active_flag || fault || busy || !cookie || !epoch || !receipt ||
       r->epoch!=epoch || seen_epoch!=epoch || !r->enemy || !r->receiver ||
       r->context!=session.context || (r->flags&15u)!=15u ||
       !Kh02ControlledStartAuthorized() || Kh02ReceiptOwnerCheck(receipt)!=KH02_MEMBER_OK)return;
    darkside_reason_v144=2;
    if(!darkside_source_v144(r->context))return;
    darkside_reason_v144=3;
    if(!active_flag || fault || busy || cookie!=session.gate.current.cookie ||
       epoch!=Kh02LifecycleEpoch(life) || receipt!=Kh02LifecycleCurrent(life) ||
       r->context!=session.context || !Kh02ControlledStartAuthorized() ||
       Kh02ReceiptOwnerCheck(receipt)!=KH02_MEMBER_OK || !darkside_source_v144(r->context))return;
    if(darkside_cookie_v144!=cookie){darkside_cookie_v144=cookie;darkside_mask_v144=0;}
    darkside_mask_v144|=r->match;darkside_reason_v144=4;
}
