#ifndef KH02_FILENAME_EVIDENCE_H
#define KH02_FILENAME_EVIDENCE_H
#include <windows.h>
#include <stdint.h>
#include "critical_menu_startup.h"
typedef struct FilenameEvent {
    LONG complete;DWORD thread;uint64_t caller;
    wchar_t name[12],path[256];
} FilenameEvent;
typedef struct FilenameEvidence {
    uint32_t magic,version,size,mode,startup_error,install_status;
    LONG count,overflow,flight,fault;
    uint64_t created;
    unsigned char binding[32];wchar_t root[240];
    Kh02StartupCriticalEvidence critical_menu;
    FilenameEvent events[32];
} FilenameEvidence;
#endif
