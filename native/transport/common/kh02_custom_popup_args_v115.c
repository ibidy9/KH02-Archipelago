#include "kh02_custom_popup_args_v115.h"
#include <stddef.h>
#include <string.h>
_Static_assert(sizeof(Kh02PopupValueV115)==56,"native UI value size");
_Static_assert(offsetof(Kh02PopupValueV115,text)==0x10,"native wide string");
_Static_assert(offsetof(Kh02PopupValueV115,narrow)==0x20,"native narrow string");
uint32_t Kh02CustomPopupArgsV115(Kh02PopupArgsV115 *out,const uint16_t *text,uint32_t length,const uint16_t *icon,uint32_t icon_length) {
    if(!out)return 0;
    memset(out,0,sizeof(*out));
    if(!icon || !icon_length || icon_length>260 || icon[icon_length])return 0;
    for(uint32_t i=0;i<icon_length;i++)if(icon[i]<32 || icon[i]==127)return 0;
    if(!text || !length || length>160 || text[length])return 0;
    for(uint32_t i=0;i<length;i++) {
        uint16_t c=text[i];
        if(c<32 || c==127 || c=='<' || c=='>' || c=='&')return 0;
        if(c>=0xd800 && c<=0xdbff) {
            if(i+1>=length || text[i+1]<0xdc00 || text[i+1]>0xdfff)return 0;
            i++;
        } else if(c>=0xdc00 && c<=0xdfff)return 0;
    }
    /* Converter 16359d0 maps type6 through164c390, borrowing text+10.
       No allocator or destructor is needed for these zero-initialized inputs. */
    out->values[0].type=6;out->values[0].text=(Kh02PopupStringV115){icon,(int32_t)icon_length+1,(int32_t)icon_length+1};
    out->values[1].type=6;
    out->values[1].text=(Kh02PopupStringV115){text,(int32_t)length+1,(int32_t)length+1};
    out->values[2].type=2;out->values[2].boolean=0;
    return 1;
}

/* Exact-build 4840b0 hash lookup, restricted to Potion (0x100). This follows
   one bounded hash chain, not a memory search. Native5911d0 uses row+38. */
static int icon_read(Kh02PopupReadV115 read,void *context,uint64_t a,void *out,uint32_t n) {
    return a>=0x10000 && a<=0x7fffffffffffULL-n && n<=64 && read(context,a,out,n)==1;
}
uint32_t Kh02PopupNativeIconV115(Kh02PopupReadV115 read,void *context,uint64_t manager,uint16_t out[261]) {
    uint64_t table,entries,buckets,row=0;int32_t count,free_count,bucket_count,index;
    struct {uint64_t pointer;int32_t count,capacity;} string,again;
    if(!read || !out || manager<0x10000 || manager>0x7ffffffff000ULL)return 0;
    memset(out,0,522);
#define R(a,v) icon_read(read,context,(a),&(v),sizeof(v))
    if(!R(manager+0x40,table) || !table || !R(manager+0x58,entries) ||
       !R(manager+0x60,count) || count<=0 || count>4096 ||
       !R(manager+0x8c,free_count) || free_count<0 || free_count>=count ||
       !R(manager+0xa0,bucket_count) || bucket_count<=0 || bucket_count>4096 ||
       (bucket_count&(bucket_count-1)) || !R(manager+0x98,buckets))return 0;
    if(!buckets)buckets=manager+0x90;
    if(buckets<0x10000 || buckets>0x7ffffffff000ULL || entries<0x10000 || entries>0x7ffffffe0000ULL ||
       !R(buckets+4*(0x100&(bucket_count-1)),index))return 0;
    for(uint32_t hops=0;hops<64;hops++) {
        uint32_t key;uint64_t entry;
        if(index<0 || index>=count)return 0;
        entry=entries+24*(uint32_t)index;
        if(!R(entry,key))return 0;
        if(key==0x100) {if(!R(entry+8,row))return 0;break;}
        if(!R(entry+0x10,index))return 0;
    }
    if(row<0x10000 || row>0x7ffffffff000ULL || !R(row+0x38,string) ||
       string.count<2 || string.count>261 || string.capacity<string.count || string.capacity>4096 ||
       string.pointer<0x10000 || string.pointer>0x7ffffffff000ULL)return 0;
    for(uint32_t i=0;i<(uint32_t)string.count;i++) {
        if(!R(string.pointer+2*i,out[i]) ||
           (i+1<(uint32_t)string.count && (out[i]<32 || out[i]==127))){memset(out,0,522);return 0;}
    }
    if(out[string.count-1] || !R(row+0x38,again) || memcmp(&string,&again,sizeof(string))) {
        memset(out,0,522);return 0;
    }
#undef R
    return (uint32_t)string.count-1;
}
