/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "Config.h"
#include "Chat.h"
#include "GameTime.h"
#include "ItemTemplate.h"
#include "MapMgr.h"
#include "Pet.h"
#include "Player.h"
#include "PoolMgr.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "StringConvert.h"
#include "TaskScheduler.h"
#include "Tokenize.h"
#include "Unit.h"
#include "ZoneDifficulty.h"

ZoneDifficulty* ZoneDifficulty::instance()
{
    static ZoneDifficulty instance;
    return &instance;
}

void ZoneDifficulty::LoadMapDifficultySettings()
{
    if (!sZoneDifficulty->IsEnabled)
    {
        return;
    }

    sZoneDifficulty->Rewards.clear();
    sZoneDifficulty->HardmodeAI.clear();
    sZoneDifficulty->CreatureOverrides.clear();
    sZoneDifficulty->DailyHeroicQuests.clear();
    sZoneDifficulty->HardmodeLoot.clear();
    sZoneDifficulty->DisallowedBuffs.clear();
    sZoneDifficulty->SpellNerfOverrides.clear();
    sZoneDifficulty->NerfInfo.clear();

    // Default values for when there is no entry in the db for duels (index 0xFFFFFFFF)
    NerfInfo[DUEL_INDEX][0].HealingNerfPct = 1;
    NerfInfo[DUEL_INDEX][0].AbsorbNerfPct = 1;
    NerfInfo[DUEL_INDEX][0].MeleeDamageBuffPct = 1;
    NerfInfo[DUEL_INDEX][0].SpellDamageBuffPct = 1;

    // Heroic Quest -> MapId Translation
    HeroicTBCQuestMapList[542] = 11362; // Blood Furnace
    HeroicTBCQuestMapList[543] = 11354; // Hellfire Ramparts
    HeroicTBCQuestMapList[547] = 11368; // Slave Pens
    HeroicTBCQuestMapList[546] = 11369; // The Underbog
    HeroicTBCQuestMapList[557] = 11373; // Mana-Tombs
    HeroicTBCQuestMapList[558] = 11374; // Auchenai Crypts
    HeroicTBCQuestMapList[560] = 11378; // The Escape From Durnholde
    HeroicTBCQuestMapList[556] = 11372; // Sethekk Halls
    HeroicTBCQuestMapList[585] = 11499; // Magisters' Terrace
    HeroicTBCQuestMapList[555] = 11375; // Shadow Labyrinth
    HeroicTBCQuestMapList[540] = 11363; // Shattered Halls
    HeroicTBCQuestMapList[552] = 11388; // The Arcatraz
    HeroicTBCQuestMapList[269] = 11382; // The Black Morass
    HeroicTBCQuestMapList[553] = 11384; // The Botanica
    HeroicTBCQuestMapList[554] = 11386; // The Mechanar
    HeroicTBCQuestMapList[545] = 11370; // The Steamvault

    // Icons
    sZoneDifficulty->ItemIcons[ITEMTYPE_MISC] = "|TInterface\\icons\\inv_misc_cape_17:15|t |TInterface\\icons\\inv_misc_gem_topaz_02:15|t |TInterface\\icons\\inv_jewelry_ring_51naxxramas:15|t ";
    sZoneDifficulty->ItemIcons[ITEMTYPE_CLOTH] = "|TInterface\\icons\\inv_chest_cloth_42:15|t ";
    sZoneDifficulty->ItemIcons[ITEMTYPE_LEATHER] = "|TInterface\\icons\\inv_helmet_41:15|t ";
    sZoneDifficulty->ItemIcons[ITEMTYPE_MAIL] = "|TInterface\\icons\\inv_chest_chain_13:15|t ";
    sZoneDifficulty->ItemIcons[ITEMTYPE_PLATE] = "|TInterface\\icons\\inv_chest_plate12:15|t ";
    sZoneDifficulty->ItemIcons[ITEMTYPE_WEAPONS] = "|TInterface\\icons\\inv_mace_25:15|t |TInterface\\icons\\inv_shield_27:15|t |TInterface\\icons\\inv_weapon_crossbow_04:15|t ";

    if (QueryResult result = WorldDatabase.Query("SELECT * FROM zone_difficulty_info"))
    {
        do
        {
            uint32 mapId = (*result)[0].Get<uint32>();
            uint32 phaseMask = (*result)[1].Get<uint32>();
            ZoneDifficultyNerfData data;
            int8 mode = (*result)[6].Get<int8>();
            if (sZoneDifficulty->HasNormalMode(mode))
            {
                data.HealingNerfPct = (*result)[2].Get<float>();
                data.AbsorbNerfPct = (*result)[3].Get<float>();
                data.MeleeDamageBuffPct = (*result)[4].Get<float>();
                data.SpellDamageBuffPct = (*result)[5].Get<float>();
                data.Enabled = data.Enabled | mode;
                sZoneDifficulty->NerfInfo[mapId][phaseMask] = data;
            }
            if (sZoneDifficulty->HasHardMode(mode))
            {
                data.HealingNerfPctHard = (*result)[2].Get<float>();
                data.AbsorbNerfPctHard = (*result)[3].Get<float>();
                data.MeleeDamageBuffPctHard = (*result)[4].Get<float>();
                data.SpellDamageBuffPctHard = (*result)[5].Get<float>();
                data.Enabled = data.Enabled | mode;
                sZoneDifficulty->NerfInfo[mapId][phaseMask] = data;
            }
            if ((mode & MODE_HARD) != MODE_HARD && (mode & MODE_NORMAL) != MODE_NORMAL)
            {
                LOG_ERROR("module", "MOD-ZONE-DIFFICULTY: Invalid mode {} used in Enabled for mapId {}, ignored.", mode, mapId);
            }

            // duels do not check for phases. Only 0 is allowed.
            if (mapId == DUEL_INDEX && phaseMask != 0)
            {
                LOG_ERROR("module", "MOD-ZONE-DIFFICULTY: Table `zone_difficulty_info` for criteria (duel mapId: {}) has wrong value ({}), must be 0 for duels.", mapId, phaseMask);
            }

        } while (result->NextRow());
    }

    if (QueryResult result = WorldDatabase.Query("SELECT * FROM zone_difficulty_spelloverrides"))
    {
        do
        {
            if ((*result)[2].Get<bool>())
            {
                sZoneDifficulty->SpellNerfOverrides[(*result)[0].Get<uint32>()] = (*result)[1].Get<float>();
            }

        } while (result->NextRow());
    }

    if (QueryResult result = WorldDatabase.Query("SELECT * FROM zone_difficulty_disallowed_buffs"))
    {
        do
        {
            std::vector<uint32> debuffs;
            uint32 mapId;
            if ((*result)[2].Get<bool>())
            {
                std::string spellString = (*result)[1].Get<std::string>();
                std::vector<std::string_view> tokens = Acore::Tokenize(spellString, ' ', false);

                mapId = (*result)[0].Get<uint32>();
                for (auto token : tokens)
                {
                    if (token.empty())
                    {
                        continue;
                    }

                    uint32 spell;
                    if ((spell = Acore::StringTo<uint32>(token).value()))
                    {
                        debuffs.push_back(spell);
                    }
                    else
                    {
                        LOG_ERROR("module", "MOD-ZONE-DIFFICULTY: Disabling buffs for spell '{}' is invalid, skipped.", spell);
                    }
                }
                sZoneDifficulty->DisallowedBuffs[mapId] = debuffs;
            }
        } while (result->NextRow());
    }

    if (QueryResult result = WorldDatabase.Query("SELECT * FROM zone_difficulty_hardmode_instance_data"))
    {
        do
        {
            ZoneDifficultyHardmodeMapData data;
            uint32 MapID = (*result)[0].Get<uint32>();
            data.EncounterEntry = (*result)[1].Get<uint32>();
            data.OverrideGO = (*result)[2].Get<uint32>();
            data.RewardType = (*result)[3].Get<uint8>();

            sZoneDifficulty->HardmodeLoot[MapID].push_back(data);
            //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: New creature for map {} with entry: {}", MapID, data.EncounterEntry);

            Expansion[MapID] = data.RewardType;

        } while (result->NextRow());
    }
    else
    {
        LOG_ERROR("module", "MOD-ZONE-DIFFICULTY: Query failed: SELECT * FROM zone_difficulty_hardmode_instance_data");
    }

    if (QueryResult result = WorldDatabase.Query("SELECT entry FROM `pool_quest` WHERE `pool_entry`=356"))
    {
        do
        {
            sZoneDifficulty->DailyHeroicQuests.push_back((*result)[0].Get<uint32>());
            //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: Adding daily heroic quest with id {}.", (*result)[0].Get<uint32>());
        } while (result->NextRow());
    }
    else
    {
        LOG_ERROR("module", "MOD-ZONE-DIFFICULTY: Query failed: SELECT entry FROM `pool_quest` WHERE `pool_entry`=356");
    }

    if (QueryResult result = WorldDatabase.Query("SELECT * FROM zone_difficulty_hardmode_creatureoverrides"))
    {
        do
        {
            uint32 creatureEntry = (*result)[0].Get<uint32>();
            float hpModifier = (*result)[1].Get<float>();
            bool enabled = (*result)[2].Get<bool>();

            if (enabled)
            {
                if (hpModifier != 0)
                {
                    sZoneDifficulty->CreatureOverrides[creatureEntry] = hpModifier;
                }
                //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: New creature with entry: {} has exception for hp: {}", creatureEntry, hpModifier);
            }
        } while (result->NextRow());
    }
    else
    {
        LOG_ERROR("module", "MOD-ZONE-DIFFICULTY: Query failed: SELECT * FROM zone_difficulty_hardmode_creatureoverrides");
    }

    if (QueryResult result = WorldDatabase.Query("SELECT * FROM zone_difficulty_hardmode_ai"))
    {
        do
        {
            bool enabled = (*result)[8].Get<bool>();

            if (enabled)
            {
                uint32 creatureEntry = (*result)[0].Get<uint32>();
                ZoneDifficultyHAI data;
                data.Chance = (*result)[1].Get<uint8>();
                data.Spell = (*result)[2].Get<uint32>();
                data.Target = (*result)[3].Get<uint8>();
                data.TargetArg = (*result)[4].Get<uint8>();
                data.Delay = (*result)[5].Get<Milliseconds>();
                data.Cooldown = (*result)[6].Get<Milliseconds>();
                data.Repetitions = (*result)[7].Get<uint8>();

                if (data.Chance != 0 && data.Spell != 0 && ((data.Target >= 1 && data.Target <= 6) || data.Target == 18))
                {
                    sZoneDifficulty->HardmodeAI[creatureEntry].push_back(data);
                    LOG_INFO("module", "MOD-ZONE-DIFFICULTY: New AI for entry {} with spell {}", creatureEntry, data.Spell);
                }
                else
                {
                    LOG_ERROR("module", "MOD-ZONE-DIFFICULTY: Unknown type for `Target`: {} in zone_difficulty_hardmode_ai", data.Target);
                }
                //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: New creature with entry: {} has exception for hp: {}", creatureEntry, hpModifier);
            }
        } while (result->NextRow());
    }
    else
    {
        LOG_ERROR("module", "MOD-ZONE-DIFFICULTY: Query failed: SELECT * FROM zone_difficulty_hardmode_ai");
    }

    //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: Starting load of rewards.");
    if (QueryResult result = WorldDatabase.Query("SELECT ContentType, ItemType, Entry, Price, Enchant, EnchantSlot, Achievement, Enabled FROM zone_difficulty_hardmode_rewards"))
    {
        /* debug
         * uint32 i = 0;
         * end debug
         */
        do
        {
            /* debug
             * ++i;
             * end debug
             */
            ZoneDifficultyRewardData data;
            uint32 contentType = (*result)[0].Get<uint32>();
            uint32 itemType = (*result)[1].Get<uint32>();
            data.Entry = (*result)[2].Get<uint32>();
            data.Price = (*result)[3].Get<uint32>();
            data.Enchant = (*result)[4].Get<uint32>();
            data.EnchantSlot = (*result)[5].Get<uint8>();
            data.Achievement = (*result)[6].Get<uint32>();
            bool enabled = (*result)[7].Get<bool>();

            if (enabled)
            {
                sZoneDifficulty->Rewards[contentType][itemType].push_back(data);
                //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: Loading item with entry {} has enchant {} in slot {}. contentType: {} itemType: {}", data.Entry, data.Enchant, data.EnchantSlot, contentType, itemType);
            }
            //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: Total items in Rewards map: {}.", i);
        } while (result->NextRow());
    }
    else
    {
        LOG_ERROR("module", "MOD-ZONE-DIFFICULTY: Query failed: SELECT ContentType, ItemType, Entry, Price, Enchant, EnchantSlot, Achievement, Enabled FROM zone_difficulty_hardmode_rewards");
    }
}

