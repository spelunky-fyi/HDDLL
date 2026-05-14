#pragma once

#include <stddef.h>
#include <stdint.h>

// This header is included from hd.h *after* hd_entity.h, so the entity classes
// are fully defined by the time anything uses AIBot. We only forward declare
// here (AIBot just stores pointers to these) to avoid an include cycle:
// hd_entity.h includes hd.h, and hd.h includes this file.
namespace hddll {

class Entity;
class EntityActive;
class EntityPlayer;
class EntityBackground;

// ===========================================================================
// AIBot
// ===========================================================================
//
// Every "hired hand" / CPU controlled spelunker (co-op AI followers and the
// deathmatch bots) owns an AIBot. It hangs off EntityPlayer::ai_bot.
//
// The AIBot is what feeds the player's PlayerInput struct every frame so the
// engine drives the spelunker as if a human were holding the controller. Its
// per-frame entry point is `ai_bot_update`, which roughly does:
//
//   1. ai_bot_scan_world       - rebuild pPerception_list + danger flags
//   2. ai_bot_shift_input_state - copy this frame's PlayerInput action bits
//                                into prev_* and clear them, ready to be set
//   3. ai_bot_pick_combat_action - decide whether to bomb/throw/etc.
//   4. ai_bot_pick_coop_target / ai_bot_pick_deathmatch_action
//                              - decide the current AiState + target
//   5. ai_bot_walk_path        - convert the current path node into stick/
//                                button inputs (writes EntityActive::pPlayer_input)
//   6. when repath_cooldown expires (or bPath_dirty is set) -> recompute the
//      A* path with ai_bot_compute_path
//
// ai_bot_init seeds a fresh bot: bPath_dirty = 1, all timers/cooldowns 0,
// combat_action = NONE, flTarget = the spelunker's own position, and
// dwPersonality_id = random 0..2 (re-rolled so co-op hands don't share one).
//
// All addresses/offsets below were reversed from Spelunky.exe 1.47.
// ===========================================================================

// High level "what is the bot trying to do right now" state. Lives in
// AIBot::ai_state and is chosen by ai_bot_pick_coop_target /
// ai_bot_pick_deathmatch_action / ai_bot_pick_combat_action.
enum class AiState : int32_t {
  // Chase down a monster/player target (co-op: kill threats near the leader).
  AI_STATE_HUNT = 0,
  // Move toward and pick up nCurrent_item_target (a weapon/pickup).
  AI_STATE_ITEM_PURSUIT = 1,
  // Path to flAnchor_x/flAnchor_y and sit there. Used to back away from
  // danger (live bombs, blocked nodes) and hold until state_timer expires.
  AI_STATE_MOVE_TO_ANCHOR = 2,
  // No target - wander around near the leader. Picks nWander_target_x/y.
  AI_STATE_WANDER = 3,
  // Bot is off-screen / separated and is trying to get back on screen.
  AI_STATE_ESCAPE_OFFSCREEN = 4,
  // Stand still (e.g. last player alive, riding something) - see
  // ai_bot_walk_path's HOLD_POSITION branch / ledge-walk logic.
  AI_STATE_HOLD_POSITION = 5,
};

// Discrete combat "verb" chosen by ai_bot_pick_combat_action and carried out
// by ai_bot_execute_combat_action. COMBAT_ACTION_NONE means "no combat this
// frame, just keep walking the path".
enum class CombatAction : int32_t {
  COMBAT_ACTION_NONE = -1,
  COMBAT_ACTION_BOMB_PRESS = 0,       // place / press-throw a bomb
  COMBAT_ACTION_BOMB_HOLD = 1,        // hold a bomb (cook it) before throwing
  COMBAT_ACTION_THROW_HELD_UP = 2,    // throw the currently held item upward
  COMBAT_ACTION_LIVE_BOMB_DISPOSE = 3,// get rid of a live bomb that's nearby
  COMBAT_ACTION_JUMP_DIRECTIONAL = 4, // jump in a direction (dodge / gap)
  COMBAT_ACTION_THROW_ROPE = 5,       // throw a rope to climb
  COMBAT_ACTION_DROP_AND_PURSUE = 6,  // drop held item to chase a better one
};

#pragma pack(push, 1)

// ---------------------------------------------------------------------------
// PathNode
// ---------------------------------------------------------------------------
// One cell of the bot's local A* search grid (see AIBot::pPathfind_grid).
// The grid is rebuilt every repath by `pathfind_build_grid`.
class PathNode {
public:
  // Position of this node *within the local grid* (nCell_x: 0..20,
  // nCell_y: 0..16). The bot itself always sits at cell (10, 8).
  int32_t nCell_x;        // 0x00
  int32_t nCell_y;        // 0x04
  // Position of this node in *world tile* coordinates (truncated). Use these
  // when drawing the node in the world. (Ghidra: nUnknown_08 / nUnknown_0c.)
  int32_t nWorld_x;       // 0x08
  int32_t nWorld_y;       // 0x0C
  // A* costs. nG_cost = cost from the start node so far (seeded to 99999999
  // by pathfind_build_grid, set to 0 for the bot's own node). nH_cost =
  // heuristic to the goal.
  int32_t nG_cost;        // 0x10
  int32_t nH_cost;        // 0x14
  // Classification of the tile from pathfind_build_grid. Observed values:
  //   0  = open / walkable air
  //   1  = solid floor (or a pushable/ladder-ish blocker)
  //   5  = liquid (water/lava) - also sets bPit_flag
  //   9  = spikes (entity_type 1 sitting on the tile)
  //   11 = (level dependent)
  //   12 = tree trunk
  //   13 = bounce trap tile
  //   14 = edge / ledge tile
  //   15/16 = door-ish / level-transition tile
  // Not every value is fully understood - treat as a hint.
  int32_t nTile_type;     // 0x18
  // True once this node has been moved onto the closed list by the A* search.
  uint8_t bIn_closed;     // 0x1C
  uint8_t bFlag_1d;       // 0x1D
  // Tile sits on a ledge/edge the bot may need to climb or drop from.
  uint8_t bEdge_flag;     // 0x1E
  // Tile is blocked (cannot be entered) - e.g. solid floor.
  uint8_t bBlocked_flag;  // 0x1F
  // Tile is a pit / liquid the bot should avoid falling into.
  uint8_t bPit_flag;      // 0x20
  // Tile is "locked" (a hard no-go for this search).
  uint8_t bLocked_flag;   // 0x21
  // Something dangerous (live bomb, boulder, spike ball, ...) overlaps this
  // tile - set by ai_bot_scan_world / ai_bot_mark_danger_rect. The bot will
  // refuse to path through danger cells and may bail to MOVE_TO_ANCHOR.
  uint8_t bDanger_flag;   // 0x22
  uint8_t bFlag_23;       // 0x23
  // A* linkage. pParent points back toward the bot's node during the search;
  // after a path is found, ai_bot_compute_path re-links pParent->pNext so the
  // path can be walked *forward* from pPath_current_node via ->pNext until
  // pPath_start_node (the goal) is reached.
  PathNode *pParent;      // 0x24
  PathNode *pNext;        // 0x28
  // The open/closed list this node currently belongs to (if any).
  void *pOwning_list;     // 0x2C
};
static_assert(sizeof(PathNode) == 0x30);

// ---------------------------------------------------------------------------
// AIBot
// ---------------------------------------------------------------------------
class AIBot {
public:
  // Engine vtable pointer for the bot object.
  void **vftable;                       // 0x0000
  // The spelunker this bot drives.
  EntityPlayer *entity;                 // 0x0004
  // Background entity associated with the bot (used by off-screen logic).
  EntityBackground *pEntity_background; // 0x0008

