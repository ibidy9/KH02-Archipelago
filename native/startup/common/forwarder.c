/* Offline forwarding experiment. No game hooks or save-path changes. */
#include <windows.h>
typedef HRESULT (WINAPI *CreateFn)(HINSTANCE,DWORD,const GUID*,void**,IUnknown*);
static INIT_ONCE once=INIT_ONCE_STATIC_INIT;
static CreateFn original;
static HMODULE retained;
static BOOL CALLBACK initialize(PINIT_ONCE lock,PVOID parameter,PVOID *context) {
    wchar_t path[MAX_PATH],actual[MAX_PATH];UINT length;HMODULE module;
    (void)lock;(void)parameter;(void)context;
    length=GetSystemDirectoryW(path,MAX_PATH);
    if(!length || length>=MAX_PATH-13)return TRUE;
    if(lstrcatW(path,L"\\dinput8.dll")==NULL)return TRUE;
    module=LoadLibraryExW(path,NULL,LOAD_LIBRARY_SEARCH_SYSTEM32);
    if(!module)return TRUE;
    /* Keep the system library resident, including on validation failure. */
    retained=module;
    length=GetModuleFileNameW(module,actual,MAX_PATH);
    if(!length || length>=MAX_PATH || lstrcmpiW(path,actual))return TRUE;
    original=(CreateFn)(void*)GetProcAddress(retained,"DirectInput8Create");
    return TRUE;
}
__declspec(dllexport) HRESULT WINAPI DirectInput8Create(HINSTANCE instance,DWORD version,
    const GUID *iid,void **object,IUnknown *outer) {
    /* Called outside DllMain in this lab. No worker or load in initialization. */
    if(!InitOnceExecuteOnce(&once,initialize,NULL,NULL) || !original)
        return HRESULT_FROM_WIN32(ERROR_DLL_INIT_FAILED);
    return original(instance,version,iid,object,outer);
}
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID reserved) {
    (void)module;(void)reason;(void)reserved;
    return TRUE;
}
