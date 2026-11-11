/* Synthetic filename fixture, with both authority exports checked together. */
#define main filename_fixture_main
#include "filename_fixture.c"
#undef main
int main(int argc,char **argv) {
    int status=filename_fixture_main(argc,argv);if(status)return status;
    HMODULE module=GetModuleHandleW(L"dinput8.dll");assert(module);
    typedef unsigned (*Check)(uint64_t,const unsigned char*,const unsigned char*);
    Check fresh=(Check)(uintptr_t)GetProcAddress(module,"Kh02FreshStorageCheck");
    Check resume=(Check)(uintptr_t)GetProcAddress(module,"Kh02ResumeStorageCheck");
    FilenameEvidence *e=(void*)GetProcAddress(module,"Kh02FilenameEvidence");
    assert(fresh && resume && e);
    unsigned char bytes[1104];DWORD got;
    HANDLE f=CreateFileW(L"kh02-ap-ready.bin",GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    assert(f!=INVALID_HANDLE_VALUE && ReadFile(f,bytes,sizeof(bytes),&got,NULL) && got==sizeof(bytes));CloseHandle(f);
    unsigned is_resume=0;
    if(GetFileAttributesW(L"kh02-ap-resume.bin")!=INVALID_FILE_ATTRIBUTES) {
        unsigned char kind[8];
        f=CreateFileW(L"kh02-ap-resume.bin",GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
        assert(f!=INVALID_HANDLE_VALUE && ReadFile(f,kind,8,&got,NULL) && got==8);CloseHandle(f);
        is_resume=!memcmp(kind,"KH02RS01",8);
    }
    assert(fresh(e->created,e->binding,bytes+96)==!is_resume);
    assert(resume(e->created,e->binding,bytes+96)==is_resume);
    Check checks[]={fresh,resume};
    for(unsigned i=0;i<2;i++) {
        assert(!checks[i](e->created+1,e->binding,bytes+96));
        assert(!checks[i](e->created,NULL,bytes+96));
        assert(!checks[i](e->created,e->binding,NULL));
        bytes[96]^=1;assert(!checks[i](e->created,e->binding,bytes+96));bytes[96]^=1;
        unsigned char wrong[32];memcpy(wrong,e->binding,32);wrong[0]^=1;
        assert(!checks[i](e->created,wrong,bytes+96));
    }
    /* A second startup cannot switch the already-latched storage authority. */
    assert(!Kh02FilenameTestBoot(module,NULL));
    assert(fresh(e->created,e->binding,bytes+96)==!is_resume);
    assert(resume(e->created,e->binding,bytes+96)==is_resume);
    puts("PASS mutually exclusive fresh/resume authority, scope/nonce/process rejection, no mode switch");
    return 0;
}
