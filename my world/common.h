#pragma once
// ↓ 必须放在 windows.h 之前
#include <winsock2.h>
#include <ws2tcpip.h>

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <unordered_map>
#include <conio.h>
#include <windows.h>
using namespace std;

const int CONSOLE_COLS = 160;
const int CONSOLE_ROWS = 54;
inline int SCR_W = 158;
inline int SCR_H = 48;

#define FOOD_DRAIN_INTERVAL 40.0f
#define FOOD_HEAL_INTERVAL  2.0f
#define FOOD_HEAL_THRESHOLD 18
#define TOOL_DUR_WOOD       60
#define TOOL_DUR_STONE      130
#define TOOL_DUR_IRON       250
#define DAY_LENGTH          600.0f

struct Vec3 {
	float x, y, z;
	Vec3 operator+(const Vec3& o) const { return {x+o.x, y+o.y, z+o.z}; }
	Vec3 operator-(const Vec3& o) const { return {x-o.x, y-o.y, z-o.z}; }
	Vec3 operator*(float s)      const { return {x*s, y*s, z*s}; }
	float dot(const Vec3& o)     const { return x*o.x + y*o.y + z*o.z; }
	Vec3 cross(const Vec3& o)    const { return {y*o.z-z*o.y, z*o.x-x*o.z, x*o.y-y*o.x}; }
	Vec3 normalize() const {
		float l = sqrtf(x*x + y*y + z*z);
		if (l < 1e-6f) return {0,0,0};
		return {x/l, y/l, z/l};
	}
};

const int NUM_TYPES = 38;
inline const char* blockNamesCN[NUM_TYPES] = {
	"", "草方块", "泥土", "石头", "原木", "树叶", "木板", "木棍", "工作台",
	"木斧", "木镐", "木铲", "石斧", "石镐", "石铲", "苹果", "箱子",
	"熔炉", "铁矿石", "煤", "铁锭", "铁斧", "铁镐", "铁铲",
	"木剑", "石剑", "铁剑",
	"生牛肉", "熟牛肉", "生羊肉", "熟羊肉",
	"生鸡肉", "熟鸡肉", "生猪肉", "熟猪肉",
	"羊毛", "床", "基岩"
};

struct RGB { uint8_t r, g, b; };

struct ItemStack {
	int type = 0;
	int count = 0;
	int durability = 0;
	bool empty() const { return type == 0 || count <= 0; }
	void clear() { type = 0; count = 0; durability = 0; }
};

struct ItemEntity {
	Vec3 pos, vel;
	int  type, count;
	float spin, life;
	bool onGround, picked;
};

struct Mob {
	Vec3 pos;
	float velY;
	int   hp;
	float hurtTimer;
	float attackCd;
	float life;
	bool  alive;
};
inline vector<Mob> g_zombies;

struct Animal {
	Vec3 pos;
	float velY;
	int   type;
	int   hp;
	float hurtTimer;
	float wanderTimer;
	float dirX, dirZ;
	bool  alive;
};
inline vector<Animal> g_animals;

struct BlockChange { int x, y, z; uint8_t type; };
struct RayHit { bool hit; float t; int bx, by, bz; int face; uint8_t blockType; };

const int CHUNK_SX = 16, CHUNK_SY = 64, CHUNK_SZ = 16;
struct Chunk {
	uint8_t blocks[CHUNK_SX][CHUNK_SY][CHUNK_SZ];
	bool generated = false;
};
struct ChestData {
	vector<ItemStack> slots;
	ChestData() : slots(27) {}
};
struct FurnaceData {
	ItemStack input;
	ItemStack fuel;
	ItemStack output;
	float cookProgress = 0;
	float fuelRemain   = 0;
};

inline string g_currentUser = "Guest";
inline uint32_t g_seed = 0;
inline unordered_map<int64_t, Chunk*> g_chunks;
inline vector<BlockChange> g_changes;
inline vector<ItemEntity> g_items;

const int INV_SIZE = 51;
inline ItemStack inventory[INV_SIZE];
inline ItemStack cursorItem;
inline int  hotbarSelected = 0;
inline bool bagOpen = false;
inline bool tableOpen = false;
inline bool chestOpen = false;
inline bool furnaceOpen = false;