/**
 *  @brief Loads the HardmodeInstanceData from the database. Fetch from zone_difficulty_instance_saves.
 *
 *  `InstanceID` INT NOT NULL DEFAULT 0,
 *  `HardmodeOn` TINYINT NOT NULL DEFAULT 0,
 *
 *  Exclude data not in the IDs stored in GetInstanceIDs() and delete
 *  zone_difficulty_instance_saves for instances that no longer exist.
 */
void ZoneDifficulty::LoadHardmodeInstanceData()
{
    std::vector<bool> instanceIDs = sMapMgr->GetInstanceIDs();
    /* debugging
    * for (int i = 0; i < int(instanceIDs.size()); i++)
    * {
    *   LOG_INFO("module", "MOD-ZONE-DIFFICULTY: ZoneDifficulty::LoadHardmodeInstanceData: id {} exists: {}:", i, instanceIDs[i]);
    * }
    * end debugging
    */
    if (QueryResult result = CharacterDatabase.Query("SELECT * FROM zone_difficulty_instance_saves"))
    {
        do
        {
            uint32 InstanceId = (*result)[0].Get<uint32>();
            bool HardmodeOn = (*result)[1].Get<bool>();

            if (instanceIDs[InstanceId])
            {
                LOG_INFO("module", "MOD-ZONE-DIFFICULTY: Loading from DB for instanceId {}: HardmodeOn = {}", InstanceId, HardmodeOn);
                sZoneDifficulty->HardmodeInstanceData[InstanceId] = HardmodeOn;
            }
            else
            {
                CharacterDatabase.Execute("DELETE FROM zone_difficulty_instance_saves WHERE InstanceID = {}", InstanceId);
            }


        } while (result->NextRow());
    }
}

/**
 *  @brief Loads the score data from the database.
 *  Fetch from zone_difficulty_hardmode_score.
 *
 *  `CharacterGuid` INT NOT NULL DEFAULT 0,
 *  `Type` TINYINT NOT NULL DEFAULT 0,
 *  `Score` INT NOT NULL DEFAULT 0,
 *
 */
void ZoneDifficulty::LoadHardmodeScoreData()
{
    if (QueryResult result = CharacterDatabase.Query("SELECT * FROM zone_difficulty_hardmode_score"))
    {
        do
        {
            uint32 GUID = (*result)[0].Get<uint32>();
            uint8 Type = (*result)[1].Get<uint8>();
            uint32 Score = (*result)[2].Get<uint32>();

            //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: Loading from DB for player with GUID {}: Type = {}, Score = {}", GUID, Type, Score);
            sZoneDifficulty->HardmodeScore[GUID][Type] = Score;

        } while (result->NextRow());
    }
}

/**
 *  @brief Sends a whisper to all members of the player's raid in the same instance as the creature.
 *
 *  @param message The message which should be sent to the <Player>.
 *  @param creature The creature who sends the whisper.
 *  @param player The object of the player, whose whole group should receive the message.
 */
void ZoneDifficulty::SendWhisperToRaid(std::string message, Creature* creature, Player* player)
{
    Group::MemberSlotList const& members = player->GetGroup()->GetMemberSlots();
    for (auto& member : members)
    {
        Player* mplr = ObjectAccessor::FindConnectedPlayer(member.guid);
        if (creature && mplr && mplr->GetMap()->GetInstanceId() == player->GetMap()->GetInstanceId())
        {
            //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: Player {} should receive a whisper.", mplr->GetName());
            creature->Whisper(message, LANG_UNIVERSAL, mplr);
        }
    }
}

std::string ZoneDifficulty::GetItemTypeString(uint32 type)
{
    std::string typestring;
    switch (type)
    {
    case ITEMTYPE_MISC:
        typestring = "Back, Finger, Neck, and Trinket";
        break;
    case ITEMTYPE_CLOTH:
        typestring = "Cloth";
        break;
    case ITEMTYPE_LEATHER:
        typestring = "Leather";
        break;
    case ITEMTYPE_MAIL:
        typestring = "Mail";
        break;
    case ITEMTYPE_PLATE:
        typestring = "Plate";
        break;
    case ITEMTYPE_WEAPONS:
        typestring = "Weapons and Shields";
        break;
    default:
        LOG_ERROR("module", "MOD-ZONE-DIFFICULTY: Unknown type {} in ZoneDifficulty::GetItemTypeString.", type);
    }
    return typestring;
}

std::string ZoneDifficulty::GetContentTypeString(uint32 type)
{
    std::string typestring;
    switch (type)
    {
    case TYPE_VANILLA:
        typestring = "for Vanilla dungeons.";
        break;
    case TYPE_RAID_MC:
        typestring = "for Molten Core.";
        break;
    case TYPE_RAID_ONY:
        typestring = "for Onyxia.";
        break;
    case TYPE_RAID_BWL:
        typestring = "for Blackwing Lair.";
        break;
    case TYPE_RAID_ZG:
        typestring = "for Zul Gurub.";
        break;
    case TYPE_RAID_AQ20:
        typestring = "for Ruins of Ahn'Qiraj.";
        break;
    case TYPE_RAID_AQ40:
        typestring = "for Temple of Ahn'Qiraj.";
        break;
    case TYPE_HEROIC_TBC:
        typestring = "for Heroic TBC dungeons.";
        break;
    case TYPE_RAID_T4:
        typestring = "for T4 Raids.";
        break;
    case TYPE_RAID_T5:
        typestring = "for T5 Raids.";
        break;
    case TYPE_RAID_T6:
        typestring = "for T6 Raids.";
        break;
    case TYPE_HEROIC_WOTLK:
        typestring = "for Heroic WotLK dungeons.";
        break;
    case TYPE_RAID_T7:
        typestring = "for T7 Raids.";
        break;
    case TYPE_RAID_T8:
        typestring = "for T8 Raids.";
        break;
    case TYPE_RAID_T9:
        typestring = "for T9 Raids.";
        break;
    case TYPE_RAID_T10:
        typestring = "for T10 Raids.";
        break;
    default:
        typestring = "-";
    }
    return typestring;
}

/**
 *  @brief Grants every player in the group one score for the hardmode.
 *
 *  @param map The map where the player is currently.
 *  @param type The type of instance the score is awarded for.
 */
