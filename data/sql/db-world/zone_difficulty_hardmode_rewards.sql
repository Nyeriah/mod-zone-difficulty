-- Content types:
-- TYPE_VANILLA = 1;
-- TYPE_RAID_MC = 2;
-- TYPE_RAID_ONY = 3;
-- TYPE_RAID_BWL = 4;
-- TYPE_RAID_ZG = 5;
-- TYPE_RAID_AQ20 = 6;
-- TYPE_RAID_AQ40 = 7;
-- TYPE_HEROIC_TBC = 8;
-- TYPE_RAID_T4 = 9;
-- TYPE_RAID_T5 = 10;
-- TYPE_RAID_T6 = 11;
-- TYPE_HEROIC_WOTLK = 12;
-- TYPE_RAID_T7 = 13;
-- TYPE_RAID_T8 = 14;
-- TYPE_RAID_T9 = 15;
-- TYPE_RAID_T10 = 16;

-- Item types:
-- Back, Finger, Trinket, Neck = 1
-- Cloth = 2
-- Leather = 3
-- Mail = 4
-- Plate = 5
-- Weapons, Holdables, Shields = 6

DROP TABLE IF EXISTS `zone_difficulty_hardmode_rewards`;
CREATE TABLE `zone_difficulty_hardmode_rewards` (
    `ContentType` INT NOT NULL DEFAULT 0,
    `ItemType` INT NOT NULL DEFAULT 0,
    `Entry` INT NOT NULL DEFAULT 0,
    `Price` INT NOT NULL DEFAULT 0,
    `Enchant` INT NOT NULL DEFAULT 0,
    `EnchantSlot` TINYINT NOT NULL DEFAULT 0,
    `Enabled` TINYINT DEFAULT 0,
	`Comment` TEXT,
	PRIMARY KEY (`ContentType`, `Entry`, `Enchant`)
);

