#ifndef KH02_MENU_STORY_PAIR_V157_H
#define KH02_MENU_STORY_PAIR_V157_H
#include <stdint.h>
#include <string.h>
/* Source-world checkpoint lag observed live; independent of destination.
 * Callers must still validate both maps, stable snapshots and ownership. */
static int menu_story_pair157(const uint32_t story[2],const char map[32]) {
    if(story[0]==story[1])return 1;
    if(!memcmp(map,"/Game/Levels/dw/dw_09/dw_09",26) && story[0]==410 && story[1]==400)return 1;
    if(!memcmp(map,"/Game/Levels/dw/dw_02/dw_02",26) && story[0]>=120 && story[0]<=150 && !(story[0]%10) &&
       (story[1]==70 || (story[1]>=110 && story[1]<story[0] && !(story[1]%10))))return 1;
    return (story[0]==210 && story[1]==200 && !memcmp(map,"/Game/Levels/dw/dw_03/dw_03",26)) ||
           (story[0]==350 && story[1]==340 && !memcmp(map,"/Game/Levels/dw/dw_08/dw_08",26)) ||
           /* Archived natural rail landing: rocky-path-appeared a323/b322. */
           (story[0]==323 && story[1]==322 && !memcmp(map,"/Game/Levels/dw/dw_08/dw_08",26)) ||
           (story[0]==300 && story[1]==280 && !memcmp(map,"/Game/Levels/dw/dw_08/dw_08",26)) ||
           (story[0]==110 && story[1]==70 && !memcmp(map,"/Game/Levels/dw/dw_02/dw_02",26));
}
#endif
