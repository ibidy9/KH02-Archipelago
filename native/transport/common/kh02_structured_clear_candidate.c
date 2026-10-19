#include "kh02_structured_clear_candidate.h"
#include <string.h>
#define PAIR_DISTANCE 0x3d28u
#define NATIVE_GUARD 0x27f8u
#define NATIVE_SUMMARY 0x3206u
/* Preserve the authentic normal58/NG+65 default, explicitly initialized at
   RVA434AF4. Never normalize this block to zero or import completed progress. */
static const unsigned char clean_guard[0x34]={[4]=7};
static const unsigned char clean_summary[3]={2,0,0};
static const unsigned char fresh_summary[3]={0,0,0};
static uint32_t writable(uintptr_t address,uint32_t size) {
    MEMORY_BASIC_INFORMATION m;
    if(!address || address>UINTPTR_MAX-size || !VirtualQuery((void*)address,&m,sizeof(m)))return 0;
    return m.State==MEM_COMMIT && !(m.Protect&(PAGE_GUARD|PAGE_NOACCESS)) &&
        (m.Protect&(PAGE_READWRITE|PAGE_WRITECOPY)) &&
        address+size>address && m.RegionSize<=UINTPTR_MAX-(uintptr_t)m.BaseAddress &&
        address+size<=(uintptr_t)m.BaseAddress+m.RegionSize;
}
static uint32_t validate(uintptr_t state,unsigned char before[3]) {
    if(!state || state>UINTPTR_MAX-NATIVE_SUMMARY-3 ||
       !writable(state+NATIVE_GUARD,sizeof(clean_guard)) ||
       !writable(state+NATIVE_SUMMARY,3))return 2;
    if(memcmp((void*)(state+NATIVE_GUARD),clean_guard,sizeof(clean_guard)))return 3;
    memcpy(before,(void*)(state+NATIVE_SUMMARY),3);
    return !memcmp(before,fresh_summary,3) || !memcmp(before,clean_summary,3)?1:4;
}
static uint32_t clear_initialize(uintptr_t primary,uintptr_t checkpoint) {
    unsigned char a[3],b[3];uint32_t status;
    if(checkpoint<=primary || checkpoint-primary!=PAIR_DISTANCE)return 2;
    status=validate(primary,a);if(status!=1)return status;
    status=validate(checkpoint,b);if(status!=1)return status;
    if(memcmp(a,b,3))return 4;
    /* Original copy, caller, authority and inventory checks belong to the
       existing fresh owner. No Critical-setting or serialized-header writes.
       The serializer derives flags5 from the resulting clear progression. */
    memcpy((void*)(primary+NATIVE_SUMMARY),clean_summary,3);
    memcpy((void*)(checkpoint+NATIVE_SUMMARY),clean_summary,3);
    if(memcmp((void*)(primary+NATIVE_SUMMARY),clean_summary,3) ||
       memcmp((void*)(checkpoint+NATIVE_SUMMARY),clean_summary,3)) {
        memcpy((void*)(primary+NATIVE_SUMMARY),a,3);
        memcpy((void*)(checkpoint+NATIVE_SUMMARY),b,3);return 5;
    }
    return 1;
}

/* Fresh-owner-only addition. Exact native objective array: state+0xc20,
 * 51 records of four bytes. Low nibble: Wait0, Progress1, Clear2.
 * Native StartChallenge at 43cbf0 preserves upper flags and sets Progress.
 * Fresh admission only: reject counters, completed/error states and unknown
 * flags. Never run on a loaded/adopted save; caller retains the fresh lease.
 */