  // "Perception list": every interesting entity the bot can currently see -
  // rebuilt each frame by ai_bot_scan_world. Holds other players, monsters
  // and notable items (crates, bombs, weapons, ...). NULL terminated, max
  // 128 entries. Combat/target picking iterates this list.
  Entity *pPerception_list[128];        // 0x000C

  // Local A* search grid, 21 wide (x) by 17 tall (y), row-minor by X:
  //   index = nCell_x * 17 + nCell_y      (0x11 == 17)
  // The grid is centered on the bot, which always occupies cell (10, 8).
  // Rebuilt every repath by pathfind_build_grid.
  PathNode pPathfind_grid[357];          // 0x020C

  // A* working sets. pPathfind_open_count / pPathfind_closed_count below say
  // how many slots are live.
  PathNode *pPathfind_open_list[357];    // 0x44FC
  PathNode *pPathfind_closed_list[357];  // 0x4A90

  // Current high-level behaviour. See AiState.
  AiState ai_state;                      // 0x5024
  uint8_t pad_5028[4];                   // 0x5028 (unused gap)

  // The entity the bot is currently pursuing as an item/target (AI_STATE_
  // ITEM_PURSUIT / HUNT). null when none.
  Entity *pCurrent_item_target;          // 0x502C
  // 0 = melee/normal stance, 2 = prefer keeping ranged distance. Set by the
  // target-picking code depending on what the bot is doing.
  int32_t nPrefer_ranged_stance;         // 0x5030
  int32_t nUnknown_5034;                 // 0x5034
  // Frames to wait before the bot may pick another combat action.
  int32_t nCombat_cooldown;              // 0x5038
  // Combat verb chosen this frame. See CombatAction.
  CombatAction combat_action;            // 0x503C

