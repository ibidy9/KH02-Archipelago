#include "resource_plan_v276.h"
static Kh02StatView resource_last211,resource_source211;
static uint32_t resource_last_hp211,resource_last_mp211,resource_source_hp211,resource_source_mp211;
static uint32_t resource_pending211,resource_wait211;

static void resource_begin211(uint64_t context) {
    Kh02SessionSample sample,again;uint32_t current[3];
    if(GetCurrentThreadId()!=owner_thread || !active_flag || fault || !life ||
       !resource_last211.owner || context!=resource_last211.owner ||
       Kh02LifecycleEpoch(life)!=resource_last211.epoch)return;
    /* Read only: the route may already be marked pending. Verify the same
     * current-owner chain twice; never infer fresh current values from a timer. */
    if(Kh02OwnerReadSample(read_local,NULL,game.base,game.cache,session.gate.current.generation,&sample)!=KH02_GATE_OK ||
       sample.context!=context || sample.attributes!=resource_last211.attributes ||
       !read_local(NULL,sample.provider+0x5dc,current,12) ||
       Kh02OwnerReadSample(read_local,NULL,game.base,game.cache,session.gate.current.generation,&again)!=KH02_GATE_OK ||
       memcmp(&sample,&again,sizeof(sample)) || current[0]>resource_last211.maximum[0] ||
       current[2]>resource_last211.maximum[1])return;
    resource_source211=resource_last211;
    resource_source211.current[0]=current[0];resource_source211.current[1]=current[2];
    resource_source_hp211=resource_last_hp211;resource_source_mp211=resource_last_mp211;
    resource_pending211=1;resource_wait211=4;
    ResourceRow210 row={0};row.kind=4;row.status=1;row.before=row.after=resource_source211;
    row.hp_items=resource_source_hp211;row.mp_items=resource_source_mp211;resource_record210(&row);
}

static uint32_t resource_write211(const Kh02StatView *before,const uint32_t target[2]) {
    Kh02StatView current,expected;Kh02SessionSample sample;SIZE_T wrote;
    if(!stat_read(NULL,&current) || memcmp(before,&current,sizeof(current)) ||
       !before->current[0] || target[0]>before->maximum[0] || target[1]>before->maximum[1] ||
       Kh02OwnerReadSample(read_local,NULL,game.base,game.cache,session.gate.current.generation,&sample)!=KH02_GATE_OK ||
       sample.context!=before->owner || sample.attributes!=before->attributes ||
       !stat_read(NULL,&current) || memcmp(before,&current,sizeof(current)))return 2;
    expected=*before;
    for(uint32_t i=0;i<2;i++) {
        if(target[i]==before->current[i])continue;
        wrote=0;
        if(!WriteProcessMemory(GetCurrentProcess(),(void*)(uintptr_t)(sample.provider+(i?0x5e4:0x5dc)),&target[i],4,&wrote) || wrote!=4)
            return 3; /* Never retry an uncertain partial pair. */
        expected.current[i]=target[i];
        if(!stat_read(NULL,&current) || memcmp(&expected,&current,sizeof(current)))return 3;
    }
    return 1;
}

static uint32_t resource_finish211(Kh02StatProjection *p,uint32_t status,uint32_t hp,uint32_t mp) {
    if(resource_pending211 && (resource_source211.epoch!=Kh02LifecycleEpoch(life) ||
       (p->before.owner && p->before.owner!=resource_source211.owner)))resource_pending211=0;
    if(status!=1)return status;
    if(resource_pending211) {
        uint32_t target[2],disabled_castle_magic=0;unsigned char bits;char map[64];
        if(p->before.owner && read_local(NULL,p->before.owner+0x28ac,&bits,1) && !(bits&4) &&
           read_local(NULL,p->before.owner+0x384c,map,64) &&
           (!memcmp(map,"/Game/Levels/dw/dw_01/dw_01",28) || !memcmp(map,"/Game/Levels/dw/dw_02/dw_02",28)))disabled_castle_magic=1;
        if(resource_plan276(&resource_source211,resource_source_hp211,resource_source_mp211,p,hp,mp,disabled_castle_magic,target)) {
            resource_pending211=0;
            Kh02StatView before=p->after;
            status=resource_write211(&before,target);
            ResourceRow210 row={0};row.kind=3;row.status=status;row.before=before;
            row.hp_items=hp;row.mp_items=mp;
            if(stat_read(NULL,&row.after))p->after=row.after;
            resource_record210(&row);
        } else if(p->before.attributes==resource_source211.attributes &&
                  !memcmp(p->before.current,resource_source211.current,8) &&
                  !memcmp(p->before.maximum,resource_source211.maximum,8) && resource_wait211) {
            /* The captured native route briefly exposes the old actor before
             * replacement. Bound this allowance; any resource change cancels it. */
            resource_wait211--;
        } else resource_pending211=0;
    }
    if(status==1){resource_last211=p->after;resource_last_hp211=hp;resource_last_mp211=mp;}
    return status;
}
