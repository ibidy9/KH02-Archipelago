/* Bounded startup ABI: at most the 100 records present in one save container. */
#ifndef KH02_RESUME_BOUNDS_V254
#define KH02_RESUME_BOUNDS_V254
#define KH02_RESUME_RECORDS_V254 100u
#define KH02_SETUP_CAP_V254 (128u+4096u+4096u*32u+1023u+32u+KH02_RESUME_RECORDS_V254*(8u+0x3b40u+102456u))
#endif
