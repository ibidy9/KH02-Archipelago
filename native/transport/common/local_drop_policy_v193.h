/* Pure bounded encounter admission. An epoch change discards pending rewards.
 * Optional Darksides are one victory per named group per lifecycle epoch.
 * The observed replay boundary is reload; no same-epoch respawn assumption. */
#ifndef KH02_LOCAL_DROP_POLICY193
#define KH02_LOCAL_DROP_POLICY193
#include <stdint.h>
#include <string.h>
typedef struct LocalDropEvent193 {
 uint64_t cookie,epoch,serial;
 uint32_t group,battle;
} LocalDropEvent193;
typedef struct LocalDropPolicy193 {
 uint64_t cookie,epoch,accepted,duplicates,expired;
 uint32_t mask,count;
 LocalDropEvent193 pending[14];
} LocalDropPolicy193;
static void local_drop_epoch193(LocalDropPolicy193 *p,uint64_t cookie,uint64_t epoch) {
 if(p->cookie==cookie && p->epoch==epoch)return;
 p->expired+=p->count;p->count=0;p->mask=0;p->cookie=cookie;p->epoch=epoch;
}
static int local_drop_admit193(LocalDropPolicy193 *p,uint64_t cookie,uint64_t epoch,
 uint64_t serial,uint32_t group) {
 if(!cookie || !epoch || !serial || (!group || group>8192 || (group&(group-1))))return 0;
 local_drop_epoch193(p,cookie,epoch);
 if(p->mask&group){p->duplicates++;return 0;}
 if(p->count>=14)return 0;
 /* Alternation is test policy; first Potion, second Ether. */
 p->pending[p->count++]=(LocalDropEvent193){cookie,epoch,serial,group,(p->accepted&1)?3u:0u};
 p->accepted++;p->mask|=group;return 1;
}
static void local_drop_travel195(LocalDropPolicy193 *p) {
 p->expired+=p->count;p->count=0;p->mask=0;
}
static void local_drop_rush_travel200(LocalDropPolicy193 *p) {
 uint32_t count=0;
 for(uint32_t i=0;i<p->count;i++) {
  if(p->pending[i].group>=1024)p->pending[count++]=p->pending[i];
  else p->expired++;
 }
 p->count=count;p->mask&=0x3c00u;
}
static int local_drop_take193(LocalDropPolicy193 *p,LocalDropEvent193 *out) {
 if(!p->count)return 0;
 *out=p->pending[0];p->count--;
 memmove(p->pending,p->pending+1,p->count*sizeof(*p->pending));
 return 1;
}
#endif
