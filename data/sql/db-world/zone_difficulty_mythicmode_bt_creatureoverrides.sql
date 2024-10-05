DROP TABLE IF EXISTS `zone_difficulty_mythicmode_creatureoverrides`;
CREATE TABLE `zone_difficulty_mythicmode_creatureoverrides` (
    `CreatureEntry` INT NOT NULL DEFAULT 0,
    `HPModifier` FLOAT NOT NULL DEFAULT 1,
    `Enabled` TINYINT DEFAULT 1,
	`Comment` TEXT
);

DELETE FROM `zone_difficulty_mythicmode_creatureoverrides` WHERE CreatureEntry IN (22887, 22898, 22841, 22871, 22948, 22856, 23418, 23419, 23420, 22947, 23426, 22917);
INSERT INTO `zone_difficulty_mythicmode_creatureoverrides` (`CreatureEntry`, `HPModifier`, `Enabled`, `Comment`) VALUES
(22887, 3.5, 1, 'Najentus, Black Temple HPx3.5'),
(22898, 3.5, 1, 'Supremus, Black Temple HPx3.5'),
(22841, 3.5, 1, 'Shade of Akama, Black Temple HPx3.5'),
(22871, 3.2, 1, 'Teron Gorefiend, Black Temple HPx3.2'),
(22948, 3.2, 1, 'Gurtogg Bloodboil, Black Temple HPx3.2'),
(22856, 3, 1, 'Reliquary of the Lost, Black Temple HPx3'),
(23418, 3, 1, 'Essence of Suffering, Black Temple HPx3'),
(23419, 2, 1, 'Essence of Desire, Black Temple HPx2'),
(23420, 3, 1, 'Essence of Anger, Black Temple HPx3'),
(22947, 3.5, 1, 'Mother Shahraz, Black Temple HPx3.5'),
(23426, 3.5, 1, 'Illidari Council, Black Temple HPx3.5'),
(22917, 3.2, 1, 'Illidan, Black Temple HPx3.2');