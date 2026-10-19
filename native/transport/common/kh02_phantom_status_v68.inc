/* Read-only publication of the admitted native frame; no write commands. */
typedef struct PhantomStatusV68 {
    uint32_t magic,version,size;volatile LONG sequence;
    uint64_t owner,epoch,cache,attributes;
    uint32_t stats[6];char map[64];uint64_t tick;
} PhantomStatusV68;
_Static_assert(sizeof(PhantomStatusV68)==144,"phantom status ABI");
static void Kh02PhantomPublish(void) {
    Kh02SessionSample s;PhantomStatusV68 v={0},*b;HANDLE h;wchar_t name[96];
    /* Observation is independent of the combat-boost map allowlist. Retain
     * all owner/lifecycle guards; this path only publishes current values. */
    if(GetCurrentThreadId()!=owner_thread || !is_active(NULL) || !Kh02ControlledStartAuthorized() ||
       Kh02OwnerReadSample(read_local,NULL,game.base,game.cache,session.gate.current.generation,&s)!=KH02_GATE_OK ||
       s.phase!=3 || s.title_flags || s.pending_map || (s.travel_flags&1) || s.world_flag ||
       s.context!=session.context || s.world!=session.world || s.attributes!=session.gate.current.attributes)return;
    if(!read_local(NULL,s.attributes+0x50,v.stats,24) || !read_local(NULL,s.context+0x384c,v.map,64))return;
    v.owner=s.context;v.epoch=Kh02LifecycleEpoch(life);v.cache=game.cache;v.attributes=s.attributes;v.tick=GetTickCount64();
    _snwprintf_s(name,96,_TRUNCATE,L"Local\\KH02APPhantomStatusV68-%lu",GetCurrentProcessId());
    h=OpenFileMappingW(FILE_MAP_ALL_ACCESS,FALSE,name);if(!h)return;
    b=MapViewOfFile(h,FILE_MAP_ALL_ACCESS,0,0,sizeof(*b));
    if(b && b->magic==0x3836504b && b->version==1 && b->size==sizeof(*b)) {
        InterlockedIncrement(&b->sequence);MemoryBarrier();
        memcpy((unsigned char*)b+16,(unsigned char*)&v+16,sizeof(v)-16);
        MemoryBarrier();InterlockedIncrement(&b->sequence);
    }
    if(b)UnmapViewOfFile(b);CloseHandle(h);
}
