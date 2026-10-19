/* Development only. Fixed current-owner derived Strength/Magic/Defense, never saves. */
static uint32_t boost_enabled,boost_failed,boost_strength,boost_magic,boost_defense;
static uint64_t boost_attributes,boost_epoch;
static int boost_pair(uint64_t a,uint32_t strength,uint32_t magic,uint32_t defense) {
    uint32_t before[3],after[3];
    if(!read_local(NULL,a+0x5c,before,12))return 0;
    if(!start_write(a+0x5c,strength) || !start_write(a+0x60,magic) || !start_write(a+0x64,defense) ||
       !read_local(NULL,a+0x5c,after,12) || after[0]!=strength || after[1]!=magic || after[2]!=defense) {
        int x=start_write(a+0x5c,before[0]),y=start_write(a+0x60,before[1]),z=start_write(a+0x64,before[2]);
        if(!x || !y || !z || !read_local(NULL,a+0x5c,after,12) || memcmp(before,after,12)) {
            InterlockedExchange(&fault,1);InterlockedExchange(&active_flag,0);
        }
        boost_failed=1;boost_enabled=0;return 0;
    }
    return 1;
}
static int boost_admit(Kh02SessionSample *s) {
    char map[32];
    return GetCurrentThreadId()==owner_thread && is_active(NULL) && Kh02ControlledStartAuthorized() &&
        Kh02OwnerReadSample(read_local,NULL,game.base,game.cache,session.gate.current.generation,s)==KH02_GATE_OK &&
        s->phase==3 && !s->title_flags && !s->pending_map && !(s->travel_flags&1) && !s->world_flag &&
        s->context==session.context && s->world==session.world && s->attributes==session.gate.current.attributes &&
        read_local(NULL,s->context+0x384c,map,32) &&
        (!memcmp(map,"/Game/Levels/dw/dw_02/dw_02",26) || !memcmp(map,"/Game/Levels/dw/dw_08/dw_08",26) || !memcmp(map,"/Game/Levels/dw/dw_03/dw_03",26) || !memcmp(map,"/Game/Levels/dw/dw_11/dw_11",26) || !memcmp(map,"/Game/Levels/dw/dw_12/dw_12",26) || !memcmp(map,"/Game/Levels/dw/dw_07/dw_07",26) || !memcmp(map,"/Game/Levels/dw/dw_09/dw_09",26) || !memcmp(map,"/Game/Levels/dw/dw_10/dw_10",26));
}
static int boost_apply(Kh02SessionSample *s) {
    uint32_t v[3];uint64_t epoch=Kh02LifecycleEpoch(life);
    if(boost_failed || !read_local(NULL,s->attributes+0x5c,v,12) || v[0]>999 || v[1]>999 || v[2]>999)return 0;
    if(boost_attributes!=s->attributes || boost_epoch!=epoch) {
        boost_attributes=s->attributes;boost_epoch=epoch;boost_strength=v[0];boost_magic=v[1];boost_defense=v[2];
    } else {if(v[0]!=999)boost_strength=v[0];if(v[1]!=999)boost_magic=v[1];if(v[2]!=999)boost_defense=v[2];}
    return boost_pair(s->attributes,999,999,999);
}
static void Kh02CombatBoostTick(void) {
    Kh02SessionSample s;
    if(!boost_enabled || boost_failed || !boost_admit(&s) || InterlockedCompareExchange(&busy,1,0))return;
    if(!boost_apply(&s)){boost_enabled=0;boost_failed=1;}
    InterlockedExchange(&busy,0);
}

