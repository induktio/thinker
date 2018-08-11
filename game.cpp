
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


