#include "path_route.h"
#include <string.h>
#include <wchar.h>

static unsigned root_length(const wchar_t *root) {
    if(!root)return 0;
    unsigned n=0;
    while(n<KH02_PATH_ROOT_CAP && root[n])n++;
    if(n<4 || n>=KH02_PATH_ROOT_CAP || root[n-1]!=L'/' ||
       !((root[0]>=L'A'&&root[0]<=L'Z')||(root[0]>=L'a'&&root[0]<=L'z')) ||
       root[1]!=L':' || root[2]!=L'/')return 0;
    unsigned start=3;
    for(unsigned i=3;i<n;i++) {
        wchar_t ch=root[i];
        if(ch<32 || ch==L'\\' || ch==L':' || ch==L'"' || ch==L'<' ||
           ch==L'>' || ch==L'|' || ch==L'?' || ch==L'*')return 0;
        if(ch==L'/') {
            if(i==start || root[i-1]==L'.' || root[i-1]==L' ')return 0;
            start=i+1;
        }
    }
    return n;
}
unsigned Kh02PathChoose(Kh02PathRoute *s,unsigned mode,uint64_t created,
                       const unsigned char binding[32],const wchar_t *root,
                       unsigned root_validated) {
    if(!s)return KH02_LAB_BLOCKED;
    AcquireSRWLockExclusive(&s->lock);
    if(!s->initialized) {
        s->initialized=1;s->mode=KH02_LAB_BLOCKED;s->created=created;
        if(created && mode==KH02_LAB_VANILLA && !binding && !root && !root_validated)
            s->mode=KH02_LAB_VANILLA;
        else if(created && mode==KH02_LAB_AP && binding && root_validated==1) {
            unsigned nonzero=0,n=root_length(root);
            for(unsigned i=0;i<32;i++)nonzero|=binding[i];
            if(n && nonzero) {
                memcpy(s->binding,binding,32);
                memcpy(s->root,root,(n+1)*sizeof(wchar_t));
                s->mode=KH02_LAB_AP;
            }
        }
    }
    unsigned result=s->mode;
    ReleaseSRWLockExclusive(&s->lock);
    return result;
}
unsigned Kh02PathCall(Kh02PathRoute *s,uint64_t created,const unsigned char binding[32],
                     void *owner,Kh02PathText *out,const wchar_t *name,
                     Kh02PathOriginal original,Kh02PathFormat format,Kh02PathText **returned) {
    if(!s || !out || !name || !returned)return KH02_PATH_REFUSED;
    unsigned mode;
    AcquireSRWLockShared(&s->lock);
    mode=s->initialized && created && created==s->created?s->mode:KH02_LAB_BLOCKED;
    if(mode==KH02_LAB_VANILLA && binding)mode=KH02_LAB_BLOCKED;
    if(mode==KH02_LAB_AP && (!binding || memcmp(binding,s->binding,32)))mode=KH02_LAB_BLOCKED;
    ReleaseSRWLockShared(&s->lock);
    /* Mode/root are immutable after first choice; no lock across native callbacks. */
    if(mode==KH02_LAB_VANILLA && original) {
        *returned=original(owner,out,name);return KH02_PATH_ORIGINAL;
    }
    if(mode!=KH02_LAB_AP || !format)return KH02_PATH_REFUSED;
    /* Unknown AP names are refused until their storage ownership is classified. */
    if(wcscmp(name,L"KH0.2BbS_WW") && wcscmp(name,L"KH0.2BbS_JP"))return KH02_PATH_REFUSED;
    *returned=format(out,L"%s%s.png",s->root,name);
    return KH02_PATH_REDIRECTED;
}
