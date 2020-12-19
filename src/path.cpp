
#include "path.h"

CPath* Path = (CPath*)0x945B00;
FPath_find Path_find = (FPath_find)0x59A530;

int CPath::find(int x1, int y1, int x2, int y2, int unit_id, int faction) {
    return Path_find(this, x1, y1, x2, y2, unit_id, faction, 0, -1);
}

int path_distance(int x1, int y1, int x2, int y2, int unit_id, int faction) {
    Points visited;
    int px = x1;
    int py = y1;
    int dist = 0;
    int val = 0;
    memset(pm_overlay, 0, sizeof(pm_overlay));

    while (dist < MaxMapH && val >= 0) {
        pm_overlay[px][py]++;
        if (px == x2 && py == y2) {
            return dist;
        }
        val = Path_find(Path, px, py, x2, y2, unit_id, faction, 0, -1);
        if (val >= 0) {
            px = wrap(px + BaseOffsetX[val]);
            py = py + BaseOffsetY[val];
        }
        dist++;
        debug("path %2d %2d -> %2d %2d / %2d %2d / %2d\n", x1,y1,x2,y2,px,py,dist);
    }
    fflush(debug_log);
    return -1;
}

int path_get_next(int x1, int y1, int x2, int y2, int unit_id, int faction) {
    return Path_find(Path, x1, y1, x2, y2, unit_id, faction, 0, -1);
}
