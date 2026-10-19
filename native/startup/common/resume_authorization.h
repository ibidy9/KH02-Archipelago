#ifndef KH02_RESUME_AUTHORIZATION_H
#define KH02_RESUME_AUTHORIZATION_H
#include "startup_input.h"
#define KH02_RESUME_BYTES 144
typedef struct Kh02ResumeAuthorization {
    unsigned char bytes[KH02_RESUME_BYTES];
    HANDLE input,file;
} Kh02ResumeAuthorization;
/* Missing is fresh-only. Any explicit invalid authorization blocks startup. */
unsigned Kh02ReadResumeAuthorization(const wchar_t*,const Kh02StartupRequest*,Kh02ResumeAuthorization*);
BOOL Kh02CheckResumeRoot(const Kh02StartupRequest*,void*);
void Kh02ReleaseResumeFile(Kh02ResumeAuthorization*);
#endif
