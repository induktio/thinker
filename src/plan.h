#pragma once

#include "main.h"

struct SItem {
    int item_id;
    int score;

    bool operator<(SItem const& obj) const
    {
        return score < obj.score || (score == obj.score && item_id < obj.item_id);
    }
    bool operator>(SItem const& obj) const
    {
        return score > obj.score || (score == obj.score && item_id > obj.item_id);
    }
};

struct MItem {
    int x;
    int y;
    int score;

    bool operator<(MItem const& obj) const
    {
        return score < obj.score || (score == obj.score && (x < obj.x || (x == obj.x && y < obj.y)));
    }
    bool operator>(MItem const& obj) const
    {
        return score > obj.score || (score == obj.score && (x > obj.x || (x == obj.x && y > obj.y)));
    }
};

typedef std::priority_queue<SItem, std::vector<SItem>, std::less<SItem>> score_max_queue_t;
typedef std::priority_queue<SItem, std::vector<SItem>, std::greater<SItem>> score_min_queue_t;
typedef std::priority_queue<MItem, std::vector<MItem>, std::less<MItem>> point_max_queue_t;
typedef std::priority_queue<MItem, std::vector<MItem>, std::greater<MItem>> point_min_queue_t;

extern int plan_upkeep_turn;
extern int move_upkeep_faction;

int facility_score(FacilityId item_id, WItem& Wgov);
void governor_priorities(BASE& base, WItem& Wgov);
void reset_state();
void design_units(int faction_id);
void former_plans(int faction_id);
void plans_upkeep(int faction_id);
bool need_police(int faction_id);
int faction_might(int faction_id);
int compare_might(int faction_id, int faction_id_tgt);
int psi_score(int faction_id);
int satellite_goal(int faction_id, int item_id);
int satellite_count(int faction_id, int item_id);
