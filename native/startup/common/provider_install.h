#ifndef KH02_PROVIDER_INSTALL_H
#define KH02_PROVIDER_INSTALL_H
#include <windows.h>
#include <stdint.h>
enum { KH02_TABLE_NEW,KH02_TABLE_INSTALLED,KH02_TABLE_REFUSED,
       KH02_TABLE_ROLLED_BACK,KH02_TABLE_UNSAFE };
typedef struct Kh02ProviderInstall {
    unsigned status,changed;
    DWORD original_protection;
    void *original[9];
} Kh02ProviderInstall;
/* Injectable operating-system boundary for fault tests. NULL selects real APIs. */
typedef struct Kh02TableOps {
    BOOL (*protect)(void*,SIZE_T,DWORD,DWORD*);
    void *(*exchange)(void *volatile*,void*,void*);
} Kh02TableOps;
/* Early startup only, before any game callbacks. Hash verification is a caller
   precondition; the installer additionally checks the image and all nine cells.
   replacements order: existence, save, load, operation28, filename.
   No retry, runtime uninstall, or cloned object/vtable is exposed. */
BOOL Kh02InstallProvider(Kh02ProviderInstall*,unsigned char *base,
                        void *const replacements[5],const Kh02TableOps*);
#endif