void ZoneDifficulty::AddHardmodeScore(Map* map, uint32 type)
{
    if (!map)
    {
        LOG_ERROR("module", "MOD-ZONE-DIFFICULTY: No object for map in AddHardmodeScore.");
        return;
    }
    if (type > 255)
    {
        LOG_ERROR("module", "MOD-ZONE-DIFFICULTY: Wrong value for type: {} in AddHardmodeScore for map with id {}.", type, map->GetInstanceId());
        return;
    }
    //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: Called AddHardmodeScore for map id: {} and type: {}", map->GetId(), type);
    Map::PlayerList const& PlayerList = map->GetPlayers();
    for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
    {
        Player* player = i->GetSource();
        if (sZoneDifficulty->HardmodeScore.find(player->GetGUID().GetCounter()) == sZoneDifficulty->HardmodeScore.end())
        {
            sZoneDifficulty->HardmodeScore[player->GetGUID().GetCounter()][type] = 1;
        }
        else if (sZoneDifficulty->HardmodeScore[player->GetGUID().GetCounter()].find(type) == sZoneDifficulty->HardmodeScore[player->GetGUID().GetCounter()].end())
        {
            sZoneDifficulty->HardmodeScore[player->GetGUID().GetCounter()][type] = 1;
        }
        else
        {
            sZoneDifficulty->HardmodeScore[player->GetGUID().GetCounter()][type] = sZoneDifficulty->HardmodeScore[player->GetGUID().GetCounter()][type] + 1;
        }

        if (sZoneDifficulty->IsDebugInfoEnabled)
        {
            LOG_INFO("module", "MOD-ZONE-DIFFICULTY: Player {} new score: {}", player->GetName(), sZoneDifficulty->HardmodeScore[player->GetGUID().GetCounter()][type]);
        }

        std::string typestring = sZoneDifficulty->GetContentTypeString(type);
        ChatHandler(player->GetSession()).PSendSysMessage("You have received hardmode score %s New score: %i", typestring, sZoneDifficulty->HardmodeScore[player->GetGUID().GetCounter()][type]);
        CharacterDatabase.Execute("REPLACE INTO zone_difficulty_hardmode_score VALUES({}, {}, {})", player->GetGUID().GetCounter(), type, sZoneDifficulty->HardmodeScore[player->GetGUID().GetCounter()][type]);
    }
}

/**
 *  @brief Reduce the score of players when they pay for rewards.
 *
 *  @param player The one who pays with their score.
 *  @param type The type of instance the score is deducted for.
 */
void ZoneDifficulty::DeductHardmodeScore(Player* player, uint32 type, uint32 score)
{
    // NULL check happens in the calling function
    if (sZoneDifficulty->IsDebugInfoEnabled)
    {
        LOG_INFO("module", "MOD-ZONE-DIFFICULTY: Reducing score with type {} from player with guid {} by {}.", type, player->GetGUID().GetCounter(), score);
    }
    sZoneDifficulty->HardmodeScore[player->GetGUID().GetCounter()][type] = sZoneDifficulty->HardmodeScore[player->GetGUID().GetCounter()][type] - score;
    CharacterDatabase.Execute("REPLACE INTO zone_difficulty_hardmode_score VALUES({}, {}, {})", player->GetGUID().GetCounter(), type, sZoneDifficulty->HardmodeScore[player->GetGUID().GetCounter()][type]);
}

/**
 * @brief Send and item to the player using the data from sZoneDifficulty->Rewards.
 *
 * @param player The recipient of the mail.
 * @param category The content level e.g. TYPE_HEROIC_TBC.
 * @param itemType The type of the item e.g. ITEMTYPE_CLOTH.
 * @param id the id in the vector.
 */
void ZoneDifficulty::SendItem(Player* player, uint32 category, uint32 itemType, uint32 id)
{
    ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(sZoneDifficulty->Rewards[category][itemType][id].Entry);
    if (!itemTemplate)
    {
        LOG_ERROR("module", "MOD-ZONE-DIFFICULTY: itemTemplate could not be constructed in sZoneDifficulty->SendItem for category {}, itemType {}, id {}.", category, itemType, id);
        return;
    }

    ObjectGuid::LowType senderGuid = player->GetGUID().GetCounter();

    // fill mail
    MailDraft draft(REWARD_MAIL_SUBJECT, REWARD_MAIL_BODY);
    MailSender sender(MAIL_NORMAL, senderGuid, MAIL_STATIONERY_GM);
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    if (Item* item = Item::CreateItem(sZoneDifficulty->Rewards[category][itemType][id].Entry, 1, player))
    {
        if (sZoneDifficulty->Rewards[category][itemType][id].EnchantSlot != 0 && sZoneDifficulty->Rewards[category][itemType][id].Enchant != 0)
        {
            item->SetEnchantment(EnchantmentSlot(sZoneDifficulty->Rewards[category][itemType][id].EnchantSlot), sZoneDifficulty->Rewards[category][itemType][id].Enchant, 0, 0, player->GetGUID());
            player->ApplyEnchantment(item, EnchantmentSlot(sZoneDifficulty->Rewards[category][itemType][id].EnchantSlot), true, true, true);
        }
        item->SaveToDB(trans); // save for prevent lost at next mail load, if send fail then item will deleted
        draft.AddItem(item);
    }
    draft.SendMailTo(trans, MailReceiver(player, senderGuid), sender);
    CharacterDatabase.CommitTransaction(trans);
}

/**
 *  @brief Check if the map has assigned any data to tune it.
 *
 *  @param map The ID of the <Map> to check.
 *  @return The result as bool.
 */
bool ZoneDifficulty::IsHardmodeMap(uint32 mapId)
{
    if (sZoneDifficulty->HardmodeLoot.find(mapId) == sZoneDifficulty->HardmodeLoot.end())
    {
        return false;
    }
    return true;
}

/**
 *  @brief Check if the target is a player, a pet or a guardian.
 *
 * @param target The affected <Unit>
 * @return The result as bool. True for <Player>, <Pet> or <Guardian>.
 */
bool ZoneDifficulty::IsValidNerfTarget(Unit* target)
{
    return target->IsPlayer() || target->IsPet() || target->IsGuardian();
}

/**
 *  @brief Checks if the element is one of the uint32 values in the vector.
 *
 * @param vec A vector
 * @param element One element which can potentially be part of the values in the vector
 * @return The result as bool
 */
bool ZoneDifficulty::VectorContainsUint32(std::vector<uint32> vec, uint32 element)
{
    return find(vec.begin(), vec.end(), element) != vec.end();
}

/**
 *  @brief Checks if the target is in a duel while residing in the DUEL_AREA and their opponent is a valid object.
 *  Used to determine when the duel-specific nerfs should be applied.
 *
 * @param target The affected <Unit>
 * @return The result as bool
 */
bool ZoneDifficulty::ShouldNerfInDuels(Unit* target)
{
    if (target->GetAreaId() != DUEL_AREA)
    {
        return false;
    }

    if (target->ToTempSummon() && target->ToTempSummon()->GetSummoner())
    {
        target = target->ToTempSummon()->GetSummoner()->ToUnit();
    }

    if (!target->GetAffectingPlayer())
    {
        return false;
    }

    if (!target->GetAffectingPlayer()->duel)
    {
        return false;
    }

    if (target->GetAffectingPlayer()->duel->State != DUEL_STATE_IN_PROGRESS)
    {
        return false;
    }

    if (!target->GetAffectingPlayer()->duel->Opponent)
    {
        return false;
    }

    return true;
}

/**
 *  @brief Find the lowest phase for the target's mapId, which has a db entry for the target's map
 *  and at least partly matches the target's phase.
 *
 *  `mapId` can be the id of a map or `DUEL_INDEX` to use the duel specific settings.
 *  Return -1 if none found.
 *
 * @param mapId
 * @param phaseMask Bitmask of all phases where the unit is currently visible
 * @return the lowest phase which should be altered for this map and the unit is visible in
 */
int32 ZoneDifficulty::GetLowestMatchingPhase(uint32 mapId, uint32 phaseMask)
{
    // Check if there is an entry for the mapId at all
    if (sZoneDifficulty->NerfInfo.find(mapId) != sZoneDifficulty->NerfInfo.end())
    {

        // Check if 0 is assigned as a phase to cover all phases
        if (sZoneDifficulty->NerfInfo[mapId].find(0) != sZoneDifficulty->NerfInfo[mapId].end())
        {
            return 0;
        }

        // Check all $key in [mapId][$key] if they match the target's visible phases
        for (auto const& [key, value] : sZoneDifficulty->NerfInfo[mapId])
        {
            if (key & phaseMask)
            {
                return key;
            }
        }
    }
    return -1;
}

/**
 *  @brief Store the HardmodeInstanceData in the database for the given instance id.
 *  zone_difficulty_instance_saves is used to store the data.
 *
 *  @param InstanceID INT NOT NULL DEFAULT 0,
 *  @param HardmodeOn TINYINT NOT NULL DEFAULT 0,
 */
void ZoneDifficulty::SaveHardmodeInstanceData(uint32 instanceId)
{
    if (sZoneDifficulty->HardmodeInstanceData.find(instanceId) == sZoneDifficulty->HardmodeInstanceData.end())
    {
        LOG_INFO("module", "MOD-ZONE-DIFFICULTY: ZoneDifficulty::SaveHardmodeInstanceData: InstanceId {} not found in HardmodeInstanceData.", instanceId);
        return;
    }
    LOG_INFO("module", "MOD-ZONE-DIFFICULTY: ZoneDifficulty::SaveHardmodeInstanceData: Saving instanceId {} with HardmodeOn {}", instanceId, sZoneDifficulty->HardmodeInstanceData[instanceId]);
    CharacterDatabase.Execute("REPLACE INTO zone_difficulty_instance_saves (InstanceID, HardmodeOn) VALUES ({}, {})", instanceId, sZoneDifficulty->HardmodeInstanceData[instanceId]);
}

/**
 *  @brief Fetch all players on the [Unit]s threat list
 *
 *  @param unit The one to check for their threat list.
 *  @param entry Key value to access HardmodeAI map/vector data
 *  @param key Key value to access HardmodeAI map/vector
 */
std::list<Unit*> ZoneDifficulty::GetTargetList(Unit* unit, uint32 entry, uint32 key)
{
    auto const& threatlist = unit->GetThreatMgr().GetThreatList();
    std::list<Unit*> targetList;
    if (threatlist.empty())
    {
        LOG_INFO("module", "MOD-ZONE-DIFFICULTY: Threatlist is empty for unit {}", unit->GetName());
        return targetList;
    }

    for (auto itr = threatlist.begin(); itr != threatlist.end(); ++itr)
    {
        Unit* target = (*itr)->getTarget();
        if (!target)
        {
            continue;
        }
        if (target->GetTypeId() != TYPEID_PLAYER)
        {
            continue;
        }
        //Target chosen based on a distance give in TargetArg (for TARGET_PLAYER_DISTANCE)
        if (sZoneDifficulty->HardmodeAI[entry][key].Target == TARGET_PLAYER_DISTANCE)
        {
            if (!unit->IsWithinDist(target, sZoneDifficulty->HardmodeAI[entry][key].TargetArg))
            {
                continue;
            }
        }
        //Target chosen based on the min/max distance of the spell
        else
        {
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(sZoneDifficulty->HardmodeAI[entry][key].Spell);
            if (spellInfo)
            {
                if (unit->IsWithinDist(target, spellInfo->GetMinRange()))
                {
                    continue;
                }
                if (!unit->IsWithinDist(target, spellInfo->GetMaxRange()))
                {
                    continue;
                }
            }
        }
        targetList.push_back(target);
    }
    return targetList;
}

