/* Additive offline composition. The caller must authenticate the scope, prepare
   the service, arm fresh admission and retain all borrowed owners beforehand.
   This file does not install hooks or accept client messages. */
/* Included after the fresh replay session/router composition. */

typedef struct Kh02FreshSaveEntry {
    Kh02SaveRouter *router;
    unsigned char *baseline;
    char prefix[65];uint32_t length;
    DWORD thread;volatile LONG flight,spent,closed;
} Kh02FreshSaveEntry;

MEMBER_API Kh02FreshSaveEntry *Kh02FreshSaveEntryPrepare(Kh02SaveRouter *router,
    const unsigned char *raw,uint32_t size,const char *prefix,uint32_t length) {
    Kh02FreshSaveEntry *p;Kh02ReceiptBindings *validation;Kh02ReceiptScope scope;DWORD old;
    if(!router || GetCurrentThreadId()!=router->thread || !router->life->fresh ||
       router->life->epoch || router_active(router) || router->count ||
       InterlockedCompareExchange(&router->closed,0,0) || !raw || size!=KH02_BIND_SIZE ||
       !prefix || !length || length>64)return NULL;
    for(uint32_t i=0;i<length;++i)
        if(!((prefix[i]>='a'&&prefix[i]<='z') || (prefix[i]>='A'&&prefix[i]<='Z') ||
             (prefix[i]>='0'&&prefix[i]<='9') || prefix[i]=='-'))return NULL;
    /* This path is only for a new empty container. Occupied records require
       the existing exact disk-association path, never inferred membership. */
    for(uint32_t i=0xb0;i<size;++i)if(raw[i])return NULL;
    scope=router->life->branch->scope;scope.load_id=1;
    validation=Kh02ReceiptBindingsCreate(&scope,raw,size);
    if(!validation)return NULL;
    if(Kh02ReceiptBindingsDestroy(&validation)!=KH02_BIND_OK)return NULL;
    p=VirtualAlloc(NULL,sizeof(*p),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!p)return NULL;
    p->baseline=VirtualAlloc(NULL,size,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(!p->baseline){VirtualFree(p,0,MEM_RELEASE);return NULL;}
    memcpy(p->baseline,raw,size);
    if(!VirtualProtect(p->baseline,size,PAGE_READONLY,&old)) {
        VirtualFree(p->baseline,0,MEM_RELEASE);VirtualFree(p,0,MEM_RELEASE);return NULL;
    }
    p->router=router;p->thread=GetCurrentThreadId();p->length=length;memcpy(p->prefix,prefix,length);
    return p;
}

MEMBER_API uintptr_t Kh02FreshSaveEntryCopy(Kh02FreshSaveEntry *p,uintptr_t caller,
    uintptr_t destination,uintptr_t source) {
    uintptr_t result;Kh02SaveRouter *router;uint32_t relevant,eligible,alone=0;Kh02CallbackPin pin={0};char name[65];int length;
    if(!p)return 0;
    InterlockedIncrement(&p->flight);router=p->router;
    relevant=caller==router->config.base+0x56e280 || caller==router->config.base+0x56e277;
    eligible=relevant && GetCurrentThreadId()==p->thread &&
        !InterlockedCompareExchange(&p->closed,0,0) &&
        InterlockedCompareExchange(&p->spent,1,0)==0;
    if(relevant && !eligible)Kh02SaveRouterStop(router);
    if(!relevant) {
        result=Kh02LifecycleFreshCopy(router->life,caller,destination,source);
        InterlockedDecrement(&p->flight);return result;
    }
    if(!Kh02CallbackTransition(&router->domain,&pin,&alone)){InterlockedDecrement(&p->flight);return 0;}
    if(!alone || pin.selected){eligible=0;Kh02SaveRouterStop(router);}
    result=Kh02LifecycleFreshCopy(router->life,caller,destination,source);
    Kh02CallbackLeave(&pin);
    length=snprintf(name,sizeof(name),"%s-%llu",p->prefix,(unsigned long long)Kh02LifecycleEpoch(router->life));
    /* FreshCopy has left its lifecycle guard; attach before returning to the
       caller which may immediately capture and autosave. No polling window. */
    if(relevant && (!eligible || !Kh02LifecycleCurrent(router->life) ||
       length<=0 || length>64 || Kh02SaveRouterAttach(router,p->baseline,KH02_BIND_SIZE,name,(uint32_t)length)!=KH02_BIND_OK)) {
        InterlockedExchange(&p->closed,1);Kh02SaveRouterStop(router);
    }
    InterlockedDecrement(&p->flight);return result;
}

/* Stop routing externally and join callbacks before destroy. Borrowed router,
   lifecycle and service must outlive this object. No hook uninstall occurs. */
MEMBER_API uint32_t Kh02FreshSaveEntryDestroy(Kh02FreshSaveEntry **handle) {
    Kh02FreshSaveEntry *p;if(!handle)return KH02_BIND_REFUSED;p=*handle;if(!p)return KH02_BIND_OK;
    if(GetCurrentThreadId()!=p->thread)return KH02_BIND_REFUSED;
    InterlockedExchange(&p->closed,1);Kh02SaveRouterStop(p->router);
    if(InterlockedCompareExchange(&p->flight,0,0))return KH02_BIND_BUSY;
    if(p->baseline && !VirtualFree(p->baseline,0,MEM_RELEASE))return KH02_BIND_REFUSED;
    p->baseline=NULL;
    if(!VirtualFree(p,0,MEM_RELEASE))return KH02_BIND_REFUSED;
    *handle=NULL;return KH02_BIND_OK;
}
