#include "kh02_production_policy_candidate.h"
#include <stddef.h>
#include <string.h>
#include <windows.h>
#include <bcrypt.h>

_Static_assert(sizeof(Kh02PolicyWire)==104,"policy wire ABI");

static uint64_t revision(const Kh02PolicyWire *wire) {
    return ((uint64_t)wire->revision_high<<32)|wire->revision_low;
}

uint32_t Kh02PolicyScopeBinding(uint32_t team,uint32_t slot,const unsigned char *seed,
        uint32_t seed_size,unsigned char *out) {
    static const unsigned char tag[12]={'K','H','0','2','P','O','L','I','C','Y',1,0};
    BCRYPT_ALG_HANDLE alg=NULL;BCRYPT_HASH_HANDLE hash=NULL;DWORD object_size=0,got=0;
    unsigned char *object=NULL;uint32_t fields[3]={team,slot,seed_size};NTSTATUS status;
    if(!slot || !seed || !seed_size || seed_size>4096 || !out)return KH02_POLICY_REFUSED;
    status=BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,NULL,0);
    if(status<0)goto done;
    status=BCryptGetProperty(alg,BCRYPT_OBJECT_LENGTH,(PUCHAR)&object_size,sizeof(object_size),&got,0);
    if(status<0 || got!=sizeof(object_size) || !object_size)goto done;
    object=HeapAlloc(GetProcessHeap(),0,object_size);if(!object)goto done;
    status=BCryptCreateHash(alg,&hash,object,object_size,NULL,0,0);
    if(status<0 || BCryptHashData(hash,(PUCHAR)tag,sizeof(tag),0)<0 ||
       BCryptHashData(hash,(PUCHAR)fields,sizeof(fields),0)<0 ||
       BCryptHashData(hash,(PUCHAR)seed,seed_size,0)<0 ||
       BCryptFinishHash(hash,out,32,0)<0)goto done;
    if(hash)BCryptDestroyHash(hash);if(object)HeapFree(GetProcessHeap(),0,object);
    if(alg)BCryptCloseAlgorithmProvider(alg,0);return KH02_POLICY_OK;
done:
    if(hash)BCryptDestroyHash(hash);if(object)HeapFree(GetProcessHeap(),0,object);
    if(alg)BCryptCloseAlgorithmProvider(alg,0);return KH02_POLICY_REFUSED;
}

uint32_t Kh02PolicyProcessChallenge(uint64_t *out) {
    uint64_t value=0;if(!out)return KH02_POLICY_REFUSED;
    if(BCryptGenRandom(NULL,(PUCHAR)&value,sizeof(value),BCRYPT_USE_SYSTEM_PREFERRED_RNG)<0 || !value)
        return KH02_POLICY_REFUSED;
    *out=value;return KH02_POLICY_OK;
}

uint32_t Kh02PolicyInit(Kh02PolicyState *state,uint64_t owner_cookie,
        uint64_t process_cookie,const unsigned char *scope_binding) {
    if(!state || !owner_cookie || !process_cookie || !scope_binding)return KH02_POLICY_REFUSED;
    memset(state,0,sizeof(*state));state->owner_cookie=owner_cookie;
    state->process_cookie=process_cookie;memcpy(state->scope_binding,scope_binding,32);
    return KH02_POLICY_OK;
}

uint32_t Kh02PolicyInstall(Kh02PolicyState *state,const Kh02PolicyWire *wire,
        uint64_t live_authority) {
    uint64_t next;
    if(!state || !wire || state->faulted || state->busy)return KH02_POLICY_BUSY;
    next=revision(wire);
    if(wire->magic!=KH02_POLICY_MAGIC || wire->version!=KH02_POLICY_VERSION ||
       wire->size!=sizeof(*wire) || (wire->item_schema_version!=KH02_ITEM_SCHEMA_VERSION && wire->item_schema_version!=4) ||
       wire->owner_cookie!=state->owner_cookie || wire->process_cookie!=state->process_cookie ||
       !wire->authority || wire->authority!=live_authority || !next ||
       next<=state->revision || (state->revision==UINT64_MAX) ||
       memcmp(wire->scope_binding,state->scope_binding,32) || wire->reserved ||
       wire->hp_items>4096 || wire->mp_items>4096 ||
       (wire->ability_mask&~KH02_ABILITY_MASK) || (wire->spell_mask&~KH02_SPELL_MASK) ||
       (wire->world_mask&~KH02_WORLD_MASK))return KH02_POLICY_REFUSED;
    state->busy=1;
    state->authority=wire->authority;state->revision=next;
    state->hp_items=wire->hp_items;state->mp_items=wire->mp_items;
    state->ability_mask=wire->ability_mask;state->spell_mask=wire->spell_mask;
    state->world_mask=wire->world_mask;state->installed=1;state->generation=0;
    state->busy=0;return KH02_POLICY_OK;
}

uint32_t Kh02PolicyProject(Kh02PolicyState *state,uint64_t authority,
        uint64_t generation,Kh02PolicyProjection *out) {
    if(!state || !out || state->faulted || state->busy || !state->installed ||
       !authority || authority!=state->authority || !generation)return KH02_POLICY_REFUSED;
    state->busy=1;*out=(Kh02PolicyProjection){state->revision,generation,
        state->hp_items,state->mp_items,state->ability_mask,state->spell_mask,
        state->world_mask,0};
    return KH02_POLICY_OK;
}

uint32_t Kh02PolicyFinish(Kh02PolicyState *state,uint64_t generation,uint32_t verified) {
    if(!state || !state->busy)return KH02_POLICY_REFUSED;
    if(!verified || !generation){state->faulted=1;state->busy=0;return KH02_POLICY_REFUSED;}
    state->generation=generation;state->busy=0;return KH02_POLICY_OK;
}

uint32_t Kh02PolicyRevoke(Kh02PolicyState *state,uint64_t authority) {
    if(!state || state->busy || !authority || authority!=state->authority)return KH02_POLICY_BUSY;
    state->authority=0;return KH02_POLICY_OK;
}

uint32_t Kh02PolicyTransaction(Kh02PolicyState *state,const Kh02PolicyWire *wire,
        uint64_t live_authority,uint64_t generation,const unsigned char *membership,
        uint32_t membership_size,Kh02PolicyApply apply,Kh02PolicyCommit commit,void *context) {
    Kh02PolicyState staged;Kh02PolicyProjection projection;uint32_t status;
    if(!state || !wire || !membership || membership_size<56 || membership_size>102456 ||
       !apply || !commit || state->busy || state->faulted)return KH02_POLICY_REFUSED;
    staged=*state;
    status=Kh02PolicyInstall(&staged,wire,live_authority);
    if(status!=KH02_POLICY_OK)return status;
    status=Kh02PolicyProject(&staged,live_authority,generation,&projection);
    if(status!=KH02_POLICY_OK)return status;
    /* A verified projection can have changed game state. Every later failure is
       uncertain and permanently closes this owner; it is never retried. */
    if(apply(context,&projection)!=KH02_POLICY_OK) {
        state->faulted=1;return KH02_POLICY_UNCERTAIN;
    }
    if(commit(context,membership,membership_size)!=KH02_POLICY_OK) {
        state->faulted=1;return KH02_POLICY_UNCERTAIN;
    }
    if(Kh02PolicyFinish(&staged,generation,1)!=KH02_POLICY_OK) {
        state->faulted=1;return KH02_POLICY_UNCERTAIN;
    }
    *state=staged;return KH02_POLICY_OK;
}
