/* Display-only diagnostic. Fixed strings, one explicit request per case. */
#include "kh02_custom_popup_args_v115.h"
#include "kh02_custom_popup_guards_v115.h"
static struct {uint64_t epoch,cookie,id;uint32_t pending,status,submitted;} custom_popup_v115;
static uint16_t custom_icon_v115[261],custom_pending_text_v115[161],custom_active_text_v115[161];
static uint32_t custom_pending_length_v115,custom_pending_ap_icon_v241;
static HANDLE custom_mapping_v115;
static uint64_t *custom_diag_v115;
static void custom_report_v115(void) {
    if(!custom_diag_v115 && !custom_mapping_v115) {
        wchar_t name[96];_snwprintf_s(name,96,_TRUNCATE,L"Local\\KH02APCustomPopupV115-%lu",GetCurrentProcessId());
        custom_mapping_v115=CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,608,name);
        if(custom_mapping_v115 && GetLastError()!=ERROR_ALREADY_EXISTS)
            custom_diag_v115=MapViewOfFile(custom_mapping_v115,FILE_MAP_ALL_ACCESS,0,0,608);
    }
    if(!custom_diag_v115)return;
    InterlockedIncrement64((LONG64*)&custom_diag_v115[1]);
    custom_diag_v115[0]=0x323131504343;custom_diag_v115[2]=custom_popup_v115.epoch;
    custom_diag_v115[3]=custom_popup_v115.id;custom_diag_v115[4]=custom_popup_v115.pending;
    custom_diag_v115[5]=custom_popup_v115.status;custom_diag_v115[6]=custom_popup_v115.submitted;
    if(custom_popup_v115.id) {
        const uint16_t *icon=custom_icon_v115;
        uint32_t n=0;while(n<260 && icon[n])n++;
        custom_diag_v115[7]=n;
        memcpy((unsigned char*)custom_diag_v115+64,icon,522);
    }
    MemoryBarrier();InterlockedIncrement64((LONG64*)&custom_diag_v115[1]);
}
uint32_t Kh02CustomPopupRequestV115(const void *raw,uint32_t size) {
    Kh02SessionSample s;Kh02NoticeV115 notice;Kh02PopupArgsV115 checked;
    static const uint16_t placeholder[]={'/',0};
    if(size!=sizeof(notice) || !raw || GetCurrentThreadId()!=owner_thread || custom_popup_v115.pending)return 2;
    memcpy(&notice,raw,sizeof(notice));
    if((notice.magic!=0x3531314e5041ULL && notice.magic!=0x3134324e5041ULL) || !notice.id || notice.id==custom_popup_v115.id ||
       notice.reserved || notice.padding[0] || notice.padding[1] || notice.padding[2] ||
       notice.epoch!=Kh02LifecycleEpoch(life) ||
       !Kh02CustomPopupArgsV115(&checked,notice.text,notice.length,placeholder,1) ||
       !popup_sample_v111(&s,notice.epoch))return 2;
    custom_popup_v115.epoch=notice.epoch;custom_popup_v115.cookie=session.gate.current.cookie;
    custom_popup_v115.id=notice.id;custom_pending_length_v115=notice.length;
    custom_pending_ap_icon_v241=notice.magic==0x3134324e5041ULL;
    memcpy(custom_pending_text_v115,notice.text,sizeof(notice.text));
    custom_popup_v115.pending=1;custom_popup_v115.status=1;custom_report_v115();return 1;
}
static void Kh02CustomPopupServiceV115(void) {
    typedef void *(*Getter)(void*);
    typedef void *(*Invoke)(void*,void*,const void*,const void*);
    typedef void (*Activate)(void*);
    typedef void (*Free)(void*);
    Kh02SessionSample s;uint64_t vt,fn,controller,ui,manager,widget,movie,tail,node;
    uint32_t movie_type;unsigned char active;Kh02PopupArgsV115 args;Kh02PopupValueV115 result={0};
    struct {void *data;int32_t count,capacity;} list={args.values,3,3};
    static const wchar_t method[]=L"setParam";
    const Kh02PopupStringV115 method_arg={(const uint16_t*)method,9,9};
    if(GetCurrentThreadId()!=owner_thread || !custom_popup_v115.pending)return;
    if(custom_popup_v115.epoch!=Kh02LifecycleEpoch(life) || custom_popup_v115.cookie!=session.gate.current.cookie) {
        custom_popup_v115.pending=0;custom_popup_v115.status=20;custom_report_v115();return;
    }
    custom_popup_v115.status=2;
    if(!popup_sample_v111(&s,custom_popup_v115.epoch) ||
       !popup_ready273(read_local,NULL,game.base,s.context) || InterlockedCompareExchange(&busy,1,0))return;
    if(!popup_vtable_v111(s.provider,&vt) || !read_local(NULL,vt+0x928,&fn,8) || fn!=game.base+0x31e730)goto done;
    /* The full module's exact-build guards are supplemented at each native
       entry used here; generated prefixes are shared with the offline audit. */
    unsigned char observed[24];
    for(uint32_t i=0;i<sizeof(custom_guards_v115)/sizeof(custom_guards_v115[0]);i++) {
        if(!read_local(NULL,game.base+custom_guards_v115[i].rva,observed,24) ||
           memcmp(observed,custom_guards_v115[i].bytes,24))goto done;
    }
    custom_popup_v115.status=3;
    controller=(uintptr_t)((Getter)(game.base+0x31e730))((void*)(uintptr_t)s.provider);
    if(!popup_pointer_v111(controller) || !popup_same_v111(&s,custom_popup_v115.epoch))goto done;
    ui=(uintptr_t)((Getter)(game.base+0x4f7e50))((void*)(uintptr_t)controller);
    if(!popup_pointer_v111(ui) || !popup_same_v111(&s,custom_popup_v115.epoch))goto done;
    custom_popup_v115.status=4;
    if(!read_local(NULL,ui+0x88,&manager,8) || !popup_vtable_v111(manager,&vt) ||
       !read_local(NULL,manager+0x1a0,&widget,8) || !popup_vtable_v111(widget,&vt) || vt!=game.base+0x3780450 ||
       !read_local(NULL,vt+0x268,&fn,8) || fn!=game.base+0x58c770 ||
       !read_local(NULL,vt+0x278,&fn,8) || fn!=game.base+0x587970)goto done;
    custom_popup_v115.status=5;
    if(!read_local(NULL,widget+0x68,&active,1) || active ||
       !read_local(NULL,widget+0x98,&tail,8) || !popup_pointer_v111(tail) ||
       !read_local(NULL,tail,&node,8) || node)goto done; /* Never jump the native queue. */
    if(!read_local(NULL,widget+0x40,&movie,8) || !popup_pointer_v111(movie) ||
       !read_local(NULL,movie+0x58,&movie_type,4) || ((movie_type&0x8f)<8) || ((movie_type&0x8f)>11))goto done;
    uint64_t reward_manager;
    if(!read_local(NULL,session.context+0x7e08,&reward_manager,8))goto icon_failed;
    uint16_t *icon=custom_icon_v115;
    uint32_t icon_length;
    if(custom_pending_ap_icon_v241) {
        static const wchar_t ap_icon[]=L"/Game/UI/Icon/Item/apicon1.apicon1";
        icon_length=(uint32_t)(sizeof(ap_icon)/sizeof(ap_icon[0])-1);
        memcpy(icon,ap_icon,sizeof(ap_icon));
    } else icon_length=Kh02PopupNativeIconV115(read_local,NULL,reward_manager,icon);
    if(!icon_length)goto icon_failed;
    /* Only replace the pinned displayed text once the native widget is idle.
       A pending request has separate storage while the prior popup animates. */
    memcpy(custom_active_text_v115,custom_pending_text_v115,sizeof(custom_active_text_v115));
    const uint16_t *text=custom_active_text_v115;
    uint32_t length=custom_pending_length_v115;
    if(!Kh02CustomPopupArgsV115(&args,(const uint16_t*)text,length,icon,icon_length) ||
       !popup_same_v111(&s,custom_popup_v115.epoch))goto done;
    /* Pinned module strings remain alive beyond asynchronous movie processing.
       Consume before call; native output owns only its two temporary buffers. */
    custom_popup_v115.pending=0;custom_popup_v115.submitted++;
    custom_popup_v115.status=6;custom_report_v115();
    ((Invoke)(game.base+0x56ac50))((void*)(uintptr_t)widget,&result,&method_arg,&list);
    custom_popup_v115.status=8;custom_report_v115();
    if(result.text.data)((Free)(game.base+0xac3dd0))((void*)result.text.data);
    if(result.narrow)((Free)(game.base+0xac3dd0))((void*)result.narrow);
    if(!popup_same_v111(&s,custom_popup_v115.epoch) ||
       !read_local(NULL,manager+0x1a0,&node,8) || node!=widget ||
       !read_local(NULL,widget,&vt,8) || vt!=game.base+0x3780450 ||
       !read_local(NULL,widget+0x40,&node,8) || node!=movie)goto done;
    custom_popup_v115.status=9;custom_report_v115();
    ((Activate)(game.base+0x58c770))((void*)(uintptr_t)widget);
    if(popup_same_v111(&s,custom_popup_v115.epoch) &&
       read_local(NULL,ui+0x88,&node,8) && node==manager) {
        /* Match native58171d: dirty UI only, no gameplay or inventory state. */
        unsigned char dirty=1;SIZE_T written=0;
        if(!WriteProcessMemory(GetCurrentProcess(),(void*)(uintptr_t)(manager+0x218),&dirty,1,&written) || written!=1)goto done;
    } else goto done;
    custom_popup_v115.status=7;
    goto done;
icon_failed:
    custom_popup_v115.pending=0;custom_popup_v115.status=21;
done:
    InterlockedExchange(&busy,0);custom_report_v115();
}
