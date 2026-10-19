#ifndef KH02_STAT_POPUP_V111_H
#define KH02_STAT_POPUP_V111_H
#include <stdint.h>
#define KH02_STAT_POPUP_CAPACITY 128u
typedef struct Kh02StatPopupQueueV111 {
    uint64_t epoch,cookie,staged_item,staged_epoch,staged_cookie;
    uint32_t head,count,staged_count,reserved;
    uint64_t admitted,submitted,stale,overflow,failed;
    uint32_t kinds[KH02_STAT_POPUP_CAPACITY];
} Kh02StatPopupQueueV111;
void Kh02StatPopupBeginV111(Kh02StatPopupQueueV111*);
void Kh02StatPopupStageV111(Kh02StatPopupQueueV111*,uint64_t,uint64_t,uint64_t);
void Kh02StatPopupFinishV111(Kh02StatPopupQueueV111*,uint32_t,uint64_t,uint64_t);
uint32_t Kh02StatPopupPeekV111(Kh02StatPopupQueueV111*,uint64_t,uint64_t);
void Kh02StatPopupConsumeV111(Kh02StatPopupQueueV111*);
#endif