void ZoneDifficulty::HardmodeEvent(Unit* unit, uint32 entry, uint32 key)
{
    LOG_INFO("module", "MOD-ZONE-DIFFICULTY: HardmodeEvent for entry {} with key {}", entry, key);
    if (unit && unit->IsAlive())
    {
        if (!unit->IsInCombat())
        {
            unit->m_Events.CancelEventGroup(EVENT_GROUP);
            return;
        }
        LOG_INFO("module", "MOD-ZONE-DIFFICULTY: HardmodeEvent IsInCombat for entry {} with key {}", entry, key);
        // Try again in 1s if the unit is currently casting
        if (unit->HasUnitState(UNIT_STATE_CASTING))
        {
            LOG_INFO("module", "MOD-ZONE-DIFFICULTY: HardmodeEvent Re-schedule AI event in 1s because unit is casting for entry {} with key {}", entry, key);
            unit->m_Events.AddEventAtOffset([unit, entry, key]()
                {
                    sZoneDifficulty->HardmodeEvent(unit, entry, key);
                }, 1s, EVENT_GROUP);
            return;
        }

        //Re-schedule the event
        if (sZoneDifficulty->HardmodeAI[entry][key].Repetitions == 0)
        {
            unit->m_Events.AddEventAtOffset([unit, entry, key]()
                {
                    sZoneDifficulty->HardmodeEvent(unit, entry, key);
                }, sZoneDifficulty->HardmodeAI[entry][key].Cooldown, EVENT_GROUP);
        }

        // Select target. Default = TARGET_SELF
        Unit* target = unit;
        if (sZoneDifficulty->HardmodeAI[entry][key].Target == TARGET_VICTIM)
        {
            target = unit->GetVictim();
        }
        else if (sZoneDifficulty->HardmodeAI[entry][key].Target == TARGET_PLAYER_DISTANCE)
        {
            std::list<Unit*> targetList = sZoneDifficulty->GetTargetList(unit, entry, key);
            if (targetList.empty())
            {
                return;
            }
            for (Unit* trg : targetList)
            {
                if (trg)
                {
                    LOG_INFO("module", "Creature casting HardmodeAI spell: {} at target {}", sZoneDifficulty->HardmodeAI[entry][key].Spell, trg->GetName());
                    unit->CastSpell(trg, sZoneDifficulty->HardmodeAI[entry][key].Spell, true);
                }
            }
            return;
        }
        else
        {
            std::list<Unit*> targetList = sZoneDifficulty->GetTargetList(unit, entry, key);
            if (targetList.empty())
            {
                return;
            }
            if (targetList.size() < 2)
            {
                target = unit->GetVictim();
            }
            else
            {
                uint8 counter = targetList.size() - 1;
                if (sZoneDifficulty->HardmodeAI[entry][key].TargetArg > counter)
                {
                    counter = sZoneDifficulty->HardmodeAI[entry][key].TargetArg;
                }

                switch (sZoneDifficulty->HardmodeAI[entry][key].Target)
                {
                case TARGET_HOSTILE_AGGRO_FROM_TOP:
                {
                    std::list<Unit*>::const_iterator itr = targetList.begin();
                    std::advance(itr, counter);
                    target = *itr;
                    break;
                }
                case TARGET_HOSTILE_AGGRO_FROM_BOTTOM:
                {
                    std::list<Unit*>::reverse_iterator ritr = targetList.rbegin();
                    std::advance(ritr, counter);
                    target = *ritr;
                    break;
                }
                case TARGET_HOSTILE_RANDOM:
                {
                    std::list<Unit*>::const_iterator itr = targetList.begin();
                    std::advance(itr, urand(0, targetList.size() - 1));
                    target = *itr;
                    break;
                }
                case TARGET_HOSTILE_RANDOM_NOT_TOP:
                {
                    std::list<Unit*>::const_iterator itr = targetList.begin();
                    std::advance(itr, urand(1, targetList.size() - 1));
                    target = *itr;
                    break;
                }
                default:
                {
                    LOG_ERROR("module", "MOD-ZONE-DIFFICULTY: Unknown type for Target: {} in zone_difficulty_hardmode_ai", sZoneDifficulty->HardmodeAI[entry][key].Target);
                }
                }
            }
        }

        if (target)
        {
            LOG_INFO("module", "Creature casting HardmodeAI spell: {} at target {}", sZoneDifficulty->HardmodeAI[entry][key].Spell, target->GetName());
            unit->CastSpell(target, sZoneDifficulty->HardmodeAI[entry][key].Spell, true);
        }
    }
}

class mod_zone_difficulty_unitscript : public UnitScript
{
public:
    mod_zone_difficulty_unitscript() : UnitScript("mod_zone_difficulty_unitscript") { }

