#include "startup_gate.h"
#include <string.h>
#include <wchar.h>

static BOOL nonzero(const unsigned char *p) {
    unsigned n=0;for(unsigned i=0;i<32;i++)n|=p[i];return n!=0;
}
static BOOL directory_shape(const wchar_t *path) {
    Kh02PathRoute temporary={0};unsigned char binding[32]={1};
    return Kh02PathChoose(&temporary,KH02_LAB_AP,1,binding,path,1)==KH02_LAB_AP;
}
static void unpin(Kh02StartupGate *s) {
    while(s->pin_count)CloseHandle(s->pins[--s->pin_count]);
}
/* Open and retain every path component without delete sharing. Reject junctions
   and symlinks, including ancestors, instead of resolving them into another root. */
static BOOL pin_directory(Kh02StartupGate *s,const wchar_t *path,BY_HANDLE_FILE_INFORMATION *last) {
    wchar_t prefix[KH02_PATH_ROOT_CAP];
    if(!directory_shape(path))return FALSE;
    size_t n=wcslen(path);
    memcpy(prefix,path,(n+1)*sizeof(wchar_t));
    for(size_t i=2;i<n;i++)if(prefix[i]==L'/') {
        if(s->pin_count>=64)return FALSE;
        if(i!=2)prefix[i]=0;else prefix[i+1]=0;
        HANDLE h=CreateFileW(prefix,FILE_LIST_DIRECTORY|FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE,
            NULL,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT,NULL);
        memcpy(prefix,path,(n+1)*sizeof(wchar_t));
        if(h==INVALID_HANDLE_VALUE)return FALSE;
        s->pins[s->pin_count++]=h;
        if(!GetFileInformationByHandle(h,last) || !(last->dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) ||
            (last->dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT))return FALSE;
    }
    return TRUE;
}
static BOOL empty_root(const wchar_t *root) {
    wchar_t pattern[KH02_PATH_ROOT_CAP+2];WIN32_FIND_DATAW info;
    wcscpy(pattern,root);wcscat(pattern,L"*");
    HANDLE search=FindFirstFileW(pattern,&info);
    if(search==INVALID_HANDLE_VALUE)return GetLastError()==ERROR_FILE_NOT_FOUND;
    BOOL empty=TRUE;
    do {
        if(wcscmp(info.cFileName,L".") && wcscmp(info.cFileName,L"..")){empty=FALSE;break;}
    }while(FindNextFileW(search,&info));
    if(empty && GetLastError()!=ERROR_NO_MORE_FILES)empty=FALSE;
    FindClose(search);return empty;
}
static BOOL claim(const Kh02StartupRequest *r,uint64_t created) {
    wchar_t path[KH02_PATH_ROOT_CAP+72];
    const wchar_t hex[]=L"0123456789abcdef";
    size_t n=wcslen(r->claim_directory);wcscpy(path,r->claim_directory);
    for(unsigned i=0;i<32;i++) {
        path[n+2*i]=hex[r->nonce[i]>>4];path[n+2*i+1]=hex[r->nonce[i]&15];
    }
    wcscpy(path+n+64,L".claim");
    HANDLE h=CreateFileW(path,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
    if(h==INVALID_HANDLE_VALUE)return FALSE;
    /* Even a partial/failed claim stays spent. No deletion or automatic retry. */
    DWORD wrote=0;
    BOOL ok=WriteFile(h,&created,sizeof(created),&wrote,NULL)&&wrote==sizeof(created)&&FlushFileBuffers(h);
    CloseHandle(h);return ok;
}
BOOL Kh02StartupBeginChecked(Kh02StartupGate *s,unsigned intent,const Kh02StartupRequest *input,
    uint64_t created,uint64_t now,const unsigned char build[32],Kh02StartupInstall install,void *context,
    Kh02StartupRootCheck root_check,void *root_context) {
    if(!s)return FALSE;
    AcquireSRWLockExclusive(&s->lock);
    if(s->attempted){ReleaseSRWLockExclusive(&s->lock);return FALSE;}
    s->attempted=1;s->mode=KH02_LAB_BLOCKED;s->created=created;s->error=KH02_STARTUP_INPUT;
    if(!created || now<created)goto failed;
    if(intent==KH02_LAB_NO_INTENT && !input) {
        s->mode=Kh02PathChoose(&s->route,KH02_LAB_VANILLA,created,NULL,NULL,0);
        s->ready=1;s->error=KH02_STARTUP_OK;
        ReleaseSRWLockExclusive(&s->lock);return TRUE;
    }
    if(intent!=KH02_LAB_AP_INTENT || !input || !build || !install)goto failed;
    Kh02StartupRequest r=*input;
    if(r.version!=KH02_STARTUP_VERSION || r.size!=sizeof(r) || !nonzero(r.binding) || !nonzero(r.nonce))goto failed;
    s->error=KH02_STARTUP_TIME;
    if(!r.prepared || r.prepared>=created || r.expires<=r.prepared || now>=r.expires)goto failed;
    s->error=KH02_STARTUP_BUILD;
    if(!nonzero(build) || memcmp(r.build,build,32))goto failed;
    s->error=KH02_STARTUP_ROOT;
    if(!directory_shape(r.root) || !directory_shape(r.claim_directory))goto failed;
    size_t a=wcslen(r.root),b=wcslen(r.claim_directory);
    /* State/claim files cannot live inside the fresh saves directory (or vice versa). */
    if(!_wcsnicmp(r.root,r.claim_directory,a<b?a:b))goto failed;
    BY_HANDLE_FILE_INFORMATION identity;
    if(!pin_directory(s,r.root,&identity) || identity.dwVolumeSerialNumber!=r.root_volume ||
       (((uint64_t)identity.nFileIndexHigh<<32)|identity.nFileIndexLow)!=r.root_file ||
       !(root_check?root_check(&r,root_context):empty_root(r.root)) ||
       !pin_directory(s,r.claim_directory,&identity))goto failed;
    s->error=KH02_STARTUP_CLAIM;
    if(!claim(&r,created))goto failed;
    if(Kh02PathChoose(&s->route,KH02_LAB_AP,created,r.binding,r.root,1)!=KH02_LAB_AP)goto failed;
    s->error=KH02_STARTUP_INSTALL;
    if(!install(context))goto failed;
    s->mode=KH02_LAB_AP;s->ready=1;s->error=KH02_STARTUP_OK;
    ReleaseSRWLockExclusive(&s->lock);return TRUE;
failed:
    unpin(s);ReleaseSRWLockExclusive(&s->lock);return FALSE;
}
BOOL Kh02StartupBegin(Kh02StartupGate *s,unsigned intent,const Kh02StartupRequest *input,
    uint64_t created,uint64_t now,const unsigned char build[32],Kh02StartupInstall install,void *context) {
    return Kh02StartupBeginChecked(s,intent,input,created,now,build,install,context,NULL,NULL);
}
unsigned Kh02StartupPath(Kh02StartupGate *s,uint64_t created,const unsigned char binding[32],
    void *owner,Kh02PathText *out,const wchar_t *name,Kh02PathOriginal original,
    Kh02PathFormat format,Kh02PathText **returned) {
    if(!s)return KH02_PATH_REFUSED;
    AcquireSRWLockShared(&s->lock);unsigned ready=s->ready;
    ReleaseSRWLockShared(&s->lock);
    if(!ready)return KH02_PATH_REFUSED;
    return Kh02PathCall(&s->route,created,binding,owner,out,name,original,format,returned);
}
