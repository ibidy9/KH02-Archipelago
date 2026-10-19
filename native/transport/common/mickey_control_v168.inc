/* Scoped AP companion policy. Fixed name and actions, no arbitrary spawn input. */
typedef struct MickeyControl168 {
    uint32_t magic,version,size,request,response,command,status,result;
    uint64_t cookie,epoch,owner,manager,name;
    uint32_t before,after,mode,attempts;
    char text[128];
} MickeyControl168;
_Static_assert(sizeof(MickeyControl168)==216,"Mickey control ABI");
static MickeyControl168 *mickey_control168;
static HANDLE mickey_control_mapping168;
static uint64_t mickey_cookie168,mickey_name168;
static uint32_t mickey_mode168,mickey_attempts168;
static char mickey_text168[128];
static int mickey_fname168(uint64_t name,char out[128]) {
    uint64_t pool,chunk,entry;uint32_t index=(uint32_t)name;uint16_t head;
    unsigned char text[256];unsigned i;
    memset(out,0,128);
    /* Only unnumbered, bounded ASCII names are admitted by this experiment. */
    if(!index || index>=0x200000 || (name>>32) ||
       !read_local(NULL,game.base+0x4695040,&pool,8) || !mickey_ptr167(pool) ||
       !read_local(NULL,pool+(index>>14)*8,&chunk,8) || !mickey_ptr167(chunk) ||
       !read_local(NULL,chunk+(index&0x3fff)*8,&entry,8) || !mickey_ptr167(entry) ||
       !read_local(NULL,entry,&head,2) || !read_local(NULL,entry+0x10,text,256))return 0;
    for(i=0;i<127;i++) {
        uint16_t ch=(head&1)?(uint16_t)(text[i*2]|((uint16_t)text[i*2+1]<<8)):text[i];
        if(!ch)return i!=0;
        if(ch<32 || ch>126)return 0;
        out[i]=(char)ch;
    }
    memset(out,0,128);return 0;
}
static int mickey_control_init168(void) {
    wchar_t name[96];
    _snwprintf_s(name,96,_TRUNCATE,L"Local\\KH02APMickeyControlV168-%lu",GetCurrentProcessId());
    mickey_control_mapping168=CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeof(MickeyControl168),name);
    if(!mickey_control_mapping168 || GetLastError()==ERROR_ALREADY_EXISTS)return 0;
    mickey_control168=MapViewOfFile(mickey_control_mapping168,FILE_MAP_ALL_ACCESS,0,0,sizeof(MickeyControl168));
    if(!mickey_control168)return 0;
    mickey_control168->magic=0x38364b4d;mickey_control168->version=1;mickey_control168->size=sizeof(MickeyControl168);
    return 1;
}

extern uint32_t Kh02MickeyOwnership171(uint32_t*);
static uint64_t mickey_attempt_epoch171,mickey_attempt_player171,mickey_attempt_manager171;
static uint32_t mickey_failed171;
static uint64_t mickey_lab_manager221,mickey_lab_epoch221,mickey_lab_handle221;
static char mickey_lab_map225[32];
static int lab_map225(const char *map) {
    static const char *allowed[]={"/Game/Levels/dw/dw_08/dw_08","/Game/Levels/dw/dw_02/dw_02",
        "/Game/Levels/dw/dw_03/dw_03","/Game/Levels/dw/dw_06/dw_06",
        "/Game/Levels/dw/dw_01/dw_01","/Game/Levels/dw/dw_07/dw_07",
        "/Game/Levels/dw/dw_04/dw_04","/Game/Levels/dw/dw_05/dw_05",
        "/Game/Levels/dw/dw_11/dw_11","/Game/Levels/dw/dw_12/dw_12"};
    for(unsigned i=0;i<sizeof(allowed)/sizeof(allowed[0]);i++)if(!memcmp(map,allowed[i],26))return 1;
    return 0;
}

static int mickey_lab_warped221;
/* Called only by the authenticated startup command on the shared owner thread.
 * Mode is immutable for this process/seed. No pawn or save mutation here. */
