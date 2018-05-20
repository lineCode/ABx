#pragma once

namespace AB {
namespace Entities {
namespace Limits {
// General
static constexpr int MAX_UUID = 36;
static constexpr int MAX_FILENAME = 260;
// Account
static constexpr int MAX_ACCOUNT_NAME = 32;
static constexpr int MAX_ACCOUNT_PASS = 61;
static constexpr int MAX_ACCOUNT_EMAIL = 60;
static constexpr int MAX_ACCOUNT_CHARACTERS = 50;
// AccountKey
static constexpr int MAX_ACCOUNTKEY_DESCRIPTION = 255;

// Character
static constexpr int MAX_CHARACTER_NAME = 20;
static constexpr int MAX_CHARACTER_PROF = 2;
static constexpr int MAX_CHARACTER_SKILLTEMPLATE = 36;

// Map
static constexpr int MAX_MAP_NAME = 50;
static constexpr int MAX_GAMES = 500;

// Guild
static constexpr int MAX_GUILD_NAME = 32;
static constexpr int MAX_GUILD_TAG = 4;
static constexpr int MAX_GUILD_MEMBERS = 500;

// Ban
static constexpr int MAX_BAN_COMMENT = 255;

// Version
static constexpr int MAX_VERSION_NAME = 20;

// Friendlist
static constexpr int MAX_FRIENDS = 1000;

// Mail
static constexpr int MAX_MAIL_SUBJECT = 60;
static constexpr int MAX_MAIL_MESSAGE = 255;
static constexpr int MAX_MAIL_COUNT = 500;

// Professions
static constexpr int MAX_PROFESSION_NAME = 32;
static constexpr int MAX_PROFESSION_ABBR = 2;
static constexpr int MAX_PROFESSION_ATTRIBUTES = 5;
static constexpr int MAX_PROFESSIONS = 24;                            // Max professions in game
// Attribute
static constexpr int MAX_ATTRIBUTE_NAME = 32;
static constexpr int MAX_ATTRIBUTES = 64;
// Skill
static constexpr int MAX_SKILL_NAME = 64;
static constexpr int MAX_SKILL_DESCRIPTION = 255;
static constexpr int MAX_SKILL_SHORT_DESCRIPTION = 255;
static constexpr int MAX_SKILLS = 4096;                            // Max skills in game
// Effect
static constexpr int MAX_EFFECT_NAME = 64;
static constexpr int MAX_EFFECTS = 4096;                            // Max effects in game
// Item
static constexpr int MAX_ITEM_NAME = 64;

}
}
}
