#ifndef KH02_CRITICAL_MENU_STARTUP_H
#define KH02_CRITICAL_MENU_STARTUP_H
#include <stdint.h>
typedef uint32_t (*Kh02StartupCriticalPermit)(void *context);
typedef struct Kh02StartupCriticalEvidence {
    volatile long installed,calls,permitted,changed,last_before,last_after;
} Kh02StartupCriticalEvidence;
uint32_t Kh02StartupCriticalInstall(unsigned char *image,Kh02StartupCriticalPermit permit,
    void *context,Kh02StartupCriticalEvidence *evidence);
uint32_t Kh02StartupCriticalRemove(void);
#ifdef KH02_STARTUP_CRITICAL_SELFTEST
__declspec(dllexport) uint32_t Kh02StartupCriticalTestInstall(void *target,uint32_t allowed,
    Kh02StartupCriticalEvidence *evidence);
__declspec(dllexport) void Kh02StartupCriticalTestPermit(uint32_t allowed);
__declspec(dllexport) uint32_t Kh02StartupCriticalTestRemove(void);
#endif
#endif
