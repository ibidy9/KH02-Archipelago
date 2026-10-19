/* Offline candidate. Preparation/authentication and game failure handling are external. */
#ifndef KH02_LAB_PATH_ROUTE_H
#define KH02_LAB_PATH_ROUTE_H
#include "launch_mode.h"
#define KH02_PATH_ROOT_CAP 240
typedef struct Kh02PathText { wchar_t *data; int count,capacity; } Kh02PathText;
typedef Kh02PathText *(*Kh02PathOriginal)(void*,Kh02PathText*,const wchar_t*);
typedef Kh02PathText *(*Kh02PathFormat)(Kh02PathText*,const wchar_t*,...);
typedef struct Kh02PathRoute {
    SRWLOCK lock;
    unsigned initialized,mode;
    uint64_t created;
    unsigned char binding[32];
    wchar_t root[KH02_PATH_ROOT_CAP];
} Kh02PathRoute;
enum { KH02_PATH_REFUSED, KH02_PATH_ORIGINAL, KH02_PATH_REDIRECTED };
/* Zero initialize before use. First call fixes the decision, including refusal.
   root_validated means an external preparer verified isolation and directory identity;
   this kernel only checks the bounded string shape. It does not authenticate a root. */
unsigned Kh02PathChoose(Kh02PathRoute*,unsigned mode,uint64_t created,
                       const unsigned char binding[32],const wchar_t *root,
                       unsigned root_validated);
/* Refusal invokes NEITHER callback. The caller must handle it before game I/O;
   this status must never be cast to a native filename pointer or trigger fallback.
   Output must be unused constructor storage, exactly as at the original callsite. */
unsigned Kh02PathCall(Kh02PathRoute*,uint64_t created,const unsigned char binding[32],
                     void *owner,Kh02PathText *out,const wchar_t *name,
                     Kh02PathOriginal,Kh02PathFormat,Kh02PathText **returned);
#endif
