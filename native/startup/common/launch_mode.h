/* Offline policy kernel; does not authorize any real game/file operation. */
#ifndef KH02_LAB_LAUNCH_MODE_H
#define KH02_LAB_LAUNCH_MODE_H
#include <windows.h>
#include <stdint.h>
enum { KH02_LAB_UNSET, KH02_LAB_VANILLA, KH02_LAB_BLOCKED, KH02_LAB_AP };
enum { KH02_LAB_NO_INTENT, KH02_LAB_AP_INTENT, KH02_LAB_INTENT_ERROR };
typedef struct Kh02LabLaunchMode {
    SRWLOCK lock;
    unsigned mode;
    uint64_t process_created;
    unsigned char binding[32];
} Kh02LabLaunchMode;
/* Initialize zeroed storage before exposing it to any thread. Never reset it. */
unsigned Kh02LabChoose(Kh02LabLaunchMode*,unsigned intent,unsigned validated,
                      uint64_t created,const unsigned char binding[32]);
unsigned Kh02LabCheck(Kh02LabLaunchMode*,uint64_t created,
                     const unsigned char binding[32]);
#endif
