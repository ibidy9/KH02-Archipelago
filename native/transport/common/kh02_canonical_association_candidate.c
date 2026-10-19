#include "kh02_canonical_association_candidate.h"
#include <bcrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct Buffer {unsigned char *p;uint32_t n,cap;} Buffer;
typedef struct Member {uint64_t revision;Buffer json;} Member;
static uint32_t u32(const unsigned char *p){uint32_t n;memcpy(&n,p,4);return n;}
static uint64_t u64(const unsigned char *p){uint64_t n;memcpy(&n,p,8);return n;}
static int add(Buffer *b,const void *p,uint32_t n) {
    if(n>b->cap-b->n)return 0;memcpy(b->p+b->n,p,n);b->n+=n;return 1;
}
static int text(Buffer *b,const char *p){return add(b,p,(uint32_t)strlen(p));}
static int number(Buffer *b,uint64_t n){char s[32];int size=snprintf(s,sizeof(s),"%llu",(unsigned long long)n);return size>0 && add(b,s,(uint32_t)size);}
static int string(Buffer *b,const WCHAR *wide,int n) {
    int i;char escape[7];
    if(!text(b,"\""))return 0;
    for(i=0;i<n;i++) {
        unsigned int ch=wide[i];const char *short_escape=NULL;
        switch(ch){case 8:short_escape="\\b";break;case 9:short_escape="\\t";break;
            case 10:short_escape="\\n";break;case 12:short_escape="\\f";break;case 13:short_escape="\\r";break;
            case '"':short_escape="\\\"";break;case '\\':short_escape="\\\\";break;}
        if(short_escape){if(!text(b,short_escape))return 0;}
        else if(ch<32 || ch>=127){snprintf(escape,sizeof(escape),"\\u%04x",ch);if(!add(b,escape,6))return 0;}
        else {unsigned char byte=(unsigned char)ch;if(!add(b,&byte,1))return 0;}
    }
    return text(b,"\"");
}
static int compare(const void *a,const void *b) {
    const Member *x=*(Member*const*)a,*y=*(Member*const*)b;
    return strcmp((const char*)x->json.p,(const char*)y->json.p);
}
static int hash_parts(const unsigned char *base,const Kh02CanonicalPart *parts,unsigned char result[32]) {
    BCRYPT_ALG_HANDLE algorithm=NULL;BCRYPT_HASH_HANDLE hash=NULL;unsigned char *object=NULL;
    DWORD length=0,written=0;uint32_t i;int ok=0;
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,NULL,0)<0)goto done;
    if(BCryptGetProperty(algorithm,BCRYPT_OBJECT_LENGTH,(PUCHAR)&length,sizeof(length),&written,0)<0 || written!=sizeof(length) || !length)goto done;
    object=HeapAlloc(GetProcessHeap(),0,length);if(!object)goto done;
    if(BCryptCreateHash(algorithm,&hash,object,length,NULL,0,0)<0)goto done;
    for(i=0;i<3;i++) {
        uint64_t size=parts[i].size;
        if(BCryptHashData(hash,(PUCHAR)&size,8,0)<0 || BCryptHashData(hash,(PUCHAR)(base+parts[i].offset),parts[i].size,0)<0)goto done;
    }
    ok=BCryptFinishHash(hash,result,32,0)>=0;
