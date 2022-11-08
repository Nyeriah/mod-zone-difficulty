#ifndef DEF_ZONEDIFFICULTY_H
#define DEF_ZONEDIFFICULTY_H

#include "Player.h"
#include "Config.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"

struct ZoneDifficultyData
{
    float HealingNerfPct;
    float AbsorbNerfPct;
    float SpellDamageBuffPct;
    float MeleeDamageBuffPct;
    bool Enabled;
};

int DUEL_INDEX = 0xFFFFFFFF

class ZoneDifficulty
{
public:
    static ZoneDifficulty* instance();

    void LoadMapDifficultySettings();
    [[nondiscard]] bool IsValidNerfTarget(Unit* unit);

    bool IsEnabled{ false };
    bool IsDebugInfoEnabled{ false };

    typedef std::map<uint32, ZoneDifficultyData> ZoneDifficultyDataMap;
    ZoneDifficultyDataMap ZoneDifficultyInfo;
    std::map<uint32, float> SpellNerfOverrides;

};

#define sZoneDifficulty ZoneDifficulty::instance()

#endif