    void OnAuraApply(Unit* target, Aura* aura) override
    {
        if (!sZoneDifficulty->IsEnabled)
        {
            return;
        }

        if (sZoneDifficulty->IsValidNerfTarget(target))
        {
            uint32 mapId = target->GetMapId();
            bool nerfInDuel = sZoneDifficulty->ShouldNerfInDuels(target);

            //Check if the map of the target is subject of a nerf at all OR if the target is subject of a nerf in a duel
            if (sZoneDifficulty->NerfInfo.find(mapId) != sZoneDifficulty->NerfInfo.end() || nerfInDuel)
            {
                if (SpellInfo const* spellInfo = aura->GetSpellInfo())
                {
                    // Skip spells not affected by vulnerability (potions)
                    if (spellInfo->HasAttribute(SPELL_ATTR0_NO_IMMUNITIES))
                    {
                        return;
                    }

                    if (spellInfo->HasAura(SPELL_AURA_SCHOOL_ABSORB))
                    {
                        std::list<AuraEffect*> AuraEffectList = target->GetAuraEffectsByType(SPELL_AURA_SCHOOL_ABSORB);

                        for (AuraEffect* eff : AuraEffectList)
                        {
                            if ((eff->GetAuraType() != SPELL_AURA_SCHOOL_ABSORB) || (eff->GetSpellInfo()->Id != spellInfo->Id))
                            {
                                continue;
                            }

                            if (sZoneDifficulty->IsDebugInfoEnabled)
                            {
                                if (Player* player = target->ToPlayer()) // Pointless check? Perhaps.
                                {
                                    if (player->GetSession())
                                    {
                                        ChatHandler(player->GetSession()).PSendSysMessage("Spell: %s (%u) Base Value: %i", spellInfo->SpellName[player->GetSession()->GetSessionDbcLocale()], spellInfo->Id, eff->GetAmount());
                                    }
                                }
                            }

                            int32 absorb = eff->GetAmount();
                            uint32 phaseMask = target->GetPhaseMask();
                            int matchingPhase = sZoneDifficulty->GetLowestMatchingPhase(mapId, phaseMask);
                            int8 mode = sZoneDifficulty->NerfInfo[mapId][matchingPhase].Enabled;
                            if (matchingPhase != -1)
                            {
                                Map* map = target->GetMap();
                                if (sZoneDifficulty->HasNormalMode(mode))
                                {
                                    absorb = eff->GetAmount() * sZoneDifficulty->NerfInfo[mapId][matchingPhase].AbsorbNerfPct;
                                }
                                if (sZoneDifficulty->HasHardMode(mode) && sZoneDifficulty->HardmodeInstanceData[target->GetMap()->GetInstanceId()])
                                {
                                    if (map->IsRaid() ||
                                        (map->IsHeroic() && map->IsDungeon()))
                                    {
                                        absorb = eff->GetAmount() * sZoneDifficulty->NerfInfo[mapId][matchingPhase].AbsorbNerfPctHard;
                                    }
                                }
                            }
                            else if (sZoneDifficulty->NerfInfo[DUEL_INDEX][0].Enabled > 0 && nerfInDuel)
                            {
                                absorb = eff->GetAmount() * sZoneDifficulty->NerfInfo[DUEL_INDEX][0].AbsorbNerfPct;
                            }

                            //This check must be last and override duel and map adjustments
                            if (sZoneDifficulty->SpellNerfOverrides.find(spellInfo->Id) != sZoneDifficulty->SpellNerfOverrides.end())
                            {
                                absorb = eff->GetAmount() * sZoneDifficulty->SpellNerfOverrides[spellInfo->Id];
                            }

                            eff->SetAmount(absorb);

                            if (sZoneDifficulty->IsDebugInfoEnabled)
                            {
                                if (Player* player = target->ToPlayer()) // Pointless check? Perhaps.
                                {
                                    if (player->GetSession())
                                    {
                                        ChatHandler(player->GetSession()).PSendSysMessage("Spell: %s (%u) Post Nerf Value: %i", spellInfo->SpellName[player->GetSession()->GetSessionDbcLocale()], spellInfo->Id, eff->GetAmount());
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    void ModifyHealReceived(Unit* target, Unit* /*healer*/, uint32& heal, SpellInfo const* spellInfo) override
    {
        if (!sZoneDifficulty->IsEnabled)
        {
            return;
        }

        if (sZoneDifficulty->IsValidNerfTarget(target))
        {
            if (spellInfo)
            {
                // Skip spells not affected by vulnerability (potions) and bandages
                if (spellInfo->HasAttribute(SPELL_ATTR0_NO_IMMUNITIES) || spellInfo->Mechanic == MECHANIC_BANDAGE)
                {
                    return;
                }
            }

            uint32 mapId = target->GetMapId();
            bool nerfInDuel = sZoneDifficulty->ShouldNerfInDuels(target);
            //Check if the map of the target is subject of a nerf at all OR if the target is subject of a nerf in a duel
            if (sZoneDifficulty->NerfInfo.find(mapId) != sZoneDifficulty->NerfInfo.end() || sZoneDifficulty->ShouldNerfInDuels(target))
            {
                //This check must be first and skip the rest to override everything else.
                if (spellInfo)
                {
                    if (sZoneDifficulty->SpellNerfOverrides.find(mapId) != sZoneDifficulty->SpellNerfOverrides.end())
                    {
                        heal = heal * sZoneDifficulty->SpellNerfOverrides[spellInfo->Id];
                        return;
                    }
                }

                uint32 phaseMask = target->GetPhaseMask();
                int matchingPhase = sZoneDifficulty->GetLowestMatchingPhase(mapId, phaseMask);
                int8 mode = sZoneDifficulty->NerfInfo[mapId][matchingPhase].Enabled;
                if (matchingPhase != -1)
                {
                    Map* map = target->GetMap();
                    if (sZoneDifficulty->HasNormalMode(mode))
                    {
                        heal = heal * sZoneDifficulty->NerfInfo[mapId][matchingPhase].HealingNerfPct;
                    }
                    if (sZoneDifficulty->HasHardMode(mode) && sZoneDifficulty->HardmodeInstanceData[map->GetInstanceId()])
                    {
                        if (map->IsRaid() ||
                            (map->IsHeroic() && map->IsDungeon()))
                        {
                            heal = heal * sZoneDifficulty->NerfInfo[mapId][matchingPhase].HealingNerfPctHard;
                        }
                    }
                }
                else if (sZoneDifficulty->NerfInfo[DUEL_INDEX][0].Enabled > 0 && nerfInDuel)
                {
                    heal = heal * sZoneDifficulty->NerfInfo[DUEL_INDEX][0].HealingNerfPct;
                }
            }
        }
    }

    void ModifyPeriodicDamageAurasTick(Unit* target, Unit* attacker, uint32& damage, SpellInfo const* spellInfo) override
    {
        if (!sZoneDifficulty->IsEnabled)
        {
            return;
        }

        bool isDot = false;

        if (spellInfo)
        {
            for (auto const& eff : spellInfo->GetEffects())
            {
                if (eff.ApplyAuraName == SPELL_AURA_PERIODIC_DAMAGE || eff.ApplyAuraName == SPELL_AURA_PERIODIC_DAMAGE_PERCENT)
                {
                    isDot = true;
                }
            }
        }

        if (!isDot)
        {
            return;
        }

        // Disclaimer: also affects disables boss adds buff.
        if (sConfigMgr->GetOption<bool>("ModZoneDifficulty.SpellBuff.OnlyBosses", false))
        {
            if (attacker->ToCreature() && !attacker->ToCreature()->IsDungeonBoss())
            {
                return;
            }
        }

        if (sZoneDifficulty->IsValidNerfTarget(target))
        {
            uint32 mapId = target->GetMapId();
            uint32 phaseMask = target->GetPhaseMask();
            int32 matchingPhase = sZoneDifficulty->GetLowestMatchingPhase(mapId, phaseMask);

            if (sZoneDifficulty->IsDebugInfoEnabled)
            {
                if (Player* player = attacker->ToPlayer())
                {
                    if (player->GetSession())
                    {
                        ChatHandler(player->GetSession()).PSendSysMessage("A dot tick will be altered. Pre Nerf Value: %i", damage);
                    }
                }
            }

            if (sZoneDifficulty->NerfInfo.find(mapId) != sZoneDifficulty->NerfInfo.end() && matchingPhase != -1)
            {
                int8 mode = sZoneDifficulty->NerfInfo[mapId][matchingPhase].Enabled;
                Map* map = target->GetMap();
                if (sZoneDifficulty->HasNormalMode(mode))
                {
                    damage = damage * sZoneDifficulty->NerfInfo[mapId][matchingPhase].SpellDamageBuffPct;
                }
                if (sZoneDifficulty->HasHardMode(mode) && sZoneDifficulty->HardmodeInstanceData[map->GetInstanceId()])
                {
                    if (map->IsRaid() ||
                        (map->IsHeroic() && map->IsDungeon()))
                    {
                        damage = damage * sZoneDifficulty->NerfInfo[mapId][matchingPhase].SpellDamageBuffPctHard;
                    }
                }
            }
            else if (sZoneDifficulty->ShouldNerfInDuels(target))
            {
                if (sZoneDifficulty->NerfInfo[DUEL_INDEX][0].Enabled > 0)
                {
                    damage = damage * sZoneDifficulty->NerfInfo[DUEL_INDEX][0].SpellDamageBuffPct;
                }
            }

            if (sZoneDifficulty->IsDebugInfoEnabled)
            {
                if (Player* player = attacker->ToPlayer())
                {
                    if (player->GetSession())
                    {
                        ChatHandler(player->GetSession()).PSendSysMessage("A dot tick was altered. Post Nerf Value: %i", damage);
                    }
                }
            }
        }
    }

    void ModifySpellDamageTaken(Unit* target, Unit* attacker, int32& damage, SpellInfo const* spellInfo) override
    {
        if (!sZoneDifficulty->IsEnabled)
        {
            return;
        }

        // Disclaimer: also affects disables boss adds buff.
        if (sConfigMgr->GetOption<bool>("ModZoneDifficulty.SpellBuff.OnlyBosses", false))
        {
            if (attacker->ToCreature() && !attacker->ToCreature()->IsDungeonBoss())
            {
                return;
            }
        }

        if (sZoneDifficulty->IsValidNerfTarget(target))
        {
            uint32 mapId = target->GetMapId();
            uint32 phaseMask = target->GetPhaseMask();
            int32 matchingPhase = sZoneDifficulty->GetLowestMatchingPhase(mapId, phaseMask);
            if (spellInfo)
            {
                //This check must be first and skip the rest to override everything else.
                if (sZoneDifficulty->SpellNerfOverrides.find(spellInfo->Id) != sZoneDifficulty->SpellNerfOverrides.end())
                {
                    damage = damage * sZoneDifficulty->SpellNerfOverrides[spellInfo->Id];
                    return;
                }

                if (sZoneDifficulty->IsDebugInfoEnabled)
                {
                    if (Player* player = target->ToPlayer()) // Pointless check? Perhaps.
                    {
                        if (player->GetSession())
                        {
                            ChatHandler(player->GetSession()).PSendSysMessage("Spell: %s (%u) Before Nerf Value: %i (%f Normal Mode)", spellInfo->SpellName[player->GetSession()->GetSessionDbcLocale()], spellInfo->Id, damage, sZoneDifficulty->NerfInfo[mapId][matchingPhase].SpellDamageBuffPct);
                            ChatHandler(player->GetSession()).PSendSysMessage("Spell: %s (%u) Before Nerf Value: %i (%f Hard Mode)", spellInfo->SpellName[player->GetSession()->GetSessionDbcLocale()], spellInfo->Id, damage, sZoneDifficulty->NerfInfo[mapId][matchingPhase].SpellDamageBuffPctHard);
                        }
                    }
                }
            }

            if (sZoneDifficulty->NerfInfo.find(mapId) != sZoneDifficulty->NerfInfo.end() && matchingPhase != -1)
            {
                int8 mode = sZoneDifficulty->NerfInfo[mapId][matchingPhase].Enabled;
                Map* map = target->GetMap();
                if (sZoneDifficulty->HasNormalMode(mode))
                {
                    damage = damage * sZoneDifficulty->NerfInfo[mapId][matchingPhase].SpellDamageBuffPct;
                }
                if (sZoneDifficulty->HasHardMode(mode) && sZoneDifficulty->HardmodeInstanceData[map->GetInstanceId()])
                {
                    if (map->IsRaid() ||
                        (map->IsHeroic() && map->IsDungeon()))
                    {
                        damage = damage * sZoneDifficulty->NerfInfo[mapId][matchingPhase].SpellDamageBuffPctHard;
                    }
                }
            }
            else if (sZoneDifficulty->ShouldNerfInDuels(target))
            {
                if (sZoneDifficulty->NerfInfo[DUEL_INDEX][0].Enabled > 0)
                {
                    damage = damage * sZoneDifficulty->NerfInfo[DUEL_INDEX][0].SpellDamageBuffPct;
                }
            }

            if (sZoneDifficulty->IsDebugInfoEnabled)
            {
                if (Player* player = target->ToPlayer()) // Pointless check? Perhaps.
                {
                    if (player->GetSession())
                    {
                        ChatHandler(player->GetSession()).PSendSysMessage("Spell: %s (%u) Post Nerf Value: %i", spellInfo->SpellName[player->GetSession()->GetSessionDbcLocale()], spellInfo->Id, damage);
                    }
                }
            }
        }
    }

    void ModifyMeleeDamage(Unit* target, Unit* attacker, uint32& damage) override
    {
        if (!sZoneDifficulty->IsEnabled)
        {
            return;
        }

        // Disclaimer: also affects disables boss adds buff.
        if (sConfigMgr->GetOption<bool>("ModZoneDifficulty.MeleeBuff.OnlyBosses", false))
        {
            if (attacker->ToCreature() && !attacker->ToCreature()->IsDungeonBoss())
            {
                return;
            }
        }

        if (sZoneDifficulty->IsValidNerfTarget(target))
        {
            uint32 mapId = target->GetMapId();
            uint32 phaseMask = target->GetPhaseMask();
            int matchingPhase = sZoneDifficulty->GetLowestMatchingPhase(mapId, phaseMask);
            if (sZoneDifficulty->NerfInfo.find(mapId) != sZoneDifficulty->NerfInfo.end() && matchingPhase != -1)
            {
                int8 mode = sZoneDifficulty->NerfInfo[mapId][matchingPhase].Enabled;
                Map* map = target->GetMap();
                if (sZoneDifficulty->HasNormalMode(mode))
                {
                    damage = damage * sZoneDifficulty->NerfInfo[mapId][matchingPhase].MeleeDamageBuffPct;
                }
                if (sZoneDifficulty->HasHardMode(mode) && sZoneDifficulty->HardmodeInstanceData[target->GetMap()->GetInstanceId()])
                {
                    if (map->IsRaid() ||
                        (map->IsHeroic() && map->IsDungeon()))
                    {
                        damage = damage * sZoneDifficulty->NerfInfo[mapId][matchingPhase].MeleeDamageBuffPctHard;
                    }
                }
            }
            else if (sZoneDifficulty->ShouldNerfInDuels(target))
            {
                if (sZoneDifficulty->NerfInfo[DUEL_INDEX][0].Enabled > 0)
                {
                    damage = damage * sZoneDifficulty->NerfInfo[DUEL_INDEX][0].MeleeDamageBuffPct;
                }
            }
        }
    }

    //void OnUnitEnterEvadeMode(Unit* unit, uint8 /*why*/) override
    //{
    //    uint32 entry = unit->GetEntry();
    //    LOG_INFO("module", "OnUnitEnterEvadeMode fired. Entry: {}", entry);
    //    if (entry == 19389)
    //    {
    //        unit->m_Events.CancelEventGroup(EVENT_GROUP);
    //        LOG_INFO("module", "Unit evading, events reset.");
    //    }
    //}

    /**
     *  @brief Check if the Hardmode is activated for the instance and if the creature has any hardmode AI assigned. Schedule the events, if so.
     */
    void OnUnitEnterCombat(Unit* unit, Unit* /*victim*/) override
    {
        //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: OnUnitEnterCombat for unit {}", unit->GetEntry());
        if (sZoneDifficulty->HardmodeInstanceData.find(unit->GetInstanceId()) == sZoneDifficulty->HardmodeInstanceData.end())
        {
            return;
        }
        if (!sZoneDifficulty->HardmodeInstanceData[unit->GetInstanceId()])
        {
            return;
        }

        if (Creature* creature = unit->ToCreature())
        {
            if (creature->IsTrigger())
            {
                return;
            }
        }

        unit->m_Events.CancelEventGroup(EVENT_GROUP);

        uint32 entry = unit->GetEntry();
        if (sZoneDifficulty->HardmodeAI.find(entry) == sZoneDifficulty->HardmodeAI.end())
        {
            return;
        }

        LOG_INFO("module", "MOD-ZONE-DIFFICULTY: OnUnitEnterCombat checks passed for unit {}", unit->GetEntry());
        uint32 i = 0;
        for (ZoneDifficultyHAI& data : sZoneDifficulty->HardmodeAI[entry])
        {
            if (data.Chance == 100 || data.Chance >= urand(1, 100))
            {
                unit->m_Events.AddEventAtOffset([unit, entry, i]()
                    {
                        sZoneDifficulty->HardmodeEvent(unit, entry, i);
                    }, data.Delay, EVENT_GROUP);
            }
            ++i;
        }
    }

    //void BeforeSendNonMeleeDamage(Unit* caster, SpellNonMeleeDamage* log)
    //void BeforeSendHeal(Unit* caster, HealInfo* healInfo) override
    //{
    //    LOG_INFO("module", "GetHeal {}, GetAbsorb {}, Healer {}, Target {} ", healInfo->GetHeal(), healInfo->GetAbsorb(), healInfo->GetHealer()->GetName(), healInfo->GetTarget()->GetName());
    //}
};

class mod_zone_difficulty_playerscript : public PlayerScript
{
public:
    mod_zone_difficulty_playerscript() : PlayerScript("mod_zone_difficulty_playerscript") { }

    void OnMapChanged(Player* player) override
    {
        uint32 mapId = player->GetMapId();
        if (sZoneDifficulty->DisallowedBuffs.find(mapId) != sZoneDifficulty->DisallowedBuffs.end())
        {
            for (auto aura : sZoneDifficulty->DisallowedBuffs[mapId])
            {
                player->RemoveAura(aura);
            }
        }
    }
};

class mod_zone_difficulty_petscript : public PetScript
{
public:
    mod_zone_difficulty_petscript() : PetScript("mod_zone_difficulty_petscript") { }

    void OnPetAddToWorld(Pet* pet) override
    {
        uint32 mapId = pet->GetMapId();
        if (sZoneDifficulty->DisallowedBuffs.find(mapId) != sZoneDifficulty->DisallowedBuffs.end())
        {
            pet->m_Events.AddEventAtOffset([mapId, pet]()
                {
                    for (uint32 aura : sZoneDifficulty->DisallowedBuffs[mapId])
                    {
                        pet->RemoveAurasDueToSpell(aura);
                    }
                }, 2s);
        }
    }
};

class mod_zone_difficulty_worldscript : public WorldScript
{
public:
    mod_zone_difficulty_worldscript() : WorldScript("mod_zone_difficulty_worldscript") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        sZoneDifficulty->IsEnabled = sConfigMgr->GetOption<bool>("ModZoneDifficulty.Enable", false);
        sZoneDifficulty->IsDebugInfoEnabled = sConfigMgr->GetOption<bool>("ModZoneDifficulty.DebugInfo", false);
        sZoneDifficulty->HardmodeHpModifier = sConfigMgr->GetOption<float>("ModZoneDifficulty.Hardmode.HpModifier", 2);
        sZoneDifficulty->LoadMapDifficultySettings();
    }

    void OnStartup() override
    {
        sZoneDifficulty->LoadHardmodeInstanceData();
        sZoneDifficulty->LoadHardmodeScoreData();
    }
};

class mod_zone_difficulty_globalscript : public GlobalScript
{
public:
    mod_zone_difficulty_globalscript() : GlobalScript("mod_zone_difficulty_globalscript") { }

    void OnBeforeSetBossState(uint32 id, EncounterState newState, EncounterState oldState, Map* instance) override
    {
        if (sZoneDifficulty->IsDebugInfoEnabled)
        {
            LOG_INFO("module", "MOD-ZONE-DIFFICULTY: OnBeforeSetBossState: bossId = {}, newState = {}, oldState = {}, MapId = {}, InstanceId = {}", id, newState, oldState, instance->GetId(), instance->GetInstanceId());
        }
        uint32 instanceId = instance->GetInstanceId();
        if (!sZoneDifficulty->IsHardmodeMap(instance->GetId()))
        {
            //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: OnBeforeSetBossState: Instance not handled because there is no hardmode loot data for map id: {}", instance->GetId());
            return;
        }
        if (oldState != IN_PROGRESS && newState == IN_PROGRESS)
        {
            if (sZoneDifficulty->HardmodeInstanceData[instanceId])
            {
                sZoneDifficulty->EncountersInProgress[instanceId] = GameTime::GetGameTime().count();
            }
        }
        else if (oldState == IN_PROGRESS && newState == DONE)
        {
            if (sZoneDifficulty->HardmodeInstanceData[instanceId])
            {
                //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: Hardmode is on.");
                if (sZoneDifficulty->EncountersInProgress.find(instanceId) != sZoneDifficulty->EncountersInProgress.end() && sZoneDifficulty->EncountersInProgress[instanceId] != 0)
                {
                    Map::PlayerList const& PlayerList = instance->GetPlayers();
                    for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                    {
                        Player* player = i->GetSource();
                        if (!player->IsGameMaster() && !player->IsDeveloper())
                        {
                            CharacterDatabase.Execute(
                                "REPLACE INTO `zone_difficulty_encounter_logs` VALUES({}, {}, {}, {}, {}, {}, {})",
                                instanceId, sZoneDifficulty->EncountersInProgress[instanceId], GameTime::GetGameTime().count(), instance->GetId(), id, player->GetGUID().GetCounter(), 64);
                        }
                    }
                }
            }
        }
    }

    void OnInstanceIdRemoved(uint32 instanceId) override
    {
        //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: OnInstanceIdRemoved: instanceId = {}", instanceId);
        if (sZoneDifficulty->HardmodeInstanceData.find(instanceId) != sZoneDifficulty->HardmodeInstanceData.end())
        {
            sZoneDifficulty->HardmodeInstanceData.erase(instanceId);
        }

        CharacterDatabase.Execute("DELETE FROM zone_difficulty_instance_saves WHERE InstanceID = {};", instanceId);
    }

    void OnAfterUpdateEncounterState(Map* map, EncounterCreditType /*type*/, uint32 /*creditEntry*/, Unit* source, Difficulty /*difficulty_fixed*/, DungeonEncounterList const* /*encounters*/, uint32 /*dungeonCompleted*/, bool /*updated*/) override
    {
        if (!source)
        {
            //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: source is a nullptr in OnAfterUpdateEncounterState");
            return;
        }

        if (sZoneDifficulty->HardmodeInstanceData.find(map->GetInstanceId()) != sZoneDifficulty->HardmodeInstanceData.end())
        {
            //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: Encounter completed. Map relevant. Checking for source: {}", source->GetEntry());
            // Give additional loot, if the encounter was in hardmode.
            if (sZoneDifficulty->HardmodeInstanceData[map->GetInstanceId()])
            {
                uint32 mapId = map->GetId();
                if (!sZoneDifficulty->IsHardmodeMap(mapId))
                {
                    //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: No additional loot stored in map with id {}.", map->GetInstanceId());
                    return;
                }

                bool SourceAwardsHardmodeLoot = false;
                //iterate over all listed creature entries for that map id and see, if the encounter should yield hardmode loot and if a go is to be looted instead
                for (auto value : sZoneDifficulty->HardmodeLoot[mapId])
                {
                    if (value.EncounterEntry == source->GetEntry())
                    {
                        SourceAwardsHardmodeLoot = true;
                        break;
                    }
                }

                if (!SourceAwardsHardmodeLoot)
                {
                    return;
                }

                if (map->IsHeroic() && map->IsNonRaidDungeon())
                {
                    sZoneDifficulty->AddHardmodeScore(map, sZoneDifficulty->Expansion[mapId]);
                }
                else if (map->IsRaid())
                {
                    sZoneDifficulty->AddHardmodeScore(map, sZoneDifficulty->Expansion[mapId]);
                }
                /* debug
                 * else
                 * {
                 *   LOG_INFO("module", "MOD-ZONE-DIFFICULTY: Map with id {} is not a raid or a dungeon. Hardmode loot not granted.", map->GetInstanceId());
                 * }
                 */
            }
        }
    }
};

class mod_zone_difficulty_rewardnpc : public CreatureScript
{
public:
    mod_zone_difficulty_rewardnpc() : CreatureScript("mod_zone_difficulty_rewardnpc") { }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        if (sZoneDifficulty->IsDebugInfoEnabled)
        {
            LOG_INFO("module", "MOD-ZONE-DIFFICULTY: OnGossipSelectRewardNpc action: {}", action);
        }
        ClearGossipMenuFor(player);
        uint32 npcText = 0;
        //Safety measure. There's a way for action 0 to happen even though it's not provided in the gossip menu.
        if (action == 0)
        {
            CloseGossipMenuFor(player);
            return true;
        }

        if (action == 999998)
        {
            CloseGossipMenuFor(player);
            return true;
        }

        if (action == 999999)
        {
            npcText = NPC_TEXT_SCORE;
            for (int i = 1; i <= 16; ++i)
            {
                std::string whisper;
                whisper.append("You score is ");
                if (sZoneDifficulty->HardmodeScore.find(player->GetGUID().GetCounter()) == sZoneDifficulty->HardmodeScore.end())
                {
                    whisper.append("0 ");
                }
                else if (sZoneDifficulty->HardmodeScore[player->GetGUID().GetCounter()].find(i) == sZoneDifficulty->HardmodeScore[player->GetGUID().GetCounter()].end())
                {
                    whisper.append("0 ");
                }
                else
                {
                    whisper.append(std::to_string(sZoneDifficulty->HardmodeScore[player->GetGUID().GetCounter()][i])).append(" ");
                }
                whisper.append(sZoneDifficulty->GetContentTypeString(i));
                creature->Whisper(whisper, LANG_UNIVERSAL, player);
            }
        }
        // player has selected a content type
        else if (action < 100)
        {
            npcText = NPC_TEXT_CATEGORY;
            uint32 i = 1;
            for (auto& itemType : sZoneDifficulty->Rewards[action])
            {
                //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: typedata.first is {}", itemType.first);
                std::string gossip;
                std::string typestring = sZoneDifficulty->GetItemTypeString(itemType.first);
                if (sZoneDifficulty->ItemIcons.find(i) != sZoneDifficulty->ItemIcons.end())
                {
                    gossip.append(sZoneDifficulty->ItemIcons[i]);
                }
                gossip.append("I am interested in items from the ").append(typestring).append(" category.");
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, gossip, GOSSIP_SENDER_MAIN, itemType.first + (action * 100));
                ++i;
            }
        }
        else if (action < 1000)
        {
            npcText = NPC_TEXT_ITEM;
            uint32 category = 0;
            uint32 counter = action;
            while (counter > 99)
            {
                ++category;
                counter = counter - 100;
            }
            //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: Building gossip with category {} and counter {}", category, counter);

            for (size_t i = 0; i < sZoneDifficulty->Rewards[category][counter].size(); ++i)
            {
                //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: Adding gossip option for entry {}", sZoneDifficulty->Rewards[category][counter][i].Entry);
                ItemTemplate const* proto = sObjectMgr->GetItemTemplate(sZoneDifficulty->Rewards[category][counter][i].Entry);
                std::string name = proto->Name1;
                if (ItemLocale const* leftIl = sObjectMgr->GetItemLocale(sZoneDifficulty->Rewards[category][counter][i].Entry))
                {
                    ObjectMgr::GetLocaleString(leftIl->Name, player->GetSession()->GetSessionDbcLocale(), name);
                }

                AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, name, GOSSIP_SENDER_MAIN, (1000 * category) + (100 * counter) + i);
                //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: AddingGossipItem with action {}", (1000 * category) + (100 * counter) + i);
            }
        }
        else if (action < 100000)
        {
            uint32 category = 0;
            uint32 itemType = 0;
            uint32 counter = action;
            while (counter > 999)
            {
                ++category;
                counter = counter - 1000;
            }
            while (counter > 99)
            {
                ++itemType;
                counter = counter - 100;
            }
            //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: Handling item with category {}, itemType {}, counter {}", category, itemType, counter);

            // Check if the player has enough score in the respective category.
            uint32 availableScore = 0;
            if (sZoneDifficulty->HardmodeScore.find(player->GetGUID().GetCounter()) != sZoneDifficulty->HardmodeScore.end())
            {
                if (sZoneDifficulty->HardmodeScore[player->GetGUID().GetCounter()].find(category) != sZoneDifficulty->HardmodeScore[player->GetGUID().GetCounter()].end())
                {
                    availableScore = sZoneDifficulty->HardmodeScore[player->GetGUID().GetCounter()][category];
                }
            }

            if (availableScore < sZoneDifficulty->Rewards[category][itemType][counter].Price)
            {
                npcText = NPC_TEXT_DENIED;
                SendGossipMenuFor(player, npcText, creature);
                std::string whisper;
                whisper.append("I am sorry, time-traveler. This item costs ");
                whisper.append(std::to_string(sZoneDifficulty->Rewards[category][itemType][counter].Price));
                whisper.append(" score but you only have ");
                whisper.append(std::to_string(sZoneDifficulty->HardmodeScore[category][player->GetGUID().GetCounter()]));
                whisper.append(" ");
                whisper.append(sZoneDifficulty->GetContentTypeString(category));
                creature->Whisper(whisper, LANG_UNIVERSAL, player);
                return true;
            }

            npcText = NPC_TEXT_CONFIRM;
            ItemTemplate const* proto = sObjectMgr->GetItemTemplate(sZoneDifficulty->Rewards[category][itemType][counter].Entry);
            std::string gossip;
            std::string name = proto->Name1;
            if (ItemLocale const* leftIl = sObjectMgr->GetItemLocale(sZoneDifficulty->Rewards[category][itemType][counter].Entry))
            {
                ObjectMgr::GetLocaleString(leftIl->Name, player->GetSession()->GetSessionDbcLocale(), name);
            }
            gossip.append("Yes, ").append(name).append(" is the item i want.");
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "No!", GOSSIP_SENDER_MAIN, 999998);
            AddGossipItemFor(player, GOSSIP_ICON_VENDOR, gossip, GOSSIP_SENDER_MAIN, 100000 + (1000 * category) + (100 * itemType) + counter);
            //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: AddingGossipItem with action {}", 100000 + (1000 * category) + (100 * itemType) + counter);
        }
        else if (action > 100000)
        {
            npcText = NPC_TEXT_GRANT;
            uint32 category = 0;
            uint32 itemType = 0;
            uint32 counter = action;
            counter = counter - 100000;
            while (counter > 999)
            {
                ++category;
                counter = counter - 1000;
            }
            while (counter > 99)
            {
                ++itemType;
                counter = counter - 100;
            }

            // Check (again) if the player has enough score in the respective category.
            uint32 availableScore = 0;
            if (sZoneDifficulty->HardmodeScore.find(player->GetGUID().GetCounter()) != sZoneDifficulty->HardmodeScore.end())
            {
                if (sZoneDifficulty->HardmodeScore[player->GetGUID().GetCounter()].find(category) != sZoneDifficulty->HardmodeScore[player->GetGUID().GetCounter()].end())
                {
                    availableScore = sZoneDifficulty->HardmodeScore[player->GetGUID().GetCounter()][category];
                }
            }
            if (availableScore < sZoneDifficulty->Rewards[category][itemType][counter].Price)
            {
                return true;
            }

            // Check if the player has the neccesary achievement
            if (sZoneDifficulty->Rewards[category][itemType][counter].Achievement != 0)
            {
                if (!player->HasAchieved(sZoneDifficulty->Rewards[category][itemType][counter].Achievement))
                {
                    std::string gossip = "You do not have the required achievement with ID ";
                    gossip.append(std::to_string(sZoneDifficulty->Rewards[category][itemType][counter].Achievement));
                    gossip.append(" to receive this item. Before i can give it to you, you need to complete the whole dungeon where it can be obtained.");
                    creature->Whisper(gossip, LANG_UNIVERSAL, player);
                    /* debug
                     * LOG_INFO("module", "MOD-ZONE-DIFFICULTY: Player missing achiement with ID {} to obtain item with category {}, itemType {}, counter {}",
                     *    sZoneDifficulty->Rewards[category][itemType][counter].Achievement, category, itemType, counter);
                     * end debug
                     */
                    CloseGossipMenuFor(player);
                    return true;
                }
            }

            //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: Sending item with category {}, itemType {}, counter {}", category, itemType, counter);
            sZoneDifficulty->DeductHardmodeScore(player, category, sZoneDifficulty->Rewards[category][itemType][counter].Price);
            sZoneDifficulty->SendItem(player, category, itemType, counter);
        }

        SendGossipMenuFor(player, npcText, creature);
        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: OnGossipHelloRewardNpc");
        uint32 npcText = NPC_TEXT_OFFER;
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "|TInterface\\icons\\inv_misc_questionmark:15|t Can you please remind me of my score?", GOSSIP_SENDER_MAIN, 999999);

        for (auto& typedata : sZoneDifficulty->Rewards)
        {
            //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: typedata.first is {}", typedata.first);
            if (typedata.first != 0)
            {
                std::string gossip;
                std::string typestring = sZoneDifficulty->GetContentTypeString(typedata.first);
                gossip.append("I want to redeem rewards ").append(typestring);
                //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: typestring is: {} gossip is: {}", typestring, gossip);
                // typedata.first is the ContentType
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, gossip, GOSSIP_SENDER_MAIN, typedata.first);
            }
        }

        SendGossipMenuFor(player, npcText, creature);
        return true;
    }
};

class mod_zone_difficulty_dungeonmaster : public CreatureScript
{
public:
    mod_zone_difficulty_dungeonmaster() : CreatureScript("mod_zone_difficulty_dungeonmaster") { }

    struct mod_zone_difficulty_dungeonmasterAI : public ScriptedAI
    {
        mod_zone_difficulty_dungeonmasterAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: mod_zone_difficulty_dungeonmasterAI: Reset happens.");
            if (me->GetMap() && me->GetMap()->IsHeroic() && !me->GetMap()->IsRaid())
            {
                //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: We're inside a heroic 5man now.");
                //todo: add the list for the wotlk heroic dungeons quests
                for (auto& quest : sZoneDifficulty->DailyHeroicQuests)
                {
                    //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: Checking quest {} and MapId {}", quest, me->GetMapId());
                    if (sPoolMgr->IsSpawnedObject<Quest>(quest))
                    {
                        if (sZoneDifficulty->HeroicTBCQuestMapList[me->GetMapId()] == quest)
                        {
                            //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: mod_zone_difficulty_dungeonmasterAI: Quest with id {} is active.", quest);
                            me->SetPhaseMask(1, true);

                            _scheduler.Schedule(15s, [this](TaskContext /*context*/)
                                {
                                    me->Yell("If you want a challenge, please talk to me soon adventurer!", LANG_UNIVERSAL);
                                });
                            _scheduler.Schedule(45s, [this](TaskContext /*context*/)
                                {
                                    me->Yell("I will take my leave then and offer my services to other adventurers. See you again!", LANG_UNIVERSAL);
                                });
                            _scheduler.Schedule(60s, [this](TaskContext /*context*/)
                                {
                                    me->DespawnOrUnsummon();
                                });
                            break;
                        }
                    }
                }
            }
        }

        void UpdateAI(uint32 diff) override
        {
            _scheduler.Update(diff);
        }

    private:
        TaskScheduler _scheduler;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new mod_zone_difficulty_dungeonmasterAI(creature);
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        uint32 instanceId = player->GetMap()->GetInstanceId();
        if (action == 100)
        {
            //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: Try turn on");
            bool canTurnOn = true;

            // Forbid turning hardmode on ...
            // ...if a single encounter was completed on normal mode
            if (sZoneDifficulty->HardmodeInstanceData.find(instanceId) != sZoneDifficulty->HardmodeInstanceData.end())
            {
                if (player->GetInstanceScript()->GetBossState(0) == DONE)
                {
                    //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: Hardmode is not Possible for instanceId {}", instanceId);
                    canTurnOn = false;
                    creature->Whisper("I am sorry, time-traveler. You can not return to this version of the time-line anymore. You have already completed one of the lessons.", LANG_UNIVERSAL, player);
                    sZoneDifficulty->SaveHardmodeInstanceData(instanceId);
                }
            }
            // ... if there is an encounter in progress
            else if (player->GetInstanceScript() && player->GetInstanceScript()->IsEncounterInProgress())
            {
                //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: IsEncounterInProgress");
                canTurnOn = false;
                creature->Whisper("I am sorry, time-traveler. You can not return to this version of the time-line currently. There is already a battle in progress.", LANG_UNIVERSAL, player);
            }

            if (canTurnOn)
            {
                //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: Turn on hardmode for id {}", instanceId);
                sZoneDifficulty->HardmodeInstanceData[instanceId] = true;
                sZoneDifficulty->SaveHardmodeInstanceData(instanceId);
                sZoneDifficulty->SendWhisperToRaid("We're switching to the challenging version of the history lesson now. (Hard mode)", creature, player);
            }

            CloseGossipMenuFor(player);
        }
        else if (action == 101)
        {
            if (player->GetInstanceScript()->GetBossState(0) != DONE)
            {
                //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: Turn off hardmode for id {}", instanceId);
                sZoneDifficulty->HardmodeInstanceData[instanceId] = false;
                sZoneDifficulty->SaveHardmodeInstanceData(instanceId);
                sZoneDifficulty->SendWhisperToRaid("We're switching to the cinematic version of the history lesson now. (Normal mode)", creature, player);
                CloseGossipMenuFor(player);
            }
            else
            {
                ClearGossipMenuFor(player);
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Yes, i am sure. I know we can not go back to the harder version anymore, but we still want to stick with the less challenging route.", GOSSIP_SENDER_MAIN, 102);
                SendGossipMenuFor(player, NPC_TEXT_LEADER_FINAL, creature);
            }
        }
        else if (action == 102)
        {
            //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: Turn off hardmode for id {}", instanceId);
            sZoneDifficulty->HardmodeInstanceData[instanceId] = false;
            sZoneDifficulty->SaveHardmodeInstanceData(instanceId);
            sZoneDifficulty->SendWhisperToRaid("We're switching to the cinematic version of the history lesson now. (Normal mode)", creature, player);
            CloseGossipMenuFor(player);
        }

        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: OnGossipHelloChromie");
        uint32 npcText = NPC_TEXT_OTHER;
        if (Group* group = player->GetGroup())
        {
            //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: OnGossipHello Has Group");
            if (group->IsLeader(player->GetGUID()))
            {
                //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: OnGossipHello Is Leader");
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Please Chromie, let us re-experience how all the things really happened back then. (Hard mode)", GOSSIP_SENDER_MAIN, 100);
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "I think we will be fine with the cinematic version from here. (Normal mode)", GOSSIP_SENDER_MAIN, 101);

                if (sZoneDifficulty->HardmodeInstanceData[player->GetMap()->GetInstanceId()])
                {
                    npcText = NPC_TEXT_LEADER_HARD;
                }
                else
                {
                    npcText = NPC_TEXT_LEADER_NORMAL;
                }
            }
            else
            {
                creature->Whisper("I will let the leader of your group decide about this subject. You will receive a notification, when they make a new request to me.", LANG_UNIVERSAL, player);
            }
        }
        SendGossipMenuFor(player, npcText, creature);
        return true;
    }
};

