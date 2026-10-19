#ifndef KH02_STARTUP_INPUT_H
#define KH02_STARTUP_INPUT_H
#include "startup_gate.h"
#define KH02_READY_BYTES 1104
/* Only ERROR_FILE_NOT_FOUND at the exact intent filename means vanilla.
   Successful reads retain a non-writable, non-deletable handle until exit. */
unsigned Kh02ReadStartupInput(const wchar_t*,Kh02StartupRequest*,HANDLE*);
/* Streaming file SHA-256 using local arithmetic; no provider loading in DllMain. */
BOOL Kh02HashStartupFile(const wchar_t*,unsigned char digest[32],HANDLE*);
#endif
