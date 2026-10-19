/* Read-only, fixed-capacity snapshots through the existing admitted stat reader.
 * No hook, game write, second owner or scan is added. Mapping failure is inert. */
typedef struct ResourceRow210 {
    uint64_t tick,caller,map,spawn;
    uint32_t kind,status,hp_items,mp_items;
    Kh02StatView before,after;
    char source_map[64];
} ResourceRow210;
typedef struct ResourceTrace210 {
    uint32_t magic,version,size,capacity;
    volatile LONG sequence,count,dropped,reserved;
    ResourceRow210 rows[256];
} ResourceTrace210;
static ResourceTrace210 *resource_trace210;
static HANDLE resource_mapping210;
static ResourceRow210 resource_last210;
static uint32_t resource_seen210,resource_attempted210;
static uint32_t resource_refusal275;
static void resource_record210(ResourceRow210 *row) {
    if(GetCurrentThreadId()!=owner_thread)return;
    if(!resource_trace210 && !resource_attempted210) {
        wchar_t name[96];resource_attempted210=1;
        _snwprintf_s(name,96,_TRUNCATE,L"Local\\KH02APResource210-%lu",GetCurrentProcessId());
        resource_mapping210=CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeof(ResourceTrace210),name);
        if(!resource_mapping210 || GetLastError()==ERROR_ALREADY_EXISTS)return;
        resource_trace210=MapViewOfFile(resource_mapping210,FILE_MAP_ALL_ACCESS,0,0,sizeof(ResourceTrace210));
        if(!resource_trace210)return;
    if(row->kind==5){if(resource_refusal275)return;resource_refusal275=1;}
    else resource_refusal275=0;
        resource_trace210->magic=0x30313252;resource_trace210->version=1;
        resource_trace210->size=sizeof(ResourceTrace210);resource_trace210->capacity=256;
    }
    if(!resource_trace210)return;
    if(row->kind==5){if(resource_refusal275)return;resource_refusal275=1;}
    else resource_refusal275=0;
    if(row->kind==1 && resource_seen210 &&
       !memcmp(&row->before,&resource_last210.before,sizeof(row->before)) &&
       !memcmp(&row->after,&resource_last210.after,sizeof(row->after)) &&
       !memcmp(row->source_map,resource_last210.source_map,64) &&
       row->status==resource_last210.status && row->hp_items==resource_last210.hp_items &&
       row->mp_items==resource_last210.mp_items)return;
    row->tick=GetTickCount64();
    InterlockedIncrement(&resource_trace210->sequence);
    if(resource_trace210->count<256)resource_trace210->rows[resource_trace210->count++]=*row;
    else resource_trace210->dropped++;
    MemoryBarrier();InterlockedIncrement(&resource_trace210->sequence);
    if(row->kind==1){resource_last210=*row;resource_seen210=1;}
}
static void resource_projection210(const Kh02StatProjection *p,uint32_t status,uint32_t hp,uint32_t mp) {
    if(!p->before.owner)return;
    ResourceRow210 row={0};row.kind=1;row.status=status;row.hp_items=hp;row.mp_items=mp;
    row.before=p->before;row.after=p->after;
    if(!read_local(NULL,row.before.owner+0x384c,row.source_map,64))memset(row.source_map,0,64);
    resource_record210(&row);
}
static void resource_travel210(uint64_t context,uint64_t map,uint64_t spawn,void *caller) {
    if(!resource_seen210 || GetCurrentThreadId()!=owner_thread || !active_flag || fault ||
       !life || context!=resource_last210.before.owner ||
       Kh02LifecycleEpoch(life)!=resource_last210.before.epoch)return;
    ResourceRow210 row=resource_last210;
    row.kind=2;row.caller=(uintptr_t)caller;row.map=map;row.spawn=spawn;
    /* Deliberately label as last admitted snapshot, not a fresh transition read. */
    resource_record210(&row);
}
