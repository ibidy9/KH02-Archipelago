/* Retain an authenticated native death across the immediate gameplay-owner gap.
   Always forwards all six arguments. No game writes, names, scans or save work. */
typedef struct DeathObservation250 {
    uint32_t magic,version,size;volatile LONG sequence;
    uint64_t serial,cookie,epoch,actor,remote,hp,state,result;
} DeathObservation250;
_Static_assert(sizeof(DeathObservation250)==80,"death observation ABI");
static DeathObservation250 *death_observation250;
static HANDLE death_mapping250;
static void *death_trampoline250;
static uint32_t death_remote250;
static const unsigned char death_prefix250[18]={64,83,87,65,86,72,131,236,96,72,139,1,72,139,250,72,139,217};
static int death_init250(void) {
    wchar_t name[96];
    _snwprintf_s(name,96,_TRUNCATE,L"Local\\KH02APDeathV250-%lu",GetCurrentProcessId());
    death_mapping250=CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeof(DeathObservation250),name);
    if(!death_mapping250 || GetLastError()==ERROR_ALREADY_EXISTS)return 0;
    death_observation250=MapViewOfFile(death_mapping250,FILE_MAP_ALL_ACCESS,0,0,sizeof(DeathObservation250));
    if(!death_observation250)return 0;
    death_observation250->magic=0x3035444b;death_observation250->version=1;
    death_observation250->size=sizeof(DeathObservation250);return 1;
}
static unsigned char __fastcall death_observe250(void *receiver,const void *damage,float amount,
        const void *direction,const void *direction2,void *attacker) {
    typedef unsigned char (__fastcall *Fn)(void*,const void*,float,const void*,const void*,void*);
    Kh02SessionSample s;TrapReply242 t={0};DeathObservation250 row={0};int admitted=0;
    unsigned char result;uint64_t vt=0;
    InterlockedIncrement(&flight);
    if(GetCurrentThreadId()==owner_thread && death_observation250 &&
       popup_sample_v111(&s,Kh02LifecycleEpoch(life)) &&
       (uintptr_t)receiver==s.provider+0x450 && trap_read242(&s,&t) && t.hp==0 &&
       t.state!=0x33 && t.state!=0x5e &&
       read_local(NULL,s.provider,&vt,8) && (vt==game.base+0x3735bc8 || vt==game.base+0x373d490)) {
        row.cookie=session.gate.current.cookie;row.epoch=Kh02LifecycleEpoch(life);
        row.actor=s.provider;row.remote=death_remote250;row.hp=t.hp;row.state=t.state;
        admitted=1;
    }
    result=((Fn)death_trampoline250)(receiver,damage,amount,direction,direction2,attacker);
    if(admitted && result && death_observation250->serial<UINT64_MAX) {
        InterlockedIncrement(&death_observation250->sequence);
        row.serial=death_observation250->serial+1;row.result=result;
        memcpy(&death_observation250->serial,&row.serial,sizeof(row)-16);
        MemoryBarrier();InterlockedIncrement(&death_observation250->sequence);
    }
    InterlockedDecrement(&flight);return result;
}
#ifdef KH02_GRANT_OWNER_SELFTEST
KH02_RECEIPT_API void Kh02DeathObserveTest250(void *fn) {death_trampoline250=fn;}
KH02_RECEIPT_API unsigned char Kh02DeathObserveCall250(void *a,const void *b,float c,const void *d,const void *e,void *f) {
    return death_observe250(a,b,c,d,e,f);
}
#endif
