#pragma once

#include "hd.h"
// Windows.h defines PlaySound as a macro (PlaySoundA/PlaySoundW) which
// collides with Entity::PlaySound. Undefine it so the method name is preserved.
#ifdef PlaySound
#undef PlaySound
#endif

namespace hddll {

// Defined in hd_aibot.h - drives hired-hand / CPU spelunkers.
class AIBot;

enum class Ownership : int32_t {
  Unowned = -99,
  HiredHand = -1,
  Player1 = 0,
  Player2 = 1,
  Player3 = 2,
  Player4 = 3
};

enum class EntityKind : int32_t {
  KIND_ENTITY = -99,
  KIND_FLOOR = 0,
  KIND_ACTIVE = 1,
  KIND_PLAYER = 2,
  KIND_MONSTER = 3,
  KIND_ITEM = 4,
  KIND_BACKGROUND = 5,
  KIND_EXPLOSION = 6
};

// The per-frame controller state for a player. For human players it is filled
// from the keyboard/gamepad; for hired hands / CPU players the AIBot writes it
// every frame (see ai_bot_shift_input_state + ai_bot_walk_path /
// ai_bot_execute_combat_action). Reading it back is the ground truth for "what
// is the bot actually pressing right now".
//
// Per-frame controller state for one player. Filled each frame by the engine's
// input processor for human players, or written directly by AIBot for CPU
// players / hired hands.
//
// The action_* bytes are 1 while the action is held this frame, 0 otherwise;
// prev_action_* hold the previous frame's value so the engine can detect
// press/release edges. left_right / up_down are the movement stick:
// -32 (0xFFE0), 0, or +32 (0x20).
//
// IMPORTANT: the action *index* (the 0..9 ordering below) is FIXED -- it is the
// XInput button slot and is NOT affected by key rebinding. Rebinding only
// changes which physical key/pad button feeds a given index. The mapping was
// verified against Spelunky.exe (controls_init + the XInput input path + the
// jump.wav consumer in the EntityPlayer update vfunc):
//
//   index 0 jump   (pad A)     index 5 door   (pad RB)  purchase / enter door
//   index 1 bomb   (pad B)     index 6 lt     (pad LT)  see note below
//   index 2 whip   (pad X)     index 7 run    (pad RT)
//   index 3 rope   (pad Y)     index 8 pause  (pad START)
//   index 4 shoulder-l (pad LB)index 9 journal(pad BACK) map / journal
//
// index 6 (action_lt) is written from the gamepad left trigger every frame but
// has NO consumer anywhere in the engine -- a dead / write-only slot. index 4
// (action_shoulder_l) is confirmed as the LB slot; its exact in-game role is
// less certain, hence the generic name.
struct PlayerInput {
  uint8_t action_jump;            // 0x00  index 0  (pad A)
  uint8_t action_bomb;            // 0x01  index 1  (pad B)  use bomb / use item
  uint8_t action_whip;            // 0x02  index 2  (pad X)  attack / throw held
  uint8_t action_rope;            // 0x03  index 3  (pad Y)
  uint8_t action_shoulder_l;      // 0x04  index 4  (pad LB)
  uint8_t action_door;            // 0x05  index 5  (pad RB)  purchase / door
  uint8_t prev_action_jump;       // 0x06
  uint8_t prev_action_bomb;       // 0x07
  uint8_t prev_action_whip;       // 0x08
  uint8_t prev_action_rope;       // 0x09
  uint8_t prev_action_shoulder_l; // 0x0A
  uint8_t prev_action_door;       // 0x0B
  uint8_t action_lt;              // 0x0C  index 6  (pad LT) -- set, never read
  uint8_t action_run;             // 0x0D  index 7  (pad RT)
  uint8_t prev_action_lt;         // 0x0E
  uint8_t prev_action_run;        // 0x0F
  uint8_t action_pause;           // 0x10  index 8  (pad START)
  uint8_t action_journal;         // 0x11  index 9  (pad BACK)  map / journal
  uint8_t prev_action_pause;      // 0x12
  uint8_t prev_action_journal;    // 0x13
  uint8_t unknown_14[4];          // 0x14
  int32_t input_type;             // 0x18  1 = keyboard, 2 = gamepad / XInput
  int32_t left_right;             // 0x1C  -32 / 0 / +32
  int32_t up_down;                // 0x20  -32 / 0 / +32
  // 0x24..0x38: NOT raw button codes. These are the rebind layer -- each named
  // control stores the action *index* (0..9) it is bound to. controls_init
  // defaults: whip->2, jump->0, bomb->1, rope->3, run->7, door->5. The engine
  // resolves these via resolve_action_index() before touching the action_*
  // bytes above. (A parallel gamepad copy lives elsewhere in the Controls
  // struct; this block is the keyboard path.)
  int32_t remap_whip;             // 0x24
  int32_t remap_jump;             // 0x28
  int32_t remap_bomb;             // 0x2C
  int32_t remap_rope;             // 0x30
  int32_t remap_run;              // 0x34
  int32_t remap_purchase_door;    // 0x38
  int32_t unknown_3C[11];         // 0x3C
  // 0x68..0x7C: raw DirectInput keyboard scancodes for each control. These ARE
  // what key rebinding edits. controls_init defaults: whip=0x2D 'X',
  // jump=0x2C 'Z', bomb=0x1F 'S', rope=0x1E 'A', run=0x2A LShift,
  // purchase_door=0x39 Space.
  int32_t key_whip;               // 0x68
  int32_t key_jump;               // 0x6C
  int32_t key_bomb;               // 0x70
  int32_t key_rope;               // 0x74
  int32_t key_run;                // 0x78
  int32_t key_purchase_door;      // 0x7C
  int32_t unknown_80[2];          // 0x80
  int32_t key_up;                 // 0x88
  int32_t key_down;               // 0x8C
  int32_t key_left;               // 0x90
  int32_t key_right;              // 0x94
};
static_assert(sizeof(PlayerInput) == 0x98);

class Entity {
public:
  void **vfTable;
  int entity_index;
  EntityKind entity_kind;
  uint32_t entity_type;
  int bin_x;
  int bin_y;
  Ownership owner;
  int field6_0x18;
  int field7_0x1c;
  int field8_0x20;
  int field9_0x24;
  int z_depth_as_int;
  float x;
  float y;
  float width;
  float height;
  float current_z;
  float original_z;
  float alpha;
  float hitbox_up;
  float hitbox_down;
  float hitbox_x;
  float field21_0x54;
  float angle;
  float field23_0x5c;
  float field24_0x60;
  float field25_0x64;
  float texture_draw_bound_left; /* from 0 to 1 */
  float texture_draw_bound_top;
  float texture_draw_bound_right;
  float texture_draw_bound_bottom;
  float field30_0x78;
  float field31_0x7c;
  float field32_0x80;
  float brightness;
  float red;
  float green;
  float blue;
  float field37_0x94;
  char flag_deletion;
  char flag_horizontal_flip;
  char flag_3;
  char flag_4;
  char flag_5;
  char flag_6;
  char flag_7;
  char flag_8;
  char flag_9;
  char flag_10;
  char flag_11;
  char flag_12;
  char flag_13;
  char flag_14;
  char flag_15;
  char flag_16;
  char flag_17;
  char flag_18;
  char flag_19;
  char flag_20;
  char flag_21;
  char flag_22;
  char flag_23;
  char flag_24;
  class TextureDefinition *texture_definition;
  int field63_0xb4;
  class Entity *deco_over;
  class Entity *deco_top;
  class Entity *deco_bottom;
  class Entity *deco_left;
  class Entity *deco_right;
  short field69_0xcc;
  char unused_maybe[94];
  int field71_0x12c;

