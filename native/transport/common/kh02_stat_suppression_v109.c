#include "kh02_stat_suppression_v109.h"
#include <string.h>
typedef struct Snapshot {
    uint64_t root,instance,current,vtable,manager,manager_vtable,getter,table;
    uint64_t names[7];
    uint32_t character;
    unsigned char bonuses[32],header[80],bitmap[16],entries[128*24],rows[128*48];
} Snapshot;
typedef struct Reader {Kh02BonusReadV109 fn;void *opaque;uint32_t used;} Reader;
static int pointer(uint64_t p){return p>=0x10000 && p<=0x7fffffffffffULL-0x10000 && !(p&7);}
static int read(Reader *r,uint64_t p,void *out,uint32_t n) {
    if(!n || n>6144 || p<0x10000 || p>0x7fffffffffffULL-n || r->used>32768-n)return 0;
    r->used+=n;return r->fn(r->opaque,p,out,n);
}
static uint64_t u64(const unsigned char *p){uint64_t x;memcpy(&x,p,8);return x;}
static int32_t i32(const unsigned char *p){int32_t x;memcpy(&x,p,4);return x;}
static int capture(Reader *r,const Kh02BonusRequestV109 *q,Snapshot *s) {
    static const unsigned char codes[7]={2,3,1,3,1,2,4};
    unsigned char zero[32]={0};uint32_t seen=0;int named=0;
    memset(s,0,sizeof(*s));
    if(!read(r,q->base+0x480b5d0,&s->root,8) || !pointer(s->root) ||
       !read(r,s->root+0x230,&s->instance,8) || !pointer(s->instance) ||
       !read(r,s->instance+0x50,&s->current,8) || s->current!=q->context ||
       !read(r,q->context,&s->vtable,8) || s->vtable!=q->base+0x36df228 ||
       !read(r,q->context+0x2934,&s->character,4) || s->character!=6 ||
       !read(r,q->context+0x2994,s->bonuses,32) || memcmp(s->bonuses,zero,32))return 0;
    /* No migration: only an empty native primary history is admitted. */
    if(!read(r,q->context+0x7e08,&s->manager,8) || !pointer(s->manager) ||
       !read(r,s->manager,&s->manager_vtable,8) || s->manager_vtable!=q->base+0x36f7198 ||
       !read(r,s->manager_vtable+0x240,&s->getter,8) || s->getter!=q->base+0x2cbe20 ||
       !read(r,s->manager+0x50,&s->table,8) || !pointer(s->table) ||
       !read(r,s->table+0x48,s->header,80) ||
       !read(r,q->base+0x45342b0+42*8,s->names,56))return 0;
    for(unsigned i=0;i<7;i++) {
        if(!s->names[i])return 0;
        for(unsigned j=0;j<i;j++)if(s->names[i]==s->names[j])return 0;
    }
    int32_t count=i32(s->header+8),capacity=i32(s->header+12);
    int32_t bits=i32(s->header+0x28),bit_capacity=i32(s->header+0x2c);
    uint64_t data=u64(s->header),external=u64(s->header+0x20);
    if(count<=0 || count>capacity || capacity>128 || bits!=count || bit_capacity<bits || bit_capacity>128 || !pointer(data))return 0;
    uint32_t mask_size=((uint32_t)bits+31)/32*4;
    if(external) {if(!pointer(external) || !read(r,external,s->bitmap,mask_size))return 0;}
    else memcpy(s->bitmap,s->header+0x10,mask_size);
    if(!read(r,data,s->entries,(uint32_t)count*24))return 0;
    for(int32_t i=0;i<count;i++) {
        if(!(s->bitmap[i/8]&(1u<<(i%8))))continue;
        uint64_t row=u64(s->entries+i*24+8);unsigned char *value=s->rows+i*48;
        if(!pointer(row) || !read(r,row,value,48))return 0;
        uint64_t condition=u64(value+8);
        if(value[0x10]&1)return 0; /* No objective-conditioned bonus suppression. */
        if(!condition)continue;
        unsigned index=0;while(index<7 && condition!=s->names[index])index++;
        if(index==7 || (seen&(1u<<index)))return 0;
        if(value[0x28]!=codes[index] || memcmp(value+0x29,zero,5))return 0;
        seen|=1u<<index;if(condition==q->name)named=1;
    }
    /* All current battle rows are player-only. Any companion bonus/change
       refuses interception instead of dropping unrelated native behavior. */
    return seen==127 && (q->kind==2 || named);
}
uint32_t Kh02BonusSuppressV109(Kh02BonusReadV109 fn,void *opaque,const Kh02BonusRequestV109 *q) {
    Snapshot a,b;Reader r={fn,opaque,0};
    if(!fn || !q || !q->authorized || !pointer(q->base) || !pointer(q->context) ||
       q->base>0x7fffffffffffULL-0x04f5f000 ||
       (q->kind!=1 && q->kind!=2) || q->subrecord!=q->context+0x2930 ||
       (q->kind==1 && (!q->name || q->objective!=-1)))return 0;
    return capture(&r,q,&a) && capture(&r,q,&b) && !memcmp(&a,&b,sizeof(a));
}
