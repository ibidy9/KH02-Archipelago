#include <windows.h>
#include "../native/sdk_pump_api.h"
static unsigned calls;
__declspec(dllexport) unsigned Kh02MockCalls(void){return calls;}
__declspec(dllexport) uint32_t Kh02SdkOwnerPump(uint32_t version,uint32_t size,Kh02SdkPumpStatus* out){
 if(version!=1 || size!=sizeof(*out) || !out)return 0;
 *out=(Kh02SdkPumpStatus){sizeof(*out),1,GetCurrentProcessId(),GetCurrentThreadId(),1,++calls,calls,0,0,1,0,0};return 1;
}
