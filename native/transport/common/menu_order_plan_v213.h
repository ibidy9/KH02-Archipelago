#ifndef MENU_ORDER_PLAN_213
#define MENU_ORDER_PLAN_213
#include <stdint.h>
#include <string.h>
/* Whole native rows move together, including their owned FString pointers.
 * No allocation, duplication, freeing or mutation of pointed-to strings. */
static int menu_plan213(const unsigned char *rows,const uint32_t *catalog,
        uint32_t count,int32_t focus,unsigned char *out,uint32_t *sorted,int32_t *new_focus) {
    static const uint32_t rank[18]={0,2,4,6,7,8,10,11,12,13,15,16,1,3,5,9,14,17};
    uint32_t order[18],seen=0;
    if(!rows || !catalog || !out || !sorted || !new_focus || count>18 || !count ||
       focus<0 || (uint32_t)focus>=count)return 0;
    for(uint32_t i=0;i<count;i++) {
        if(catalog[i]>=18 || (seen&(1u<<catalog[i])))return 0;
        seen|=1u<<catalog[i];order[i]=i;
    }
    for(uint32_t i=1;i<count;i++) {
        uint32_t value=order[i],j=i;
        while(j && rank[catalog[order[j-1]]]>rank[catalog[value]]) {order[j]=order[j-1];j--;}
        order[j]=value;
    }
    for(uint32_t i=0;i<count;i++) {
        memcpy(out+i*0x38,rows+order[i]*0x38,0x38);sorted[i]=catalog[order[i]];
        if(order[i]==(uint32_t)focus)*new_focus=(int32_t)i;
    }
    return 1;
}
#endif