static uint32_t journey_enabled229,journey_attempts229;
static uint64_t journey_deadline229;
static uint32_t mickey_configured172;
uint32_t Kh02MickeyConfigure172(uint64_t cookie,uint32_t mode) {
    if(!cookie || mode>2)return 2;
    if(mickey_configured172)return mickey_cookie168==cookie && mickey_mode168==mode?1:2;
    if(mickey_mode168 || mickey_attempts168)return 2;
    mickey_cookie168=cookie;mickey_mode168=mode;mickey_configured172=1;if(mode==2)journey_enabled229=1;return 1;
}
static int mickey_block168(MickeyRow167 *r) {
    uint32_t owned=0;
    if(r->reason!=5 || (mickey_mode168!=1 && mickey_mode168!=2) || mickey_cookie168!=session.gate.current.cookie ||
       r->slot || memcmp(r->map,"/Game/Levels/dw/dw_09/dw_09",26) ||
       !mickey_fname168(r->klass,r->class_name) || strcmp(r->class_name,"n_dw003"))return 0;
    mickey_name168=r->klass;
    memcpy(mickey_text168,r->class_name,128);
    /* An authenticated randomized configuration locks exactly Mickey until history owns him. */
    if(Kh02MickeyOwnership171(&owned) && owned)return 0;
    r->reason=6;return 1;
}
static void catchup224(void);
static void journey_reset229(void);
static uint64_t now224(void);
static uint32_t auto_enabled224,auto_count224,auto_reason224;
static uint64_t auto_deadline224;
static void auto_reset224(void);
static void mickey_process168(MickeyControl168 *b) {
    Kh02SessionSample sample;MickeyRow167 r={0};uint32_t seq,cmd,owned=0;uint64_t cookie,epoch,name;
    unsigned char result;char current_name[128];
    if(!b || GetCurrentThreadId()!=owner_thread)return;
    seq=b->request;if(!seq || seq==b->response)return;
    MemoryBarrier();cmd=b->command;cookie=b->cookie;epoch=b->epoch;b->status=2;b->result=0;
    if(b->magic!=0x38364b4d || b->version!=1 || b->size!=sizeof(*b) || cmd<1 || cmd>8 ||
       cookie!=session.gate.current.cookie || epoch!=Kh02LifecycleEpoch(life) ||
       !Kh02ControlledStartAuthorized() || !is_active(NULL) || !session.context ||
       !session.gate.current.observation || InterlockedCompareExchange(&busy,1,0))goto reply;
    b->result=10;
    if(Kh02OwnerReadSample(read_local,NULL,game.base,game.cache,session.gate.current.generation,&sample)!=KH02_GATE_OK ||
       sample.context!=session.context || sample.world!=session.world ||
       sample.provider!=session.gate.current.provider || sample.attributes!=session.gate.current.attributes ||
       !read_local(NULL,sample.provider+0x9e8,&r.manager,8))goto release;
    r.thread=GetCurrentThreadId();mickey_before167(&r);
    b->owner=r.owner;b->manager=r.manager;b->before=r.before;b->after=r.before;
    if(r.reason!=5 || r.player!=sample.provider || r.owner!=sample.context)goto release;
    if(cmd==1) {
        /* Candidate configure before entering Depths. Reconfiguration cannot revoke ownership. */
        b->result=11;
        if(mickey_mode168 || memcmp(r.map,"/Game/Levels/dw/dw_08/dw_08",26) || r.before)goto release;
        mickey_cookie168=cookie;mickey_mode168=1;b->status=1;goto release;
    }
    b->result=12;
    if((mickey_mode168!=1 && mickey_mode168!=2) || cookie!=mickey_cookie168 || !Kh02MickeyOwnership171(&owned))goto release;
    if(cmd>=5) {
        if(cmd==8) {
            if(!owned || !lab_map225(r.map) || journey_enabled229)goto release;
            journey_reset229();journey_attempts229=0;journey_enabled229=1;
            journey_deadline229=now224()+1800000;b->status=1;goto release;
        }
        if(cmd==6) {journey_enabled229=0;auto_enabled224=0;auto_reset224();b->status=1;goto release;}
        if(cmd==7) {b->result=auto_reason224;b->after=auto_count224;b->status=1;goto release;}
        if(!owned || r.before!=1 || r.manager!=mickey_lab_manager221 || epoch!=mickey_lab_epoch221 ||
           (!lab_map225(r.map) || memcmp(r.map,mickey_lab_map225,32)))goto release;
        auto_reset224();auto_count224=0;auto_reason224=0;auto_enabled224=1;
        auto_deadline224=now224()+600000;b->status=1;goto release;
    }
    if(cmd==4) {
        static const unsigned char expected[]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,0x24,0x10,0x57,0x48,0x83,0xec,0x20,0x0f,0xb6,0xf2,0x48,0x8b,0xd9,0x80,0xf2,0x01,0xe8,0x53,0x32,0x00,0x00,0x44,0x8b,0x0d,0xa8,0x30,0xfd,0x03,0x48,0x81,0xc3,0x18,0x02,0x00,0x00,0x4c,0x8b,0x15,0x8e,0x30,0xfd,0x03,0xbf,0x04,0x00,0x00,0x00,0x44,0x8b,0x43,0x04,0x45,0x85,0xc0,0x74,0x69,0x48,0x63,0x13,0x85,0xd2,0x78,0x62,0x41,0x3b,0xd1,0x7d,0x5d,0x48,0x8d,0x0c,0x52,0x49,0x8d,0x04,0xca,0x48,0x85,0xc0,0x74,0x50,0x44,0x39,0x40,0x10,0x75,0x4a,0x41,0x3b,0xd1,0x7d,0x15,0x49,0x8d,0x04,0xca,0x48,0x85,0xc0,0x74,0x0e,0xf7,0x40,0x08,0x00,0x00,0x00,0x30,0x0f,0x94,0xc2,0xeb,0x04,0x33,0xc0,0x32,0xd2,0x33,0xc9,0x84,0xd2,0x48,0x0f,0x45,0xc8,0x48,0x85,0xc9,0x74,0x1f,0x48,0x8b,0x09,0x48,0x85,0xc9,0x74,0x17,0x40,0x0f,0xb6,0xd6,0xe8,0x75,0xfe,0xff,0xff,0x44,0x8b,0x0d,0x2a,0x30,0xfd,0x03,0x4c,0x8b,0x15,0x17,0x30,0xfd,0x03,0x48,0x83,0xc3,0x08,0x48,0x83,0xef,0x01,0x75,0x84,0x48,0x8b,0x5c,0x24,0x30,0x48,0x8b,0x74,0x24,0x38,0x48,0x83,0xc4,0x20,0x5f,0xc3};
        unsigned char actual[sizeof(expected)];uint64_t handle=0;uint32_t hp=0,flags=0;
        b->result=40;
        if(!owned || r.before!=1 || memcmp(r.map,"/Game/Levels/dw/dw_08/dw_08",26) ||
           mickey_lab_warped221 || r.manager!=mickey_lab_manager221 || epoch!=mickey_lab_epoch221 ||
           !read_local(NULL,r.manager+0x218,&handle,8) || handle!=mickey_lab_handle221 ||
           !(handle>>32) || (int32_t)handle<0 ||
           !read_local(NULL,r.player+0x5dc,&hp,4) || !hp ||
           !read_local(NULL,r.owner+0x348,&flags,4) || (flags&2) ||
           !read_local(NULL,game.base+0x4c5b10,actual,sizeof(actual)) || memcmp(actual,expected,sizeof(actual)))goto release;
        if(b->request!=seq || b->command!=cmd || b->cookie!=cookie || b->epoch!=epoch)goto release;
        mickey_lab_warped221=1;MemoryBarrier();
        ((void (__fastcall *)(void*,unsigned char))(game.base+0x4c5b10))((void*)(uintptr_t)r.manager,0);
        b->result=41;b->status=1;
        if(!read_local(NULL,r.manager+0x238,&b->after,4) || b->after!=1)b->status=3;
        goto release;
    }
    if(cmd==3) {
        uint32_t flags=0,hp=0;uint64_t handle=0;
        if(!owned || !lab_map225(r.map) || sample.phase!=3 || sample.title_flags ||
           sample.pending_map || (sample.travel_flags&1) || sample.world_flag ||
           !read_local(NULL,r.owner+0x348,&flags,4) || (flags&2) ||
           !read_local(NULL,r.player+0x5dc,&hp,4) || !hp)goto release;
        /* Rebind only our exact already-created companion; never adopt an
           unrelated native actor or create a duplicate. Operator-driven. */
        if(r.before==1) {
            if(r.manager!=mickey_lab_manager221 || !read_local(NULL,r.manager+0x218,&handle,8) ||
               handle!=mickey_lab_handle221 || !(handle>>32))goto release;
            mickey_lab_epoch221=epoch;memcpy(mickey_lab_map225,r.map,32);
            auto_enabled224=0;auto_reset224();b->status=1;b->result=32;goto release;
        }
        if(r.before==0 && memcmp(r.map,mickey_lab_map225,32) && !mickey_failed171) {
            mickey_attempt_epoch171=0;mickey_attempt_player171=0;mickey_attempt_manager171=0;
        }
    }
    b->result=30;
    if(!owned || (cmd==3?!lab_map225(r.map):memcmp(r.map,"/Game/Levels/dw/dw_09/dw_09",26)) || r.before) {b->status=1;goto release;}
    if(mickey_failed171 || (mickey_attempt_epoch171==epoch && mickey_attempt_player171==r.player && mickey_attempt_manager171==r.manager)) {
        b->result=31;goto release;
    }
    /* Fixed find-only name; never reuse another process's FName index. */
    name=start_name(L"n_dw003",0);
    if(!name || !mickey_fname168(name,current_name) || strcmp(current_name,"n_dw003") ||
       !entries[19].owned || memcmp(entries[19].target,entries[19].patch,entries[19].size))goto release;
    if(b->request!=seq || b->command!=cmd || b->cookie!=cookie || b->epoch!=epoch)goto release;
    mickey_attempt_epoch171=epoch;mickey_attempt_player171=r.player;mickey_attempt_manager171=r.manager;
    mickey_failed171=1;mickey_attempts168++;mickey_name168=name;memcpy(mickey_text168,current_name,128);
    b->result=20;MemoryBarrier();
    result=((unsigned char (__fastcall *)(void*,int,uint64_t))mickey_trampoline167)((void*)(uintptr_t)r.manager,0,name);
    b->result=result;
    if(!read_local(NULL,r.manager+0x238,&b->after,4) || b->after!=1 || result!=1)goto release;
    mickey_failed171=0;b->status=1;
    if(cmd==3) {
        mickey_lab_manager221=r.manager;mickey_lab_epoch221=epoch;mickey_lab_warped221=0;
        memcpy(mickey_lab_map225,r.map,32);auto_enabled224=0;auto_reset224();
        if(!read_local(NULL,r.manager+0x218,&mickey_lab_handle221,8))mickey_lab_manager221=0;
    }
release:
    InterlockedExchange(&busy,0);
reply:
    b->name=mickey_name168;b->mode=mickey_mode168;b->attempts=mickey_attempts168;
    memcpy(b->text,mickey_text168,128);MemoryBarrier();b->response=seq;
}
#include "mickey_catchup_v224.inc"
#include "mickey_journey_v229.inc"
static void MickeyService168(void) {mickey_process168(mickey_control168);journey229();catchup224();}
#ifdef KH02_GRANT_OWNER_SELFTEST
KH02_RECEIPT_API void Kh02MickeyProcess168(MickeyControl168 *b){mickey_process168(b);}
#endif
