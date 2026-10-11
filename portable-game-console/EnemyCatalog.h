#pragma once

struct EnemyData {
    int id;
    const char* name;
    int dropItemId;
    int dropRatePercent;
};

class EnemyCatalog {
public:
    static const EnemyData& get(int id);
    static int xpYieldFor(int id, int level);
};
