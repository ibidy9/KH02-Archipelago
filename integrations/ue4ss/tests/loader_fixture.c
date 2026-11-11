/* Loads only synthetic fixtures. Never loads the real host or framework. */
#ifdef NDEBUG
#error Fixture assertions must be enabled
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <stdint.h>
#include <wchar.h>
#include <stdio.h>
typedef uint32_t (*Query)(void);
int wmain(int argc,wchar_t **argv) {
 assert(argc==2);
 wchar_t root[1024],target[1024],decoy[1024];
 DWORD n=GetModuleFileNameW(NULL,root,1024);assert(n && n<1024);
 while(n && root[n-1]!=L'\\' && root[n-1]!=L'/')--n;assert(n);root[n]=0;
 assert(swprintf_s(target,1024,L"%lsKH02UE4SSReceiptHost\\Mods\\KH02ReceiptHost\\dlls\\main.dll",root)>0);
 assert(swprintf_s(decoy,1024,L"%lsdecoy\\main.dll",root)>0);
 HMODULE fixture=LoadLibraryW(argv[1]);assert(fixture);
 Query call=(Query)(uintptr_t)GetProcAddress(fixture,"Kh02UE4SSTestLoadedQuery");assert(call);
 assert(!GetModuleHandleW(target));assert(call()==0);assert(!GetModuleHandleW(target));
 HMODULE other=LoadLibraryW(decoy);assert(other);
 Query other_count=(Query)(uintptr_t)GetProcAddress(other,"FixtureQueries");assert(other_count);
 assert(call()==0 && other_count()==0 && !GetModuleHandleW(target));
 HMODULE actual=LoadLibraryW(target);assert(actual);
 Query actual_count=(Query)(uintptr_t)GetProcAddress(actual,"FixtureQueries");assert(actual_count);
 assert(call()==0 && actual_count()==1 && other_count()==0);
 assert(FreeLibrary(actual) && FreeLibrary(other) && FreeLibrary(fixture));
 puts("PASS exact loaded-module query in Unicode executable path; absent module is not loaded; same-name decoy is not queried; fixtures unload");
 return 0;
}