  // Number of live entries in pPathfind_open_list / pPathfind_closed_list.
  int32_t nPathfind_open_count;          // 0x5040
  int32_t nPathfind_closed_count;        // 0x5044
  // Counts down each frame in ai_bot_update; when it hits <1 the bot picks a
  // fresh flTarget and recomputes its path (then it is reset to 5).
  int32_t repath_cooldown;               // 0x5048
  // Set when ai_bot_walk_path advanced pPath_active_node to the next node.
  uint8_t bPath_step_advanced;           // 0x504C
  uint8_t pad_504D[3];                   // 0x504D

  // This bot's "personality" (0..2), randomly assigned in ai_bot_init and
  // re-rolled so co-op hired hands in the same game don't share one. Biases
  // the pick/combat behaviour.
  uint32_t dwPersonality_id;             // 0x5050
  // Cached deathmatch win counts for players 0..3, refreshed every frame in
  // ai_bot_update from GlobalState player data; used to gang up on whoever
  // is winning.
  int32_t nTracked_wins_p0;              // 0x5054
  int32_t nTracked_wins_p1;              // 0x5058
  int32_t nTracked_wins_p2;              // 0x505C
  int32_t nTracked_wins_p3;              // 0x5060

  // Non-zero when this bot is a deathmatch arena bot (vs a co-op hired hand).
  // Selects between the co-op and deathmatch decision paths.
  uint8_t bIs_deathmatch_mode;           // 0x5064
  uint8_t bUnknown_5065;                 // 0x5065
  uint8_t bUnknown_5066;                 // 0x5066
  uint8_t bUnknown_5067;                 // 0x5067
  // Set by ai_bot_compute_path: 1 if the A* search reached the goal.
  uint8_t bPathfind_found_path;          // 0x5068
  uint8_t pad_5069[3];                   // 0x5069

  // Wander destination (world tile coords) when in AI_STATE_WANDER.
  int32_t nWander_target_x_int;          // 0x506C
  int32_t nWander_target_y_int;          // 0x5070

  // The walked path is a forward-linked PathNode chain:
  //   pPath_current_node ->pNext ->pNext ... -> pPath_start_node
  // pPath_start_node   - the GOAL node (where the path ends).
  // pPath_current_node - the node at the bot's own position (path origin).
  // pPath_active_node  - the node the bot is currently moving toward; it is
  //                      advanced along ->pNext by ai_bot_walk_path.
  PathNode *pPath_start_node;            // 0x5074
  PathNode *pPath_current_node;          // 0x5078
  PathNode *pPath_active_node;           // 0x507C