class mod_zone_difficulty_allcreaturescript : public AllCreatureScript
{
public:
    mod_zone_difficulty_allcreaturescript() : AllCreatureScript("mod_zone_difficulty_allcreaturescript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        // Heavily inspired by https://github.com/azerothcore/mod-autobalance/blob/1d82080237e62376b9a030502264c90b5b8f272b/src/AutoBalance.cpp
        if (!creature || !creature->GetMap())
        {
            return;
        }

        if ((creature->IsHunterPet() || creature->IsPet() || creature->IsSummon()) && creature->IsControlledByPlayer())
        {
            return;
        }

        uint32 mapId = creature->GetMapId();
        if (sZoneDifficulty->NerfInfo.find(mapId) == sZoneDifficulty->NerfInfo.end())
        {
            return;
        }

        if (!creature->IsAlive())
        {
            return;
        }

        Map* map = creature->GetMap();
        if (!map->IsRaid() &&
            (!(map->IsHeroic() && map->IsDungeon())))
        {
            return;
        }

        CreatureTemplate const* creatureTemplate = creature->GetCreatureTemplate();
        //skip critters and special creatures (spell summons etc.) in instances
        if (creatureTemplate->maxlevel <= 1)
        {
            return;
        }

        CreatureBaseStats const* origCreatureStats = sObjectMgr->GetCreatureBaseStats(creature->GetLevel(), creatureTemplate->unit_class);
        uint32 baseHealth = origCreatureStats->GenerateHealth(creatureTemplate);
        uint32 newHp;
        uint32 entry = creature->GetEntry();
        //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: Checking hp for creature with entry {}.", entry);
        if (sZoneDifficulty->CreatureOverrides.find(entry) == sZoneDifficulty->CreatureOverrides.end())
        {
            if (creature->IsDungeonBoss())
            {
                return;
            }
            newHp = round(baseHealth * sZoneDifficulty->HardmodeHpModifier);
        }
        else
        {
            newHp = round(baseHealth * sZoneDifficulty->CreatureOverrides[entry]);
        }

        uint32 phaseMask = creature->GetPhaseMask();
        int matchingPhase = sZoneDifficulty->GetLowestMatchingPhase(creature->GetMapId(), phaseMask);
        int8 mode = sZoneDifficulty->NerfInfo[mapId][matchingPhase].Enabled;
        if (matchingPhase != -1)
        {
            if (sZoneDifficulty->HasHardMode(mode) && sZoneDifficulty->HardmodeInstanceData[creature->GetMap()->GetInstanceId()])
            {
                if (creature->GetMaxHealth() == newHp)
                {
                    return;
                }
                if (sZoneDifficulty->IsDebugInfoEnabled)
                {
                    //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: Modify creature hp for hard mode: {} to {}", baseHealth, newHp);
                }
                bool hpIsFull = false;
                if (creature->GetHealthPct() >= 100)
                {
                    hpIsFull = true;
                }
                creature->SetMaxHealth(newHp);
                creature->SetCreateHealth(newHp);
                creature->SetModifierValue(UNIT_MOD_HEALTH, BASE_VALUE, (float)newHp);
                if (hpIsFull)
                {
                    creature->SetHealth(newHp);
                }
                creature->UpdateAllStats();
                return;
            }

            if (sZoneDifficulty->HardmodeInstanceData[creature->GetMap()->GetInstanceId()] == false)
            {
                if (creature->GetMaxHealth() == newHp)
                {
                    if (sZoneDifficulty->IsDebugInfoEnabled)
                    {
                        //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: Modify creature hp for normal mode: {} to {}", baseHealth, baseHealth);
                    }
                    bool hpIsFull = false;
                    if (creature->GetHealthPct() >= 100)
                    {
                        hpIsFull = true;
                    }
                    creature->SetMaxHealth(baseHealth);
                    creature->SetCreateHealth(baseHealth);
                    creature->SetModifierValue(UNIT_MOD_HEALTH, BASE_VALUE, (float)baseHealth);
                    if (hpIsFull)
                    {
                        creature->SetHealth(baseHealth);
                    }
                    creature->UpdateAllStats();
                    return;
                }
            }
        }
    }
};

// Add all scripts in one
void AddModZoneDifficultyScripts()
{
    new mod_zone_difficulty_unitscript();
    new mod_zone_difficulty_playerscript();
    new mod_zone_difficulty_petscript();
    new mod_zone_difficulty_worldscript();
    new mod_zone_difficulty_globalscript();
    new mod_zone_difficulty_rewardnpc();
    new mod_zone_difficulty_dungeonmaster();
    new mod_zone_difficulty_allcreaturescript();
}
