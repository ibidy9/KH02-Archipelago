#include "resume_authorization.h"
#include <string.h>
#include <wchar.h>
static uint32_t le32(const unsigned char *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static uint64_t le64(const unsigned char *p){return le32(p)|((uint64_t)le32(p+4)<<32);}
unsigned Kh02ReadResumeAuthorization(const wchar_t *path,const Kh02StartupRequest *r,Kh02ResumeAuthorization *a) {
    a->input=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,NULL);
    if(a->input==INVALID_HANDLE_VALUE){a->input=NULL;return GetLastError()==ERROR_FILE_NOT_FOUND?KH02_LAB_NO_INTENT:KH02_LAB_INTENT_ERROR;}
    BY_HANDLE_FILE_INFORMATION info;LARGE_INTEGER size;DWORD got=0;
    if(!GetFileInformationByHandle(a->input,&info) || (info.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT)) ||
       !GetFileSizeEx(a->input,&size) || size.QuadPart!=KH02_RESUME_BYTES ||
       !ReadFile(a->input,a->bytes,sizeof(a->bytes),&got,NULL) || got!=sizeof(a->bytes))goto failed;
    const unsigned char *p=a->bytes;
    if(memcmp(p,"KH02RS01",8) || le32(p+8)!=1 || le32(p+12)!=KH02_RESUME_BYTES ||
       memcmp(p+16,r->binding,32) || memcmp(p+48,r->nonce,32) ||
       le32(p+80)!=r->root_volume || le32(p+84) || le64(p+88)!=r->root_file ||
       le64(p+128)!=r->prepared || le64(p+136)!=r->expires)goto failed;
    return KH02_LAB_AP_INTENT;
failed:
    CloseHandle(a->input);a->input=NULL;return KH02_LAB_INTENT_ERROR;
}
BOOL Kh02CheckResumeRoot(const Kh02StartupRequest *r,void *context) {
    Kh02ResumeAuthorization *a=context;
    wchar_t path[KH02_PATH_ROOT_CAP+20];WIN32_FIND_DATAW info;
    wcscpy(path,r->root);wcscat(path,L"*");
    HANDLE find=FindFirstFileW(path,&info);if(find==INVALID_HANDLE_VALUE)return FALSE;
    unsigned count=0;BOOL ok=TRUE;
    do {
        if(!wcscmp(info.cFileName,L".") || !wcscmp(info.cFileName,L".."))continue;
        if(wcscmp(info.cFileName,L"KH0.2BbS_WW.png") || (info.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))) {ok=FALSE;break;}
        count++;
    }while(FindNextFileW(find,&info));
    if(ok && GetLastError()!=ERROR_NO_MORE_FILES)ok=FALSE;
    FindClose(find);if(!ok || count!=1)return FALSE;
    wcscpy(path,r->root);wcscat(path,L"KH0.2BbS_WW.png");
    unsigned char hash[32];
    if(!Kh02HashStartupFile(path,hash,&a->file))return FALSE;
    BY_HANDLE_FILE_INFORMATION identity;
    ok=GetFileInformationByHandle(a->file,&identity) && identity.nNumberOfLinks==1 && !memcmp(hash,a->bytes+96,32);
    if(!ok)Kh02ReleaseResumeFile(a);
    /* Retained read-only handle prevents replacement/writes through installation.
       Release before native game startup, so native saving remains possible. */
    return ok;
}
void Kh02ReleaseResumeFile(Kh02ResumeAuthorization *a){if(a->file){CloseHandle(a->file);a->file=NULL;}}