  // World-space destination the path was computed toward (in co-op this is
  // the leader's position; in deathmatch a chosen target).
  float flTarget_x;                      // 0x5080
  float flTarget_y;                      // 0x5084
  // Generic countdown used by most states - when it expires the bot
  // re-evaluates its AiState / target.
  int32_t state_timer;                   // 0x5088
  // When set, the path is stale and will be recomputed on the next update
  // (usually paired with repath_cooldown = 0).
  uint8_t bPath_dirty;                   // 0x508C
  uint8_t pad_508D[3];                   // 0x508D

  // World-space anchor the bot returns to in AI_STATE_MOVE_TO_ANCHOR.
  float flAnchor_x;                      // 0x5090
  float flAnchor_y;                      // 0x5094
  // Ledge-walking sub-state used by the HOLD_POSITION branch of
  // ai_bot_walk_path (shuffling along a ledge without falling off).
  int32_t nLedge_walk_timer;             // 0x5098
  uint8_t bLedge_walk_state;             // 0x509C
  uint8_t pad_509D[3];                   // 0x509D

  // Convenience helpers (non-virtual, do not affect layout).
  const char *StateName() const;
  const char *CombatActionName() const;
};

#pragma pack(pop)

static_assert(sizeof(AIBot) == 0x50A0);
static_assert(offsetof(AIBot, pPathfind_grid) == 0x020C);
static_assert(offsetof(AIBot, ai_state) == 0x5024);
static_assert(offsetof(AIBot, pPath_start_node) == 0x5074);
static_assert(offsetof(AIBot, bLedge_walk_state) == 0x509C);

// Dimensions of AIBot::pPathfind_grid.
constexpr int AIBOT_GRID_W = 21;
constexpr int AIBOT_GRID_H = 17;
// The bot always occupies this cell of its local grid.
constexpr int AIBOT_GRID_ORIGIN_X = 10;
constexpr int AIBOT_GRID_ORIGIN_Y = 8;

inline const char *AiStateName(AiState s) {
  switch (s) {
  case AiState::AI_STATE_HUNT:
    return "HUNT";
  case AiState::AI_STATE_ITEM_PURSUIT:
    return "ITEM_PURSUIT";
  case AiState::AI_STATE_MOVE_TO_ANCHOR:
    return "MOVE_TO_ANCHOR";
  case AiState::AI_STATE_WANDER:
    return "WANDER";
  case AiState::AI_STATE_ESCAPE_OFFSCREEN:
    return "ESCAPE_OFFSCREEN";
  case AiState::AI_STATE_HOLD_POSITION:
    return "HOLD_POSITION";
  default:
    return "UNKNOWN";
  }
}

inline const char *CombatActionName(CombatAction a) {
  switch (a) {
  case CombatAction::COMBAT_ACTION_NONE:
    return "NONE";
  case CombatAction::COMBAT_ACTION_BOMB_PRESS:
    return "BOMB_PRESS";
  case CombatAction::COMBAT_ACTION_BOMB_HOLD:
    return "BOMB_HOLD";
  case CombatAction::COMBAT_ACTION_THROW_HELD_UP:
    return "THROW_HELD_UP";
  case CombatAction::COMBAT_ACTION_LIVE_BOMB_DISPOSE:
    return "LIVE_BOMB_DISPOSE";
  case CombatAction::COMBAT_ACTION_JUMP_DIRECTIONAL:
    return "JUMP_DIRECTIONAL";
  case CombatAction::COMBAT_ACTION_THROW_ROPE:
    return "THROW_ROPE";
  case CombatAction::COMBAT_ACTION_DROP_AND_PURSUE:
    return "DROP_AND_PURSUE";
  default:
    return "UNKNOWN";
  }
}

inline const char *AIBot::StateName() const { return AiStateName(ai_state); }
inline const char *AIBot::CombatActionName() const {
  return hddll::CombatActionName(combat_action);
}

} // namespace hddll
