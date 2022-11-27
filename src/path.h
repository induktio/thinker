#pragma once

#include "main.h"

typedef int(__thiscall *FPath_find)(
    void* This, int x1, int y1, int x2, int y2, int unit_id, int faction, int flags, int unk1);


class CPath {
public:
    int* mapTable;
    int16_t* xTable;
    int16_t* yTable;
    int index1; // specific territory count
    int index2; // overall territory count
    int factionID1;
    int xDst;
    int yDst;
    int field_20;
    int factionID2;
    int protoID;

    int find(int x1, int y1, int x2, int y2, int unit_id, int faction);
};

extern CPath* Path;

int path_distance(int x1, int y1, int x2, int y2, int unit_id, int faction);
int path_get_next(int x1, int y1, int x2, int y2, int unit_id, int faction);