INSERT INTO `zone_difficulty_hardmode_rewards` (ContentType, ItemType, Entry, Price, Enchant, EnchantSlot, Enabled, Comment) VALUES
-- TYPE_HEROIC_TBC = 8;
-- Back, Finger, Trinket, Neck = 1
(8, 1, 29347, 10, 847, 11, 1, 'Talisman of the Breaker + 1 AllStats'),
(8, 1, 29349, 10, 847, 11, 1, 'Adamantine Chain of the Unbroken + 1 AllStats'),
(8, 1, 29352, 10, 847, 11, 1, 'Cobalt Band of Tyrigosa + 1 AllStats'),
(8, 1, 31919, 10, 847, 11, 1, 'Nexus-Prince\'s Ring of Balance + 1 AllStats'),
(8, 1, 31920, 10, 847, 11, 1, 'Shaffar\'s Band of Brutality + 1 AllStats'),
(8, 1, 31921, 10, 847, 11, 1, 'Yor\'s Collapsing Band + 1 AllStats'),
(8, 1, 31922, 10, 847, 11, 1, 'Ring of Conflict Survival + 1 AllStats'),
(8, 1, 31923, 10, 847, 11, 1, 'Band of the Crystalline Void + 1 AllStats'),
(8, 1, 31924, 10, 847, 11, 1, 'Yor\'s Revenge + 1 AllStats'),
(8, 1, 32081, 10, 847, 11, 1, 'Eye of the Stalker + 1 AllStats'),
(8, 1, 29354, 10, 847, 11, 1, 'Light-Touched Stole of Altruism + 1 AllStats'),
-- Cloth = 2
(8, 2, 29240, 10, 847, 11, 1, 'Bands of Negation + 1 AllStats'),
(8, 2, 29241, 10, 847, 11, 1, 'Belt of Depravity + 1 AllStats'),
(8, 2, 29242, 10, 847, 11, 1, 'Boots of Blasphemy + 1 AllStats'),
(8, 2, 29249, 10, 847, 11, 1, 'Bands of the Benevolent + 1 AllStats'),
(8, 2, 29250, 10, 847, 11, 1, 'Cord of Sanctification + 1 AllStats'),
(8, 2, 29251, 10, 847, 11, 1, 'Boots of the Pious + 1 AllStats'),
(8, 2, 29255, 10, 847, 11, 1, 'Bands of Rarefied Magic + 1 AllStats'),
(8, 2, 29257, 10, 847, 11, 1, 'Sash of Arcane Visions + 1 AllStats'),
(8, 2, 29258, 10, 847, 11, 1, 'Boots of Ethereal Manipulation + 1 AllStats'),
(8, 2, 30531, 10, 847, 11, 1, 'Breeches of the Occultist + 1 AllStats'),
(8, 2, 30532, 10, 847, 11, 1, 'Kirin Tor Master\'s Trousers + 1 AllStats'),
(8, 2, 30543, 10, 847, 11, 1, 'Pontifex Kilt + 1 AllStats'),
-- Leather = 3
(8, 3, 29246, 10, 847, 11, 1, 'Nightfall Wristguards + 1 AllStats'),
(8, 3, 29247, 10, 847, 11, 1, 'Girdle of the Deathdealer + 1 AllStats'),
(8, 3, 29248, 10, 847, 11, 1, 'Shadowstep Striders + 1 AllStats'),
(8, 3, 29263, 10, 847, 11, 1, 'Forestheart Bracers + 1 AllStats'),
(8, 3, 29264, 10, 847, 11, 1, 'Tree-Mender\'s Belt + 1 AllStats'),
(8, 3, 29265, 10, 847, 11, 1, 'Barkchip Boots + 1 AllStats'),
(8, 3, 29357, 10, 847, 11, 1, 'Master Thief\'s Gloves + 1 AllStats'),
(8, 3, 30535, 10, 847, 11, 1, 'Forestwalker Kilt + 1 AllStats'),
(8, 3, 30538, 10, 847, 11, 1, 'Midnight Legguards + 1 AllStats'),
(8, 3, 32080, 10, 847, 11, 1, 'Mantle of Shadowy Embrace + 1 AllStats'),
-- Mail = 4
(8, 4, 29243, 10, 847, 11, 1, 'Wave-Fury Vambraces + 1 AllStats'),
(8, 4, 29244, 10, 847, 11, 1, 'Wave-Song Girdle + 1 AllStats'),
(8, 4, 29245, 10, 847, 11, 1, 'Wave-Crest Striders + 1 AllStats'),
(8, 4, 29259, 10, 847, 11, 1, 'Bracers of the Hunt + 1 AllStats'),
(8, 4, 29261, 10, 847, 11, 1, 'Girdle of Ferocity + 1 AllStats'),
(8, 4, 29262, 10, 847, 11, 1, 'Boots of the Endless Hunt + 1 AllStats'),
(8, 4, 30534, 10, 847, 11, 1, 'Wyrmscale Greaves + 1 AllStats'),
(8, 4, 30541, 10, 847, 11, 1, 'Stormsong Kilt + 1 AllStats'),
(8, 4, 32076, 10, 847, 11, 1, 'Handguards of the Steady + 1 AllStats'),
(8, 4, 32077, 10, 847, 11, 1, 'Wrath Infused Gauntlets + 1 AllStats'),
(8, 4, 32078, 10, 847, 11, 1, 'Pauldrons of Wild Magic + 1 AllStats'),
-- Plate = 5
(8, 5, 29238, 10, 847, 11, 1, 'Lion\'s Heart Girdle + 1 AllStats'),
(8, 5, 29239, 10, 847, 11, 1, 'Eaglecrest Warboots + 1 AllStats'),
(8, 5, 29252, 10, 847, 11, 1, 'Bracers of Dignity + 1 AllStats'),
(8, 5, 29253, 10, 847, 11, 1, 'Girdle of Valorous Deeds + 1 AllStats'),
(8, 5, 29254, 10, 847, 11, 1, 'Boots of the Righteous Path + 1 AllStats'),
(8, 5, 29463, 10, 847, 11, 1, 'Amber Bands of the Aggressor + 1 AllStats'),
(8, 5, 30533, 10, 847, 11, 1, 'Vanquisher\'s Legplates + 1 AllStats'),
(8, 5, 30536, 10, 847, 11, 1, 'Greaves of the Martyr + 1 AllStats'),
(8, 5, 32072, 10, 847, 11, 1, 'Gauntlets of Dissension + 1 AllStats'),
(8, 5, 32073, 10, 847, 11, 1, 'Spaulders of Dementia + 1 AllStats'),
-- Weapons, Holdables, Shields = 6
(8, 6, 32082, 10, 847, 11, 1, 'The Fel Barrier + 1 AllStats'),
(8, 6, 29362, 10, 847, 11, 1, 'The Sun Eater + 1 AllStats'),
(8, 6, 29356, 10, 847, 11, 1, 'Quantum Blade + 1 AllStats'),
(8, 6, 29355, 10, 847, 11, 1, 'Terokk\'s Shadowstaff + 1 AllStats'),
(8, 6, 29359, 10, 847, 11, 1, 'Feral Staff of Lashing + 1 AllStats'),
(8, 6, 29348, 10, 847, 11, 1, 'The Bladefist + 1 AllStats'),
(8, 6, 29346, 10, 847, 11, 1, 'Feltooth Eviscerator + 1 AllStats'),
(8, 6, 29360, 10, 847, 11, 1, 'Vileblade of the Betrayer + 1 AllStats'),
(8, 6, 29350, 10, 847, 11, 1, 'The Black Stalk + 1 AllStats'),
(8, 6, 29351, 10, 847, 11, 1, 'Wrathtide Longbow + 1 AllStats'),
(8, 6, 29353, 10, 847, 11, 1, 'Shockwave Truncheon + 1 AllStats'),

-- TYPE_RAID_T4 = 9;
(9, 1, 19965, 10, 847, 11, 1, 'Testitem + 1 AllStats');