inline int   g_health = 20;
const  int   MAX_HEALTH = 20;
inline int   g_food = 20;
const  int   MAX_FOOD = 20;
inline float g_foodTimer = 0;
inline float g_healTimer = 0;
inline float g_healthAnimTimer = 0;
inline float g_peakY = 0;
inline bool  g_dead = false;
inline float g_handSwing = 0;

inline float g_timeOfDay = 0.25f;
inline float g_globalLight = 1.0f;

inline map<tuple<int,int,int>, ChestData> g_chests;
inline tuple<int,int,int> g_openChest = {0,0,0};
inline ChestData g_openChestData;

inline map<tuple<int,int,int>, FurnaceData> g_furnaces;
inline tuple<int,int,int> g_openFurnace = {0,0,0};
inline FurnaceData g_openFurnaceData;

inline int   g_miningX = -1, g_miningY = -1, g_miningZ = -1;
inline float g_miningProgress = 0.0f;

// ---- 联机 ----
struct RemotePlayer {
	int  id;
	Vec3 pos;
	float yaw, pitch;
	bool active;
};
inline vector<RemotePlayer> g_remotePlayers;
inline int  g_myPlayerId    = 0;
inline bool g_isMultiplayer = false;

// ============================================================
//  函数声明
// ============================================================
void enableAnsi();
void hideCursor(bool hide);
void clearScreen();
void syncSystemCursor(bool wantShow);
void setupFixedConsole();
void getMouseChar(int& mx, int& my);
int  getConsoleVisibleWidth();
int  getConsoleVisibleHeight();
void gotoScreenTopLeft();
void enableConsoleInput(bool enable);

RGB  getItemColor(int type);
bool canPlace(int t);
bool isTool(int t);
bool isFood(int t);
bool isFuel(int t);
bool isSword(int t);
int  toolMaxDurability(int t);
int  getFoodValue(int t);
int  getSmeltResult(int inputType);
void addItem(int type, int count);
float getBlockHardness(int t);
float getToolMultiplier(int blockType, int toolType);
float getDigTime(int blockType, int toolType);
bool matchGrid(int* grid, const int* pattern);
bool matchRecipeGrid(int* grid, int& outType, int& outCount);
void updateCraft2x2();
void updateCraft3x3();
void outputToCursor(int ot, int oc);
void doCraftFrom(int* slots, int n);
RGB  getVoxelColor(int block, int face);
float faceLight(int face);

Chunk* getChunk(int cx, int cz);
uint8_t getBlock(int x, int y, int z);
void setBlock(int x, int y, int z, uint8_t v);
void modifyBlock(int x, int y, int z, uint8_t v);
void generateChunk(Chunk* c, int cx, int cz);
void clearChunks();
RayHit raycastDDA(const Vec3& origin, const Vec3& dir, float maxDist);
void spawnItemDrop(int x, int y, int z, int type, int count);
void updateItems(const Vec3& playerEye, float dt);
void playerBox(const Vec3& eye, float& x0, float& y0, float& z0,
			   float& x1, float& y1, float& z1);
bool  playerCollides(const Vec3& eye);
float sweepAxis(Vec3& eye, int axis, float delta);
bool  tryStepUp(Vec3& eye, float dx, float dz);

string slotTextFrom(const ItemStack& s, bool highlight);
string itemSlotText(int idx, bool highlight);
void renderFrame(const Vec3& camPos, const Vec3& camTarget);
void renderUI();
void renderHand(int type);
int  hitBagSlot(int mx, int my);
int  hitTableSlot(int mx, int my);
int  hitChestSlot(int mx, int my);
int  hitFurnaceSlot(int mx, int my);
void clickSlot(int idx);
void clickSlotRight(int idx);
void clickChestSlot(int idx);
void clickChestSlotRight(int idx);
void clickFurnaceSlot(int idx);
void clickFurnaceSlotRight(int idx);
void renderBagScreen();
void renderTableScreen();
void renderChestScreen();
void renderFurnaceScreen();
void updateFurnace(FurnaceData& fd, float dt);

string savePath();
void saveGame(const Vec3& pos, float yaw, float pitch);
bool loadGame(Vec3& pos, float& yaw, float& pitch);
void resetAll();

void runGame();

int loginSystem(string& outUser);
int showMenu(const string& user);