  const char *KindName();
  const char *TypeName();
  void PlaySound(const char *audioName);
};
const char *EntityTypeName(uint32_t);

class EntityActive : public Entity {
public:
  int field1_0x130;
  int field2_0x134;
  int field3_0x138;
  int health;
  int field5_0x140;
  int field6_0x144;
  int favor_given;
  int field8_0x14c;
  int field9_0x150;
  int field10_0x154;
  int field11_0x158;
  int field12_0x15c;
  float field13_0x160;
  float field14_0x164;
  float field15_0x168;
  float field16_0x16c;
  float field17_0x170;
  float field18_0x174;
  float field19_0x178;
  float field20_0x17c;
  int stun_timer;
  float field22_0x184;
  float field23_0x188;
  float field24_0x18c;
  float field25_0x190;
  float field26_0x194;
  int field27_0x198;
  float field28_0x19c;
  float field29_0x1a0;
  float field30_0x1a4;
  float field31_0x1a8;
  float field32_0x1ac;
  float field33_0x1b0;
  float field34_0x1b4;
  float field35_0x1b8;
  float field36_0x1bc;
  float field37_0x1c0;
  float field38_0x1c4;
  float time_in_air;
  int field40_0x1cc;
  int field41_0x1d0;
  int field42_0x1d4;
  int field43_0x1d8;
  int field44_0x1dc;
  int field45_0x1e0;
  class TextureDefinition *field46_0x1e4;
  int field47_0x1e8;
  char field48_0x1ec;
  char field49_0x1ed; // collidable/"real" - AIBot scan_world ignores entity if 0
  char field50_0x1ee;
  char field51_0x1ef;
  char field52_0x1f0; // when != 0 the AIBot suspends combat/target/walk for this frame
  char field53_0x1f1;
  char field54_0x1f2;
  char field55_0x1f3;
  char field56_0x1f4;
  char field57_0x1f5;
  char field58_0x1f6;
  char field59_0x1f7;
  char field60_0x1f8;
  char field61_0x1f9;
  char field62_0x1fa;
  char field63_0x1fb;
  char field64_0x1fc;
  char field65_0x1fd;
  char field66_0x1fe;
  char no_gravity;
  char field68_0x200; // AIBot movement gate (climb/ladder state - see ai_bot_walk_path)
  char field69_0x201;
  char field70_0x202;
  char field71_0x203; // AIBot movement gate (hanging/ledge state - see ai_bot_walk_path)
  char field72_0x204;
  char field73_0x205; // AIBot movement gate (on-rope state - see ai_bot_walk_path)
  char field74_0x206;
  char field75_0x207;
  char field76_0x208;
  char field77_0x209;
  char field78_0x20a;
  char field79_0x20b;
  char field80_0x20c;
  char stunned;
  char field82_0x20e;
  char field83_0x20f;
  char field84_0x210;
  char field85_0x211;
  char field86_0x212;
  char field87_0x213;
  char field88_0x214;
  char field89_0x215;
  char field90_0x216;
  char field91_0x217;
  char field92_0x218;
  char field93_0x219;
  char field94_0x21a;
  char field95_0x21b;
  int field96_0x21c;
  PlayerInput *pPlayer_input; // 0x220 the controller state; AIBot writes this every frame
  int field98_0x224;
  int field99_0x228;
  class EntityActive *holder_entity;
  class EntityActive *holding_entity;
  class EntityActive *backitem_entity;
  int field103_0x238;
  int field104_0x23c;
  float velocity_x;
  float velocity_y;
  int field107_0x248;
  int field108_0x24c;
  int field109_0x250;
  int field110_0x254;
  int field111_0x258;
  int field112_0x25c;
  int field113_0x260;
  int field114_0x264;
};
class EntityBackground : public Entity {
public:
  int field2_0x134;
  int field3_0x138;
  int field4_0x13c;
  int field5_0x140;
  int field6_0x144;
  float field7_0x148;
  float field8_0x14c;
  float field9_0x150;
  float field10_0x154;
  float field11_0x158;
  float field12_0x15c;
  int field13_0x160;
  int field14_0x164;
};
class EntityExplosion : public EntityActive {
public:
};
class EntityFloor : public Entity {
public:
  int field2_0x134;
  int field3_0x138;
  int field4_0x13c;
  char field5_0x140;
  char field6_0x141;
  char field7_0x142;
  char field8_0x143;
  char shopkeeper_tile;
  char under_door;
  char field11_0x146;
  char field12_0x147;
  char field13_0x148;
  char field14_0x149;
  char field15_0x14a;
  char field16_0x14b;
  char field17_0x14c;
  char field18_0x14d;
  char field19_0x14e;
  char field20_0x14f;
  char field21_0x150;
  char field22_0x151;
  char field23_0x152;
  char field24_0x153;
  int field25_0x154;
  int field26_0x158;
  int field27_0x15c;
  int field28_0x160;
  int field29_0x164;
  int field30_0x168;
  int field31_0x16c;
  int field32_0x170;
  int field33_0x174;
  int field34_0x178;
  int field35_0x17c;
  int field36_0x180;
  int field37_0x184;
  int field38_0x188;
  int field39_0x18c;
  int field40_0x190;
  int field41_0x194;
  char field42_0x198;
  char field43_0x199;
  char field44_0x19a;
  char field45_0x19b;
  char field46_0x19c;
  char field47_0x19d;
  char field48_0x19e;
  char field49_0x19f;
  char field50_0x1a0;
  char field51_0x1a1;
  char field52_0x1a2;
  char field53_0x1a3;
  char field54_0x1a4;
  char field55_0x1a5;
  char field56_0x1a6;
  char field57_0x1a7;
  char field58_0x1a8;
  char field59_0x1a9;
  char field60_0x1aa;
  char field61_0x1ab;
  char field62_0x1ac;
  char field63_0x1ad;
  char field64_0x1ae;
  char field65_0x1af;
  char field66_0x1b0;
  char field67_0x1b1;
  char field68_0x1b2;
  char field69_0x1b3;
  char field70_0x1b4;
  char field71_0x1b5;
  char field72_0x1b6;
  char field73_0x1b7;
  char field74_0x1b8;
  char field75_0x1b9;
  char field76_0x1ba;
  char field77_0x1bb;
  char field78_0x1bc;
  char field79_0x1bd;
  char field80_0x1be;
  char field81_0x1bf;
  char field82_0x1c0;
  char field83_0x1c1;
  char field84_0x1c2;
  char field85_0x1c3;
  char field86_0x1c4;
  char field87_0x1c5;
  char field88_0x1c6;
  char field89_0x1c7;
  char field90_0x1c8;
  char field91_0x1c9;
  char field92_0x1ca;
  char field93_0x1cb;
  char field94_0x1cc;
  char field95_0x1cd;
  char field96_0x1ce;
  char field97_0x1cf;
  char field98_0x1d0;
  char field99_0x1d1;
  char field100_0x1d2;
  char field101_0x1d3;
  char field102_0x1d4;
  char field103_0x1d5;
  char field104_0x1d6;
  char field105_0x1d7;
  char field106_0x1d8;
  char field107_0x1d9;
  char field108_0x1da;
  char field109_0x1db;
  char field110_0x1dc;
  char field111_0x1dd;
  char field112_0x1de;
  char field113_0x1df;
  char field114_0x1e0;
  char field115_0x1e1;
  char field116_0x1e2;
  char field117_0x1e3;
  char field118_0x1e4;
  char field119_0x1e5;
  char field120_0x1e6;
  char field121_0x1e7;
  char field122_0x1e8;
  char field123_0x1e9;
  char field124_0x1ea;
  char field125_0x1eb;
  char field126_0x1ec;
  char field127_0x1ed;
  char field128_0x1ee;
  char field129_0x1ef;
  char field130_0x1f0;
  char field131_0x1f1;
  char field132_0x1f2;
  char field133_0x1f3;
  char field134_0x1f4;
  char field135_0x1f5;
  char field136_0x1f6;
  char field137_0x1f7;
  int field138_0x1f8;
  char field139_0x1fc;
  char field140_0x1fd;
  char field141_0x1fe;
  char field142_0x1ff;
  char field143_0x200;
  char field144_0x201;
  char field145_0x202;
  char field146_0x203;
  char field147_0x204;
  char field148_0x205;
  char field149_0x206;
  char field150_0x207;
  char field151_0x208;
  char field152_0x209;
  char field153_0x20a;
  char field154_0x20b;
  char field155_0x20c;
  char field156_0x20d;
  char field157_0x20e;
  char field158_0x20f;
  char field159_0x210;
  char field160_0x211;
  char field161_0x212;
  char field162_0x213;
  char field163_0x214;
  char field164_0x215;
  char field165_0x216;
  char field166_0x217;
  char field167_0x218;
  char field168_0x219;
  char field169_0x21a;
  char field170_0x21b;
  char field171_0x21c;
  char field172_0x21d;
  char field173_0x21e;
  char field174_0x21f;
  char field175_0x220;
  char field176_0x221;
  char field177_0x222;
  char field178_0x223;
  char field179_0x224;
  char field180_0x225;
  char field181_0x226;
  char field182_0x227;
  char field183_0x228;
  char field184_0x229;
  char field185_0x22a;
  char field186_0x22b;
  char field187_0x22c;
  char field188_0x22d;
  char field189_0x22e;
  char field190_0x22f;
  char field191_0x230;
  char field192_0x231;
  char field193_0x232;
  char field194_0x233;
  char field195_0x234;
  char field196_0x235;
  char field197_0x236;
  char field198_0x237;
  char field199_0x238;
  char field200_0x239;
  char field201_0x23a;
  char field202_0x23b;
  char field203_0x23c;
  char field204_0x23d;
  char field205_0x23e;
  char field206_0x23f;
  char field207_0x240;
  char field208_0x241;
  char field209_0x242;
  char field210_0x243;
  char field211_0x244;
  char field212_0x245;
  char field213_0x246;
  char field214_0x247;
  char field215_0x248;
  char field216_0x249;
  char field217_0x24a;
  char field218_0x24b;
  char field219_0x24c;
  char field220_0x24d;
  char field221_0x24e;
  char field222_0x24f;
  char field223_0x250;
  char field224_0x251;
  char field225_0x252;
  char field226_0x253;
  char field227_0x254;
  char field228_0x255;
  char field229_0x256;
  char field230_0x257;
  char field231_0x258;
  char field232_0x259;
  char field233_0x25a;
  char field234_0x25b;
  int field235_0x25c;
  char field236_0x260;
  char field237_0x261;
  char field238_0x262;
  char field239_0x263;
  char field240_0x264;
  char field241_0x265;
  char field242_0x266;
  char field243_0x267;
  char field244_0x268;
  char field245_0x269;
  char field246_0x26a;
  char field247_0x26b;
  char field248_0x26c;
  char field249_0x26d;
  char field250_0x26e;
  char field251_0x26f;
  char field252_0x270;
  char field253_0x271;
  char field254_0x272;
  char field255_0x273;
  char field256_0x274;
  char field257_0x275;
  char field258_0x276;
  char field259_0x277;
  char field260_0x278;
  char field261_0x279;
  char field262_0x27a;
  char field263_0x27b;
  char field264_0x27c;
  char field265_0x27d;
  char field266_0x27e;
  char field267_0x27f;
  char field268_0x280;
  char field269_0x281;
  char field270_0x282;
  char field271_0x283;
  char field272_0x284;
  char field273_0x285;
  char field274_0x286;
  char field275_0x287;
  char field276_0x288;
  char field277_0x289;
  char field278_0x28a;
  char field279_0x28b;
  char field280_0x28c;
  char field281_0x28d;
  char field282_0x28e;
  char field283_0x28f;
  char field284_0x290;
  char field285_0x291;
  char field286_0x292;
  char field287_0x293;
  char field288_0x294;
  char field289_0x295;
  char field290_0x296;
  char field291_0x297;
  char field292_0x298;
  char field293_0x299;
  char field294_0x29a;
  char field295_0x29b;
  char field296_0x29c;
  char field297_0x29d;
  char field298_0x29e;
  char field299_0x29f;
  char field300_0x2a0;
  char field301_0x2a1;
  char field302_0x2a2;
  char field303_0x2a3;
  char field304_0x2a4;
  char field305_0x2a5;
  char field306_0x2a6;
  char field307_0x2a7;
  char field308_0x2a8;
  char field309_0x2a9;
  char field310_0x2aa;
  char field311_0x2ab;
  char field312_0x2ac;
  char field313_0x2ad;
  char field314_0x2ae;
  char field315_0x2af;
  char field316_0x2b0;
  char field317_0x2b1;
  char field318_0x2b2;
  char field319_0x2b3;
  char field320_0x2b4;
  char field321_0x2b5;
  char field322_0x2b6;
  char field323_0x2b7;
  char field324_0x2b8;
  char field325_0x2b9;
  char field326_0x2ba;
  char field327_0x2bb;
  char field328_0x2bc;
  char field329_0x2bd;
  char field330_0x2be;
  char field331_0x2bf;
  int field332_0x2c0;
  char field333_0x2c4;
  char field334_0x2c5;
  char field335_0x2c6;
  char field336_0x2c7;
  char field337_0x2c8;
  char field338_0x2c9;
  char field339_0x2ca;
  char field340_0x2cb;
  char field341_0x2cc;
  char field342_0x2cd;
  char field343_0x2ce;
  char field344_0x2cf;
  char field345_0x2d0;
  char field346_0x2d1;
  char field347_0x2d2;
  char field348_0x2d3;
  char field349_0x2d4;
  char field350_0x2d5;
  char field351_0x2d6;
  char field352_0x2d7;
  char field353_0x2d8;
  char field354_0x2d9;
  char field355_0x2da;
  char field356_0x2db;
  char field357_0x2dc;
  char field358_0x2dd;
  char field359_0x2de;
  char field360_0x2df;
  char field361_0x2e0;
  char field362_0x2e1;
  char field363_0x2e2;
  char field364_0x2e3;
  char field365_0x2e4;
  char field366_0x2e5;
  char field367_0x2e6;
  char field368_0x2e7;
  char field369_0x2e8;
  char field370_0x2e9;
  char field371_0x2ea;
  char field372_0x2eb;
  char field373_0x2ec;
  char field374_0x2ed;
  char field375_0x2ee;
  char field376_0x2ef;
  char field377_0x2f0;
  char field378_0x2f1;
  char field379_0x2f2;
  char field380_0x2f3;
  char field381_0x2f4;
  char field382_0x2f5;
  char field383_0x2f6;
  char field384_0x2f7;
  char field385_0x2f8;
  char field386_0x2f9;
  char field387_0x2fa;
  char field388_0x2fb;
  char field389_0x2fc;
  char field390_0x2fd;
  char field391_0x2fe;
  char field392_0x2ff;
  char field393_0x300;
  char field394_0x301;
  char field395_0x302;
  char field396_0x303;
  char field397_0x304;
  char field398_0x305;
  char field399_0x306;
  char field400_0x307;
  char field401_0x308;
  char field402_0x309;
  char field403_0x30a;
  char field404_0x30b;
  char field405_0x30c;
  char field406_0x30d;
  char field407_0x30e;
  char field408_0x30f;
  char field409_0x310;
  char field410_0x311;
  char field411_0x312;
  char field412_0x313;
  char field413_0x314;
  char field414_0x315;
  char field415_0x316;
  char field416_0x317;
  char field417_0x318;
  char field418_0x319;
  char field419_0x31a;
  char field420_0x31b;
  char field421_0x31c;
  char field422_0x31d;
  char field423_0x31e;
  char field424_0x31f;
  char field425_0x320;
  char field426_0x321;
  char field427_0x322;
  char field428_0x323;
  int field429_0x324;
  short field430_0x328;
  char field431_0x32a;
  char field432_0x32b;
};
class EntityItem : public EntityActive {
public:
  int field2_0x26c;
  int field3_0x270;
  int field4_0x274;
  int field5_0x278;
  int field6_0x27c;
  int field7_0x280;
  int field8_0x284;
  int field9_0x288;
  int field10_0x28c;
  char field11_0x290;
  char field12_0x291;
  char field13_0x292;
  char field14_0x293;
  char field15_0x294;
  char field16_0x295;
  char field17_0x296;
  char field18_0x297;
  char field19_0x298;
  char field20_0x299;
  char field21_0x29a;
  char field22_0x29b;
  int field23_0x29c;
  int field24_0x2a0;
  int field25_0x2a4;
  int field26_0x2a8;
  int field27_0x2ac;
  int field28_0x2b0;
  int field29_0x2b4;
  int field30_0x2b8;
  int field31_0x2bc;
  int field32_0x2c0;
  int field33_0x2c4;
  int field34_0x2c8;
  int field35_0x2cc;
  int field36_0x2d0;
  int field37_0x2d4;
  int field38_0x2d8;
  int field39_0x2dc;
  int field40_0x2e0;
  int field41_0x2e4;
  int field42_0x2e8;
};
class EntityMonster : public EntityActive {
public:
  int field2_0x26c;
  int field3_0x270;
  char field4_0x274;
  char field5_0x275;
  char field6_0x276;
  char field7_0x277;
  int field8_0x278;
  int field9_0x27c;
};
class EntityPlayer : public EntityActive {
public:
  char field2_0x26c;
  char field3_0x26d;
  char field4_0x26e;
  char field5_0x26f;
  char field6_0x270;
  char field7_0x271;
  char field8_0x272;
  char field9_0x273;
  int field10_0x274;
  int field11_0x278;
  int field12_0x27c;
  class PlayerData *player_data;
  EntityPlayer *follower;
  EntityPlayer *following;
  // Non-null for hired hands / CPU players. See hd_aibot.h.
  AIBot *ai_bot;
  int field17_0x290;
  int field18_0x294;
  int field19_0x298;
  int field20_0x29c;
  int field21_0x2a0;
  int field22_0x2a4;
  int field23_0x2a8;
  int field24_0x2ac;
  int field25_0x2b0;
};

} // namespace hddll
