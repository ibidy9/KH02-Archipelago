/* Side-by-side startup candidate. No game hook is installed by this module. */
#ifndef KH02_STARTUP_GATE_H
#define KH02_STARTUP_GATE_H
#include "path_route.h"
#define KH02_STARTUP_VERSION 1
typedef struct Kh02StartupRequest {
    unsigned version,size;
    uint64_t prepared,expires;
    unsigned char build[32],binding[32],nonce[32];
    DWORD root_volume;
    uint64_t root_file;
    wchar_t root[KH02_PATH_ROOT_CAP];
    wchar_t claim_directory[KH02_PATH_ROOT_CAP];
} Kh02StartupRequest;
typedef struct Kh02StartupGate {
    SRWLOCK lock;
    unsigned attempted,ready,mode,error;
    uint64_t created;
    Kh02PathRoute route;
    HANDLE pins[64];
    unsigned pin_count;
} Kh02StartupGate;
enum { KH02_STARTUP_OK,KH02_STARTUP_INPUT,KH02_STARTUP_TIME,
       KH02_STARTUP_BUILD,KH02_STARTUP_ROOT,KH02_STARTUP_CLAIM,
       KH02_STARTUP_INSTALL };
/* Installer must be an offline-tested early hook operation, never call back into
   this gate. A false result means the host MUST NOT proceed into game startup.
   build must come from independent executable verification, not the request.
   Intent discovery and request-file parsing are owned by the future bootstrap. */
typedef BOOL (*Kh02StartupInstall)(void *context);
/* Optional separately verified existing-root policy. Called only after both
   the requested root and its ancestors are pinned and identity-validated. */
typedef BOOL (*Kh02StartupRootCheck)(const Kh02StartupRequest*,void *context);
BOOL Kh02StartupBeginChecked(Kh02StartupGate*,unsigned intent,
    const Kh02StartupRequest*,uint64_t created,uint64_t now,
    const unsigned char build[32],Kh02StartupInstall,void *context,
    Kh02StartupRootCheck,void *root_context);
BOOL Kh02StartupBegin(Kh02StartupGate*,unsigned intent,
    const Kh02StartupRequest*,uint64_t created,uint64_t now,
    const unsigned char build[32],Kh02StartupInstall,void *context);
unsigned Kh02StartupPath(Kh02StartupGate*,uint64_t created,
    const unsigned char binding[32],void *owner,Kh02PathText*,const wchar_t*,
    Kh02PathOriginal,Kh02PathFormat,Kh02PathText**);
#endif
