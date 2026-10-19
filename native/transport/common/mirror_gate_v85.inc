/* Diagnostic mirror access replacement, scoped to the live-observed challenge
 * Blueprint. Collection display and native inventory queries retain vanilla
 * results. Objective43 side effects remain a live-test question. */
static void mirror_gate_v85(ZodiacQueryV83 *row,void *result) {
    uint64_t root,instance,current,table,actor_class,node_outer;
    uint32_t mask=0;unsigned char answer=0;
    if((row->flags&120)!=120 ||
       strcmp(row->node_name,"ExecuteUbergraph_dw_03_challenge") ||
       strcmp(row->node_outer,"dw_03_challenge_C") ||
       strcmp(row->object_name,"dw_03_challenge_C") ||
       memcmp(row->map,"/Game/Levels/dw/dw_03/dw_03",sizeof("/Game/Levels/dw/dw_03/dw_03")))return;
    row->flags|=128; /* Exact named challenge in WW; default locked. */
    if(row->owner && row->epoch && row->cookie && active_flag && !fault && !busy &&
       zodiac_read(game.base+0x480b5d0,&root,8) && root &&
       zodiac_read(root+0x230,&instance,8) && instance &&
       zodiac_read(instance+0x50,&current,8) && current==row->owner &&
       zodiac_read(current,&table,8) && table==game.base+0x36df228 &&
       zodiac_read(row->object+0x18,&actor_class,8) &&
       zodiac_read(row->node+0x28,&node_outer,8) && actor_class==node_outer &&
       Kh02AcceptedGoalAccessV80(&mask) &&
       session.context==row->owner && Kh02LifecycleEpoch(life)==row->epoch &&
       session.gate.current.cookie==row->cookie) {
        answer=(mask&2)?1:0;
        row->flags|=256; /* Private accepted-history decision available. */
    }
    if(answer)row->flags|=512;
    if(answer!=row->result)row->flags|=1024;
    /* Original wrapper already wrote this exact one-byte result successfully.
     * Preserve row.result as the original value for audit. */
    memcpy(result,&answer,1);
}
