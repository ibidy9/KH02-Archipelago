#ifndef KH02_CUSTOM_POPUP_ARGS_V115_H
#define KH02_CUSTOM_POPUP_ARGS_V115_H
#include <stdint.h>
/* Supported x64 native FASValue layout. Borrowed strings: never native-free
   these input descriptors. Keep their backing storage alive through dispatch. */
typedef struct Kh02PopupStringV115 {const uint16_t *data;int32_t count,capacity;} Kh02PopupStringV115;
typedef struct Kh02PopupValueV115 {
    unsigned char type,boolean,pad[2];int32_t integer;float real;uint32_t reserved;
    Kh02PopupStringV115 text;
    const char *narrow;int32_t narrow_count,narrow_capacity;uint64_t object;
} Kh02PopupValueV115;
typedef struct Kh02PopupArgsV115 {Kh02PopupValueV115 values[3];} Kh02PopupArgsV115;
uint32_t Kh02CustomPopupArgsV115(Kh02PopupArgsV115*,const uint16_t*,uint32_t,const uint16_t*,uint32_t);
typedef int (*Kh02PopupReadV115)(void*,uint64_t,void*,uint32_t);
uint32_t Kh02PopupNativeIconV115(Kh02PopupReadV115,void*,uint64_t,uint16_t[261]);
typedef struct Kh02NoticeV115 {
 uint64_t magic,id,epoch; uint32_t length,reserved; uint16_t text[161]; uint16_t padding[3];
} Kh02NoticeV115;
_Static_assert(sizeof(Kh02NoticeV115)==360,"notice wire size");
#endif
