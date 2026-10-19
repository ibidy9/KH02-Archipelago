#include "startup_input.h"
#include <string.h>
static uint32_t le32(const unsigned char *p) {
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static uint64_t le64(const unsigned char *p){return le32(p)|((uint64_t)le32(p+4)<<32);}
static BOOL decode_text(const unsigned char *p,wchar_t *out) {
    unsigned ended=0;
    for(unsigned i=0;i<KH02_PATH_ROOT_CAP;i++) {
        unsigned ch=p[2*i]|((unsigned)p[2*i+1]<<8);
        if(ended && ch)return FALSE;
        out[i]=(wchar_t)ch;if(!ch)ended=1;
    }
    return ended!=0;
}
unsigned Kh02ReadStartupInput(const wchar_t *path,Kh02StartupRequest *out,HANDLE *retained) {
    unsigned char bytes[KH02_READY_BYTES];DWORD got=0;LARGE_INTEGER size;
    *retained=NULL;
    HANDLE h=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,NULL);
    if(h==INVALID_HANDLE_VALUE)return GetLastError()==ERROR_FILE_NOT_FOUND?KH02_LAB_NO_INTENT:KH02_LAB_INTENT_ERROR;
    BY_HANDLE_FILE_INFORMATION info;
    BOOL ok=GetFileInformationByHandle(h,&info) &&
        !(info.dwFileAttributes&(FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DIRECTORY)) &&
        GetFileSizeEx(h,&size) && size.QuadPart==KH02_READY_BYTES &&
        ReadFile(h,bytes,sizeof(bytes),&got,NULL) && got==sizeof(bytes);
    if(!ok)goto failed;
    if(memcmp(bytes,"KH02AP01",8) || le32(bytes+8)!=1 || le32(bytes+12)!=KH02_READY_BYTES || le32(bytes+132))goto failed;
    memset(out,0,sizeof(*out));out->version=1;out->size=sizeof(*out);
    out->prepared=le64(bytes+16);out->expires=le64(bytes+24);
    memcpy(out->build,bytes+32,32);memcpy(out->binding,bytes+64,32);memcpy(out->nonce,bytes+96,32);
    out->root_volume=le32(bytes+128);out->root_file=le64(bytes+136);
    if(!decode_text(bytes+144,out->root) || !decode_text(bytes+624,out->claim_directory))goto failed;
    *retained=h;return KH02_LAB_AP_INTENT;
failed:
    CloseHandle(h);return KH02_LAB_INTENT_ERROR;
}

typedef struct Hash {uint32_t h[8];uint64_t length;unsigned used;unsigned char block[64];} Hash;
static uint32_t rotate(uint32_t x,unsigned n){return (x>>n)|(x<<(32-n));}
static void compress(Hash *s,const unsigned char *p) {
    static const uint32_t k[64]={
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
    uint32_t w[64];
    for(unsigned i=0;i<16;i++)w[i]=((uint32_t)p[i*4]<<24)|((uint32_t)p[i*4+1]<<16)|((uint32_t)p[i*4+2]<<8)|p[i*4+3];
    for(unsigned i=16;i<64;i++) {
        uint32_t x=w[i-15],y=w[i-2];
        w[i]=w[i-16]+(rotate(x,7)^rotate(x,18)^(x>>3))+w[i-7]+(rotate(y,17)^rotate(y,19)^(y>>10));
    }
    uint32_t a=s->h[0],b=s->h[1],c=s->h[2],d=s->h[3],e=s->h[4],f=s->h[5],g=s->h[6],h=s->h[7];
    for(unsigned i=0;i<64;i++) {
        uint32_t t=h+(rotate(e,6)^rotate(e,11)^rotate(e,25))+((e&f)^((~e)&g))+k[i]+w[i];
        uint32_t u=(rotate(a,2)^rotate(a,13)^rotate(a,22))+((a&b)^(a&c)^(b&c));
        h=g;g=f;f=e;e=d+t;d=c;c=b;b=a;a=t+u;
    }
    s->h[0]+=a;s->h[1]+=b;s->h[2]+=c;s->h[3]+=d;s->h[4]+=e;s->h[5]+=f;s->h[6]+=g;s->h[7]+=h;
}
static void update(Hash *s,const unsigned char *p,unsigned n) {
    s->length+=n;
    while(n) {
        unsigned take=64-s->used;if(take>n)take=n;
        memcpy(s->block+s->used,p,take);s->used+=take;p+=take;n-=take;
        if(s->used==64){compress(s,s->block);s->used=0;}
    }
}
BOOL Kh02HashStartupFile(const wchar_t *path,unsigned char digest[32],HANDLE *retained) {
    *retained=NULL;
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_SEQUENTIAL_SCAN,NULL);
    if(file==INVALID_HANDLE_VALUE)return FALSE;
    BY_HANDLE_FILE_INFORMATION info;LARGE_INTEGER size;
    if(!GetFileInformationByHandle(file,&info) || (info.dwFileAttributes&(FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DIRECTORY)) ||
       !GetFileSizeEx(file,&size) || size.QuadPart<0 || size.QuadPart>256*1024*1024)goto failed;
    Hash s={{0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19},0,0,{0}};
    unsigned char buffer[16384];DWORD got;
    for(;;) {
        if(!ReadFile(file,buffer,sizeof(buffer),&got,NULL))goto failed;
        if(!got)break;update(&s,buffer,got);
    }
    if(s.length!=(uint64_t)size.QuadPart)goto failed;
    uint64_t bits=s.length*8;unsigned char end[72]={0x80};
    unsigned pad=s.used<56?56-s.used:120-s.used;
    update(&s,end,pad);
    for(unsigned i=0;i<8;i++)end[i]=(unsigned char)(bits>>(56-i*8));
    update(&s,end,8);
    for(unsigned i=0;i<32;i++)digest[i]=(unsigned char)(s.h[i/4]>>(24-(i%4)*8));
    *retained=file;return TRUE;
failed:
    CloseHandle(file);return FALSE;
}
