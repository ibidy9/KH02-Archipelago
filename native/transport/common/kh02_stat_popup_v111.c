#include "kh02_stat_popup_v111.h"
void Kh02StatPopupBeginV111(Kh02StatPopupQueueV111 *q) {
    q->staged_item=q->staged_epoch=q->staged_cookie=0;q->staged_count=0;
}
void Kh02StatPopupStageV111(Kh02StatPopupQueueV111 *q,uint64_t item,uint64_t epoch,uint64_t cookie) {
    /* Only the actual new-identity callback may stage, never history/projection. */
    if(item!=0x02020b && item!=0x02020c)return;
    if(q->staged_count<2)q->staged_count++;
    q->staged_item=item;q->staged_epoch=epoch;q->staged_cookie=cookie;
}
static void current(Kh02StatPopupQueueV111 *q,uint64_t epoch,uint64_t cookie) {
    if(q->epoch==epoch && q->cookie==cookie)return;
    q->stale+=q->count;q->head=q->count=0;q->epoch=epoch;q->cookie=cookie;
}
void Kh02StatPopupFinishV111(Kh02StatPopupQueueV111 *q,uint32_t success,uint64_t epoch,uint64_t cookie) {
    current(q,epoch,cookie);
    if(q->staged_count) {
        if(!success || !epoch || !cookie || q->staged_count!=1 ||
           q->staged_epoch!=epoch || q->staged_cookie!=cookie)q->failed++;
        else if(q->count==KH02_STAT_POPUP_CAPACITY)q->overflow++;
        else {
            q->kinds[(q->head+q->count)%KH02_STAT_POPUP_CAPACITY]=q->staged_item==0x02020b?4:5;
            q->count++;q->admitted++;
        }
    }
    Kh02StatPopupBeginV111(q);
}
uint32_t Kh02StatPopupPeekV111(Kh02StatPopupQueueV111 *q,uint64_t epoch,uint64_t cookie) {
    current(q,epoch,cookie);
    return epoch && cookie && q->count?q->kinds[q->head]:0;
}
void Kh02StatPopupConsumeV111(Kh02StatPopupQueueV111 *q) {
    /* Consume before native submission: never replay an uncertain UI call. */
    if(!q->count)return;
    q->head=(q->head+1)%KH02_STAT_POPUP_CAPACITY;q->count--;q->submitted++;
}
