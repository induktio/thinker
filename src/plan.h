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
        return score > obj.score || (score == obj.score && item_id > obj.item_id);;
    }
};

typedef std::pair<int, int> ipair_t;
typedef std::priority_queue<SItem, std::vector<SItem>, std::less<SItem>> score_max_queue_t;
typedef std::priority_queue<SItem, std::vector<SItem>, std::greater<SItem>> score_min_queue_t;


extern int plan_upkeep_turn;

int facility_score(FacilityId item_id, WItem& Wgov);
void governor_priorities(BASE& base, WItem& Wgov);
void reset_state();
void design_units(int faction_id);
void former_plans(int faction_id);
void plans_upkeep(int faction_id);
bool need_police(int faction_id);
int psi_score(int faction_id);
int satellite_goal(int faction_id, int item_id);
int satellite_count(int faction_id, int item_id);
