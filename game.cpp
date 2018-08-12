
#include "game.h"


char* prod_name(int prod) {
    if (prod < 0)
        return (char*)(tx_facilities[-prod].name);
    else
        return (char*)&(tx_units[prod].name);
}

bool knows_tech(int fac, int tech) {
    return tx_tech_discovered[tech] & (1 << fac);
}

int unit_triad(int id) {
    return tx_chassis[tx_units[id].chassis_type].triad;
}

int random(int n) {
    return rand() % (n != 0 ? n : 1);
}

int wrap(int a, int b) {
    return (a < 0 ? a + b : a);
}

int map_range(int x1, int y1, int x2, int y2) {
    int xd = abs(x1-x2);
    int yd = abs(y1-y2);
    if (xd > *tx_map_axis_x/2)
        xd = *tx_map_axis_x - xd;
    return (xd + yd)/2;
}

MAP* mapsq(int x, int y) {
    int i = x/2 + (*tx_map_half_x) * y;
    if (i >= 0 && i < *tx_map_half_x * *tx_map_axis_y)
        return &((*tx_map_ptr)[i]);
    else
        return NULL;
}

void TileSearch::init(int x, int y, int tp) {
    head = 0;
    tail = 0;
    items = 0;
    type = tp;
    oldtiles.clear();
    tile = mapsq(x, y);
    if (tile) {
        items++;
        int p = x | y << 16;
        oldtiles.insert(p);
        newtiles[0] = p;
    }
}

int TileSearch::visited() {
    return oldtiles.size();
}

MAP* TileSearch::get_next() {
    while (items > 0) {
        int p = newtiles[head];
        head = (head + 1) % QSIZE;
        items--;
        cur_x = p & 0xffff;
        cur_y = p >> 16;
        if (!(tile = mapsq(cur_x, cur_y)))
            continue;
        bool skip = (type == LAND_ONLY && tile->altitude < ALTITUDE_MIN_LAND) ||
                    (type == WATER_ONLY && tile->altitude >= ALTITUDE_MIN_LAND);
        if (oldtiles.size() > 1 && skip)
            continue;
        for (int i=0; i<16; i+=2) {
            int x2 = wrap(cur_x + offset[i], *tx_map_axis_x);
            int y2 = cur_y + offset[i+1];
            p = x2 | y2 << 16;
            if (items < QSIZE && oldtiles.count(p) == 0 && y2 >= 0 && y2 < *tx_map_axis_x) {
                newtiles[tail] = p;
                tail = (tail + 1) % QSIZE;
                items++;
                oldtiles.insert(p);
            }
        }
        return tile;
    }
    return NULL;
}



