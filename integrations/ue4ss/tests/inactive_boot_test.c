/* Dedicated synthetic executable, never the game. Loads an unchanged startup
 * DLL beside this test executable, with no readiness file. */
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#ifdef NDEBUG
#error Keep test assertions enabled
#endif
typedef unsigned (*ModeFn)(void);
int main(void) {
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    assert(GetFileAttributesW(L"kh02-ap-ready.bin")==INVALID_FILE_ATTRIBUTES);
    HMODULE module=LoadLibraryExW(L".\\dinput8.dll",NULL,LOAD_WITH_ALTERED_SEARCH_PATH);
    assert(module);
    ModeFn mode=(ModeFn)(uintptr_t)GetProcAddress(module,"Kh02FilenameMode");
    const uint32_t *record=(const uint32_t*)GetProcAddress(module,"Kh02FilenameEvidence");
    assert(mode && record && mode()==1 && record[0]==0x4b484649 && record[3]==1);
    for(unsigned i=4;i<10;i++)assert(record[i]==0);
    puts("PASS exact startup DLL: no intent selects vanilla; no hooks or filename events");
    /* Keep the DLL resident until this synthetic process exits, like the game. */
    return 0;
}