static uint32_t objective_initialize(uintptr_t primary,uintptr_t checkpoint) {
    unsigned char before[204],other[204],after[204],sa[3],sb[3];uint32_t i,status;
    if(checkpoint<=primary || checkpoint-primary!=PAIR_DISTANCE ||
       primary>UINTPTR_MAX-NATIVE_SUMMARY-3 || checkpoint>UINTPTR_MAX-NATIVE_SUMMARY-3 ||
       !writable(primary+0xc20,204) || !writable(checkpoint+0xc20,204))return 6;
    memcpy(before,(void*)(primary+0xc20),204);
    memcpy(other,(void*)(checkpoint+0xc20),204);
    if(memcmp(before,other,204))return 7;
    memcpy(after,before,204);
    for(i=0;i<51;i++) {
        unsigned char *p=after+i*4;
        if((p[0]&0xef)>1 || p[1] || p[2] || p[3])return 8;
        p[0]=(p[0]&0xf0)|1;
    }
    status=validate(primary,sa);if(status!=1)return status;
    status=validate(checkpoint,sb);if(status!=1)return status;
    status=clear_initialize(primary,checkpoint);if(status!=1)return status;
    memcpy((void*)(primary+0xc20),after,204);
    memcpy((void*)(checkpoint+0xc20),after,204);
    if(memcmp((void*)(primary+0xc20),after,204) || memcmp((void*)(checkpoint+0xc20),after,204)) {
        memcpy((void*)(primary+0xc20),before,204);
        memcpy((void*)(checkpoint+0xc20),other,204);
        memcpy((void*)(primary+NATIVE_SUMMARY),sa,3);
        memcpy((void*)(checkpoint+NATIVE_SUMMARY),sb,3);
        return 9;
    }
    return 1;
}

#include "kh02_fresh_menu_plan_v52.h"
/* Same authenticated fresh boundary, before the gameplay HUD exists. Stage
 * both records first. Only commit historical menu fields, never whole records.
 * Let normal HUD creation consume initialized shortcuts; no stale UI replay. */
uint32_t Kh02StructuredClearInitialize(uintptr_t primary,uintptr_t checkpoint) {
    unsigned char a[PAIR_DISTANCE],b[PAIR_DISTANCE],pa[PAIR_DISTANCE],pb[PAIR_DISTANCE];
    static const uint32_t offsets[]={0x3c58,0x3c59,
        0x3c68,0x3c6c,0x3c70,0x3c74};
    uint32_t i,status;
    if(checkpoint<=primary || checkpoint-primary!=PAIR_DISTANCE ||
       !writable(primary,PAIR_DISTANCE) || !writable(checkpoint,PAIR_DISTANCE))return 10;
    memcpy(a,(void*)primary,PAIR_DISTANCE);memcpy(b,(void*)checkpoint,PAIR_DISTANCE);
    if(!Kh02FreshMenuPlanV52(a,pa,PAIR_DISTANCE) ||
       !Kh02FreshMenuPlanV52(b,pb,PAIR_DISTANCE))return 11;
    for(i=0;i<6;i++) {
        uint32_t n=i<2?1:4;
        if(memcmp(a+offsets[i],b+offsets[i],n))return 12;
    }
    status=objective_initialize(primary,checkpoint);if(status!=1)return status;
    for(i=0;i<6;i++) {
        uint32_t n=i<2?1:4;
        memcpy((void*)(primary+offsets[i]),pa+offsets[i],n);
        memcpy((void*)(checkpoint+offsets[i]),pb+offsets[i],n);
    }
    for(i=0;i<6;i++) {
        uint32_t n=i<2?1:4;
        if(memcmp((void*)(primary+offsets[i]),pa+offsets[i],n) ||
           memcmp((void*)(checkpoint+offsets[i]),pb+offsets[i],n))break;
    }
    if(i==6)return 1;
    for(i=0;i<6;i++) {
        uint32_t n=i<2?1:4;
        memcpy((void*)(primary+offsets[i]),a+offsets[i],n);
        memcpy((void*)(checkpoint+offsets[i]),b+offsets[i],n);
    }
    memcpy((void*)(primary+0xc20),a+0xc20,204);
    memcpy((void*)(checkpoint+0xc20),b+0xc20,204);
    memcpy((void*)(primary+NATIVE_SUMMARY),a+NATIVE_SUMMARY,3);
    memcpy((void*)(checkpoint+NATIVE_SUMMARY),b+NATIVE_SUMMARY,3);
    return 13;
}
