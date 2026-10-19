#include "kh02_meter_access_v48.h"
#include "kh02_meter_projection_v48.h"
static Kh02MeterAccess meter_access;
static int meter_write(void *unused,uint64_t address,const void *value,uint32_t size) {
    SIZE_T wrote=0;(void)unused;
    if(size!=1 || !is_active(NULL))return 0;
    return WriteProcessMemory(GetCurrentProcess(),(void*)(uintptr_t)address,value,1,&wrote) && wrote==1;
}
void Kh02MeterReconcile(void) {
    uint32_t magic=0,style=0,status;Kh02MeterProjection result;
    InterlockedIncrement(&flight);magic_inc(&meter_access.calls);
    if(GetCurrentThreadId()!=owner_thread || !is_active(NULL) ||
       !session.context || !session.world || !session.gate.current.observation ||
       InterlockedCompareExchange(&busy,0,0) ||
       !Kh02MagicOwnership(&magic) || !Kh02StyleOwnership(&style)) {
        magic_inc(&meter_access.refused);goto done;
    }
    meter_access.epoch=seen_epoch;meter_access.owner=session.context;
    meter_access.magic=magic;meter_access.style=style;
    if(!(magic&42) && !style){magic_inc(&meter_access.no_ownership);goto done;}
    status=Kh02MeterProject(read_local,meter_write,NULL,session.world,session.context,&result);
    meter_access.status=status;meter_access.before=result.before;meter_access.after=result.after;
    meter_access.reference=result.reference;
    if(status==KH02_GATE_OK)magic_inc(result.changed?&meter_access.applied:&meter_access.unchanged);
    else if(status==KH02_GATE_UNCERTAIN) {
        magic_inc(&meter_access.uncertain);InterlockedExchange(&fault,1);InterlockedExchange(&active_flag,0);
    } else magic_inc(&meter_access.refused);
done:
    InterlockedDecrement(&flight);
}
void Kh02MeterAccessSnapshot(Kh02MeterAccess *out) {
    if(out)memcpy(out,&meter_access,sizeof(*out));
}
