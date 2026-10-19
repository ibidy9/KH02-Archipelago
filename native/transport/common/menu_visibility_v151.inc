/* Two scoped native visibility predicates. Never changes game/save records. */
static void *menu_predicate_trampoline151,*menu_discovery_unused151;
static const unsigned char menu_predicate_prefix151[15]={0x48,0x89,0x5c,0x24,8,0x48,0x89,0x74,0x24,0x10,0x57,0x48,0x83,0xec,0x20};
static const unsigned char menu_discovery_prefix151[14]={0x4c,0x8b,0xc9,0x83,0xfa,0x1f,0x77,0x26,0x8b,0xc2,0x99,0x83,0xe2,0x1f};
extern void kh02_menu_discovery_thunk151(void);
static uint64_t menu_row151,menu_state151,menu_epoch151,menu_cookie151;
/* Bounded records in the existing diagnostic mapping. No gameplay writes. */
static volatile LONG menu_command_trace202,menu_rows_trace202;
static void menu_trace202(uintptr_t caller,uint32_t phase,uint32_t value) {
    volatile LONG *count=phase<220?&menu_command_trace202:&menu_rows_trace202;
    LONG limit=phase<220?24:40;
    if(InterlockedIncrement(count)<=limit)
        Kh02OpeningTraceStart(game.base,caller,session.context,phase,value);
}
static int menu_scope151(void) {
    return GetCurrentThreadId()==owner_thread && active_flag && !fault && !busy &&
        Kh02ControlledStartAuthorized() && Kh02LifecycleCurrent(life)!=NULL;
}
static int menu_classify151(uint64_t row,unsigned *world,unsigned *kind) {
    static const wchar_t *maps[]={L"/Game/Levels/dw/dw_02/dw_02",L"/Game/Levels/dw/dw_03/dw_03",L"/Game/Levels/dw/dw_08/dw_08",L"/Game/Levels/dw/dw_08/dw_08",L"/Game/Levels/dw/dw_08/dw_08",L"/Game/Levels/dw/dw_09/dw_09",L"/Game/Levels/dw/dw_01/dw_01",L"/Game/Levels/dw/dw_59/dw_59",L"/Game/Levels/dw/dw_08/dw_08",L"/Game/Levels/dw/dw_09/dw_09",L"/Game/Levels/dw/dw_07/dw_07",L"/Game/Levels/dw/dw_08/dw_08",L"/Game/Levels/dw/dw_02/dw_02",L"/Game/Levels/dw/dw_08/dw_08"};
    static const wchar_t *spawns[]={L"dw_02_Lv_Save_01",L"dw_03_Lv_Save_01",L"dw_08_Lv_Save_01",L"dw_08_Lv_Save_02",L"dw_08_Lv_Save_03",L"dw_09_Lv_Save_01",L"dw_01_Lv_Start_01",L"dw_59_Lv_Start_01",L"dw_08_Lv_Start_01",L"dw_09_Lv_Start_01",L"dw_07_Lv_Start_01",L"dw_08_Lv_Autosave_06",L"dw_02_Lv_Retry_01",L"dw_08_Lv_Retry_02"};
    static const unsigned worlds[]={0,1,2,2,2,3,0,1,2,3,1,2,0,2};
    uint64_t map,spawn,text;int32_t size,capacity;wchar_t label[64];
    if(row<0x10000 || (row&7) || !read_local(NULL,row+8,&spawn,8) || !read_local(NULL,row+16,&map,8))return 0;
    for(unsigned i=0;i<14;i++) {
        if(map!=start_name(maps[i],1) || spawn!=start_name(spawns[i],1))continue;
        *world=worlds[i];*kind=i>=6&&i<=9?1:(i==0 || i==1 || i==2 || i==5)?3:0;
        if(*kind==1) {
            if(!read_local(NULL,row+0x18,&text,8) || !read_local(NULL,row+0x20,&size,4) ||
               !read_local(NULL,row+0x24,&capacity,4) || size<1 || size>64 || capacity<size ||
               !read_local(NULL,text,label,(uint32_t)size*2) || label[size-1])return 0;
            wchar_t expected[]=L"kh02_ap.travel.0";expected[15]=(wchar_t)(L'0'+*world);
            /* Four appended alternatives; existing restart labels stay intact. */
            if(!wcscmp(label,expected))*kind=2;
        }
        return 1;
    }
    return 0;
}
static int menu_save_bit214(const wchar_t *text,uint32_t *bit) {
    uint64_t name=start_name(text,1);
    if(!name)return 0;
    int32_t index=((int32_t (__fastcall *)(uint64_t*))(game.base+0x58f870))(&name);
    if(index<0 || index>31)return 0;
    *bit=1u<<(uint32_t)index;return 1;
}
static int menu_forest_ready214(uint64_t state,int native,int end) {
    uint32_t flags,bit,story;char map[32];
    if(!menu_save_bit214(end?L"dw_08_Lv_Save_03":L"dw_08_Lv_Save_02",&bit) ||
       !read_local(NULL,state+0x518,&flags,4))return 0;
    if(flags&bit)return 1;
    return native && read_local(NULL,state+8,&story,4) && story>=323 && story<360 &&
        read_local(NULL,state+0x51c,map,32) && !memcmp(map,"/Game/Levels/dw/dw_08/dw_08",26);
}
static int menu_boss_ready239(uint64_t state,unsigned mask) {
    unsigned char flags=0;
    return (mask==4 || mask==32) && read_local(NULL,state+0x20d,&flags,1) && (flags&mask);
}
static int menu_phantom_ready214(uint64_t state) {
    /* state=structured+2ff8. Structured event byte3205, NOT legacy slot3015. */
    unsigned char flags=0;
    return read_local(NULL,state+0x20d,&flags,1) && (flags&8);
}
static int menu_visible151(uint64_t row,uint64_t state,int native) {
    unsigned world,kind;uint32_t flags;uint64_t name;int32_t index;
    static const wchar_t *first[]={L"dw_02_Lv_Save_01",L"dw_03_Lv_Save_01",L"dw_08_Lv_Save_01",L"dw_09_Lv_Save_01"};
    if(!menu_classify151(row,&world,&kind))return native;
    if(!Kh02MenuWorldOwnedV150(world))return 0;
    if(!kind) {
        uint64_t spawn;
        if(!read_local(NULL,row+8,&spawn,8))return 0;
        if(world==0 && spawn==start_name(L"dw_02_Lv_Retry_01",1))return menu_boss_ready239(state,4);
        if(world==2 && spawn==start_name(L"dw_08_Lv_Retry_02",1))return menu_boss_ready239(state,32);
        if(world==1 && spawn==start_name(L"dw_07_Lv_Start_01",1))return menu_phantom_ready214(state);
        if(world==2 && spawn==start_name(L"dw_08_Lv_Autosave_06",1))return menu_forest_ready214(state,1,0);
        if(world==2 && spawn==start_name(L"dw_08_Lv_Save_02",1))return menu_forest_ready214(state,native,0);
        if(world==2 && spawn==start_name(L"dw_08_Lv_Save_03",1))return menu_forest_ready214(state,native,1);
        return native;
    }
    name=start_name(first[world],1);
    if(!name || !read_local(NULL,state+0x518,&flags,4))return 0;
    index=((int32_t (__fastcall *)(uint64_t*))(game.base+0x58f870))(&name);
    if(index<0 || index>31)return 0;
    return kind==2?!(flags&(1u<<index)):kind==3?(native && !!(flags&(1u<<index))):!!(flags&(1u<<index));
}
uint32_t kh02_menu_discovery_capture151(void *state,int32_t index,void *row,uintptr_t caller,uint32_t catalog_index) {
    uint32_t flags=0,result=0;
    if((uint32_t)index<=31 && read_local(NULL,(uintptr_t)state+0x518,&flags,4))result=(flags>>index)&1;
    if(caller!=game.base+0x5d19f5 || !menu_scope151())return result;
    menu_row151=(uintptr_t)row;menu_state151=(uintptr_t)state;
    menu_epoch151=Kh02LifecycleEpoch(life);menu_cookie151=session.gate.current.cookie;
    uint32_t shown=(uint32_t)menu_visible151(menu_row151,menu_state151,result);
    menu_begin213(menu_row151,menu_state151,catalog_index,shown);
    menu_trace202(caller,220,(result<<16)|shown);
    return shown;
}
static uint8_t __fastcall menu_predicate151(void *condition,void *threshold) {
    typedef uint8_t (__fastcall *Fn)(void*,void*);
    uintptr_t caller=(uintptr_t)__builtin_return_address(0);
    uint8_t result=((Fn)menu_predicate_trampoline151)(condition,threshold);
    if(caller==game.base+0x5d17c1) {
        uint32_t flags=(GetCurrentThreadId()==owner_thread?1u:0u) |
            (active_flag?2u:0u)|(fault?4u:0u)|(busy?8u:0u)|
            (session.context?16u:0u)|(session.stopped?32u:0u)|(session.uncertain?64u:0u);
        menu_trace202(caller,210,((uint32_t)result<<16)|flags);
    }
    /* Save menu's separate command eligibility, not destination ownership.
     * Native builder reads ordinary WW row1 before enumerating destinations. */
    if(caller==game.base+0x5d17c1 && menu_scope151()) {
        uint64_t row=(uintptr_t)condition-0x38;
        unsigned world,kind;Kh02ReceiptOwner *receipt=Kh02LifecycleCurrent(life);
        if((uintptr_t)threshold==row+0x40 && menu_classify151(row,&world,&kind) &&
           world==1 && kind==3 && session.context && session.gate.current.cookie &&
           seen_epoch==Kh02LifecycleEpoch(life) && receipt &&
           Kh02ReceiptOwnerCheck(receipt)==KH02_MEMBER_OK && menu_root150(session.context)) {
            menu_trace202(caller,211,1);return 1;
        }
        menu_trace202(caller,211,result);
        return result;
    }
    if(caller!=game.base+0x5d1a05 || !menu_scope151())return result;
    uint64_t row=(uintptr_t)condition-0x38;
    if((uintptr_t)threshold!=row+0x40 || row!=menu_row151 ||
       menu_epoch151!=Kh02LifecycleEpoch(life) || menu_cookie151!=session.gate.current.cookie)return 0;
    result=(uint8_t)menu_visible151(row,menu_state151,result);
    menu_end213(row,result);
    menu_trace202(caller,221,result);
    menu_row151=menu_state151=0;return result;
}
