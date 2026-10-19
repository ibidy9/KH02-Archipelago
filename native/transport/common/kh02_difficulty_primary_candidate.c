/* Isolated offline primitive. Caller must establish supported build, exact
   context identity, owner thread, authenticated known load and pending request.
   Not linked into a game runtime. Never changes the checkpoint or save record. */
#include <windows.h>
#include <stdint.h>
#include <string.h>
__declspec(dllexport) uint32_t Kh02DifficultyPrimaryChange(uintptr_t context,
        uint32_t expected,uint32_t desired) {
    MEMORY_BASIC_INFORMATION m;uintptr_t address;uint32_t value;
    if(context<0x10000 || (context&7) || context>0x7fffffffffffULL-0x53c ||
       expected>3 || desired>3)return 2;
    address=context+0x538;
    if(!VirtualQuery((void*)address,&m,sizeof(m)) || m.State!=MEM_COMMIT ||
       (m.Protect&(PAGE_GUARD|PAGE_NOACCESS)) ||
       !(m.Protect&(PAGE_READWRITE|PAGE_WRITECOPY)) ||
       m.RegionSize>UINTPTR_MAX-(uintptr_t)m.BaseAddress ||
       address+4>(uintptr_t)m.BaseAddress+m.RegionSize)return 2;
    memcpy(&value,(void*)address,4);
    if(value!=expected)return 3;
    memcpy((void*)address,&desired,4);memcpy(&value,(void*)address,4);
    if(value!=desired){memcpy((void*)address,&expected,4);return 4;}
    return 1;
}