done:
    if(hash)BCryptDestroyHash(hash);
    if(object)HeapFree(GetProcessHeap(),0,object);
    if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);
    return ok;
}
const Kh02CanonicalAssociation *Kh02CanonicalCreate(const unsigned char *raw,uint32_t size,const unsigned char *token,uint32_t token_size) {
    const Kh02ReceiptBundle *bundle;Member members[64]={0},*sorted[64],*unique[64];
    uint32_t i,j,k,cursor,count=0,used[64]={0},indices[100],association_cap=2048,total,offset;
    WCHAR wide[4096];int wide_size,characters;Buffer scope={0},association={0};
    Kh02CanonicalPart set_parts[64]={0},scope_part={0},hash_input[3];Kh02CanonicalAssociation *out=NULL;DWORD old;
    if(!raw || (uintptr_t)raw%_Alignof(Kh02ReceiptBundle) || size<sizeof(Kh02ReceiptBundle)+1 || size>sizeof(Kh02ReceiptBundle)+4096+64*KH02_RECEIPT_MAX_BYTES ||
       !token || !token_size || token_size>1024)return NULL;
    wide_size=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,(const char*)token,(int)token_size,wide,4096);
    if(!wide_size)return NULL;
    for(i=0,characters=0;i<(uint32_t)wide_size;i++,characters++)if(wide[i]>=0xd800 && wide[i]<=0xdbff)i++;
    if(characters>256)return NULL;
    /* Native bundle allocations are aligned; refuse malformed unaligned input. */
    bundle=(const Kh02ReceiptBundle*)raw;
    if(!bundle->instance || !bundle->serial || !bundle->cookie || !bundle->load_id || bundle->size!=size || bundle->set_count>64 ||
       !bundle->seed_size || bundle->seed_size>4096 || bundle->seed_offset!=sizeof(*bundle) || bundle->seed_size>size-bundle->seed_offset ||
       memcmp(bundle->container,"SAVE",4) || u32(bundle->container+4)!=1 || u32(bundle->container+8)!=KH02_BIND_SIZE || u32(bundle->container+12))return NULL;
    wide_size=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,(const char*)raw+bundle->seed_offset,(int)bundle->seed_size,wide,4096);
    if(!wide_size)return NULL;
    scope.cap=24650;scope.p=HeapAlloc(GetProcessHeap(),0,scope.cap);if(!scope.p)goto failed;
    if(!text(&scope,"[") || !string(&scope,wide,wide_size) || !text(&scope,",") || !number(&scope,bundle->team) ||
       !text(&scope,",") || !number(&scope,bundle->slot) || !text(&scope,"]"))goto failed;
    cursor=bundle->seed_offset+bundle->seed_size;
    for(i=0;i<64;i++) {
        const Kh02ReceiptEntry *entry=&bundle->sets[i];const unsigned char *p;uint32_t n;uint64_t prior=0;
        if(i>=bundle->set_count){if(entry->revision || entry->offset || entry->size)goto failed;continue;}
        if(!entry->revision || entry->offset!=cursor || entry->size<56 || entry->size>KH02_RECEIPT_MAX_BYTES || entry->size>size-cursor)goto failed;
        for(j=0;j<i;j++)if(members[j].revision==entry->revision)goto failed;
        p=raw+cursor;n=u32(p+12);
        if(memcmp(p,"KH02RSET",8) || u32(p+8)!=1 || n>4096 || u32(p+52) ||
           u64(p+16)!=bundle->cookie || u64(p+24)!=bundle->load_id || u64(p+32)!=entry->revision ||
           u32(p+40)!=bundle->team || u32(p+44)!=bundle->slot || u32(p+48)!=bundle->seed_size ||
           entry->size!=56+bundle->seed_size+24*n || memcmp(p+56,raw+bundle->seed_offset,bundle->seed_size))goto failed;
        members[i].revision=entry->revision;members[i].json.cap=65*n+3;
        members[i].json.p=HeapAlloc(GetProcessHeap(),0,members[i].json.cap);if(!members[i].json.p)goto failed;
        if(!text(&members[i].json,"["))goto failed;p+=56+bundle->seed_size;
        for(j=0;j<n;j++,p+=24) {
            uint64_t index=u64(p);if(j && index<=prior)goto failed;prior=index;
            if((j && !text(&members[i].json,",")) || !text(&members[i].json,"[") || !number(&members[i].json,index) ||
               !text(&members[i].json,",") || !number(&members[i].json,u64(p+8)) || !text(&members[i].json,",") ||
               !number(&members[i].json,u64(p+16)) || !text(&members[i].json,"]"))goto failed;
        }
        if(!text(&members[i].json,"]"))goto failed;
        members[i].json.p[members[i].json.n]=0;sorted[i]=&members[i];cursor+=entry->size;
    }
    if(cursor!=size)goto failed;
    qsort(sorted,bundle->set_count,sizeof(*sorted),compare);
    for(i=0;i<bundle->set_count;i++)if(!count || strcmp((char*)unique[count-1]->json.p,(char*)sorted[i]->json.p)) {
        unique[count++]=sorted[i];association_cap+=sorted[i]->json.n+1;
    }
    for(i=0;i<100;i++) {
        indices[i]=UINT32_MAX;if(!bundle->revisions[i])continue;
        for(j=0;j<bundle->set_count;j++)if(members[j].revision==bundle->revisions[i])break;
        if(j==bundle->set_count)goto failed;used[j]=1;
        for(k=0;k<count;k++)if(!strcmp((char*)members[j].json.p,(char*)unique[k]->json.p))break;
        if(k==count)goto failed;indices[i]=k;
        for(j=0;j<i;j++)if(indices[j]!=UINT32_MAX && indices[j]!=k &&
            !memcmp(bundle->container+0xb0+j*KH02_BIND_RECORD,bundle->container+0xb0+i*KH02_BIND_RECORD,KH02_BIND_RECORD))goto failed;
    }
    for(i=0;i<bundle->set_count;i++)if(!used[i])goto failed;
    association.cap=association_cap+scope.n;association.p=HeapAlloc(GetProcessHeap(),0,association.cap);if(!association.p)goto failed;
    if(!text(&association,"{\"bindings\":["))goto failed;
    for(i=0,j=0;i<100;i++)if(indices[i]!=UINT32_MAX) {
        if((j++ && !text(&association,",")) || !text(&association,"[") || !number(&association,i) ||
           !text(&association,",") || !number(&association,indices[i]) || !text(&association,"]"))goto failed;
    }
    if(!text(&association,"],\"scope\":"))goto failed;scope_part=(Kh02CanonicalPart){association.n,scope.n};
    if(!add(&association,scope.p,scope.n) || !text(&association,",\"sets\":["))goto failed;
    for(i=0;i<count;i++) {
        if(i && !text(&association,","))goto failed;set_parts[i]=(Kh02CanonicalPart){association.n,unique[i]->json.n};
        if(!add(&association,unique[i]->json.p,unique[i]->json.n))goto failed;
    }
    if(!text(&association,"],\"version\":1}"))goto failed;
    total=sizeof(*out)+token_size+KH02_BIND_SIZE+association.n;
    out=VirtualAlloc(NULL,total,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!out)goto failed;
    out->size=total;out->set_count=count;offset=sizeof(*out);
    out->token=(Kh02CanonicalPart){offset,token_size};memcpy((unsigned char*)out+offset,token,token_size);offset+=token_size;
    out->container=(Kh02CanonicalPart){offset,KH02_BIND_SIZE};memcpy((unsigned char*)out+offset,bundle->container,KH02_BIND_SIZE);offset+=KH02_BIND_SIZE;
    out->association=(Kh02CanonicalPart){offset,association.n};memcpy((unsigned char*)out+offset,association.p,association.n);
    out->scope=(Kh02CanonicalPart){offset+scope_part.offset,scope_part.size};
    for(i=0;i<count;i++)out->sets[i]=(Kh02CanonicalPart){offset+set_parts[i].offset,set_parts[i].size};
    memcpy(out->records,indices,sizeof(indices));hash_input[0]=out->token;hash_input[1]=out->container;hash_input[2]=out->association;
    if(!hash_parts((unsigned char*)out,hash_input,out->binding) || !VirtualProtect(out,total,PAGE_READONLY,&old))goto failed;
    goto done;
failed:
    if(out){VirtualFree(out,0,MEM_RELEASE);out=NULL;}
done:
    for(i=0;i<64;i++)if(members[i].json.p)HeapFree(GetProcessHeap(),0,members[i].json.p);
    if(scope.p)HeapFree(GetProcessHeap(),0,scope.p);
    if(association.p)HeapFree(GetProcessHeap(),0,association.p);
    return out;
}
uint32_t Kh02CanonicalDestroy(const Kh02CanonicalAssociation **owner) {
    if(!owner)return 0;if(!*owner)return 1;
    if(!VirtualFree((void*)*owner,0,MEM_RELEASE))return 0;*owner=NULL;return 1;
}
