// ============================================================================
//  save.cpp — 存档 / 读档 / resetAll
// ============================================================================
#include "common.h"

string savePath() { return g_currentUser + ".save"; }

// ============================================================
//  存档
// ============================================================
void saveGame(const Vec3& pos, float yaw, float pitch) {
	ofstream f(savePath());
	if (!f.is_open()) return;
	
	// 版本 / 世界种子
	f << g_seed << "\n";
	
	// 玩家位置 / 朝向
	f << pos.x << " " << pos.y << " " << pos.z << "\n";
	f << yaw << " " << pitch << "\n";
	
	// 血量 / 饱食 / 时间
	f << g_health << " " << g_food << "\n";
	f << g_timeOfDay << "\n";
	
	// 41 格背包（含耐久）
	for (int i = 0; i < INV_SIZE; ++i)
		f << inventory[i].type << " " << inventory[i].count
		<< " " << inventory[i].durability << "\n";
	
	// 箱子
	f << g_chests.size() << "\n";
	for (auto& kv : g_chests) {
		int x = get<0>(kv.first), y = get<1>(kv.first), z = get<2>(kv.first);
		f << x << " " << y << " " << z << " 27";
		for (int i = 0; i < 27; ++i)
			f << " " << kv.second.slots[i].type
			<< " " << kv.second.slots[i].count
			<< " " << kv.second.slots[i].durability;
		f << "\n";
	}
	
	// 熔炉
	f << g_furnaces.size() << "\n";
	for (auto& kv : g_furnaces) {
		int x = get<0>(kv.first), y = get<1>(kv.first), z = get<2>(kv.first);
		f << x << " " << y << " " << z;
		f << " " << kv.second.input.type  << " " << kv.second.input.count  << " " << kv.second.input.durability;
		f << " " << kv.second.fuel.type   << " " << kv.second.fuel.count   << " " << kv.second.fuel.durability;
		f << " " << kv.second.output.type << " " << kv.second.output.count << " " << kv.second.output.durability;
		f << " " << kv.second.cookProgress << " " << kv.second.fuelRemain;
		f << "\n";
	}
	
	// 方块改动（挖过 / 放过）
	f << g_changes.size() << "\n";
	for (auto& c : g_changes)
		f << c.x << " " << c.y << " " << c.z << " " << (int)c.type << "\n";
	
	f.close();
}

// ============================================================
//  读档
// ============================================================
bool loadGame(Vec3& pos, float& yaw, float& pitch) {
	ifstream f(savePath());
	if (!f.is_open()) return false;
	
	// 世界种子
	if (!(f >> g_seed)) return false;
	
	// 玩家位置 / 朝向
	f >> pos.x >> pos.y >> pos.z;
	f >> yaw >> pitch;
	
	// 血量 / 饱食 / 时间
	f >> g_health >> g_food;
	if (g_health < 1) g_health = MAX_HEALTH;
	if (g_food < 0)   g_food = MAX_FOOD;
	if (!(f >> g_timeOfDay)) g_timeOfDay = 0.25f;
	if (g_timeOfDay < 0 || g_timeOfDay >= 1.0f) g_timeOfDay = 0.25f;
	
	// 背包
	for (int i = 0; i < INV_SIZE; ++i)
		f >> inventory[i].type >> inventory[i].count >> inventory[i].durability;
	
	// 箱子
	size_t nChest = 0;
	if (f >> nChest) {
		g_chests.clear();
		for (size_t i = 0; i < nChest; ++i) {
			int x, y, z, sz;
			f >> x >> y >> z >> sz;
			ChestData cd;
			cd.slots.resize(27);
			for (int k = 0; k < 27; ++k)
				f >> cd.slots[k].type >> cd.slots[k].count >> cd.slots[k].durability;
			g_chests[make_tuple(x, y, z)] = cd;
		}
	}
	
	// 熔炉
	size_t nFur = 0;
	if (f >> nFur) {
		g_furnaces.clear();
		for (size_t i = 0; i < nFur; ++i) {
			int x, y, z;
			FurnaceData fd;
			f >> x >> y >> z;
			f >> fd.input.type  >> fd.input.count  >> fd.input.durability;
			f >> fd.fuel.type   >> fd.fuel.count   >> fd.fuel.durability;
			f >> fd.output.type >> fd.output.count >> fd.output.durability;
			f >> fd.cookProgress >> fd.fuelRemain;
			g_furnaces[make_tuple(x, y, z)] = fd;
		}
	}
	
	// 方块改动
	size_t n = 0;
	if (!(f >> n)) n = 0;
	g_changes.clear();
	for (size_t i = 0; i < n; ++i) {
		BlockChange c; int t;
		f >> c.x >> c.y >> c.z >> t;
		c.type = (uint8_t)t;
		g_changes.push_back(c);
	}
	for (auto& c : g_changes) setBlock(c.x, c.y, c.z, c.type);
	
	f.close();
	return true;
}

// ============================================================
//  重置全局状态
// ============================================================
void resetAll() {
	for (int i = 0; i < INV_SIZE; ++i) inventory[i].clear();
	cursorItem.clear();
	hotbarSelected = 0;
	
	g_changes.clear();
	g_items.clear();
	g_zombies.clear();
	g_animals.clear();
	g_chests.clear();
	g_furnaces.clear();
	
	g_miningX = g_miningY = g_miningZ = -1;
	g_miningProgress = 0;
	
	g_health = MAX_HEALTH;
	g_food = MAX_FOOD;
	g_foodTimer = 0;
	g_healTimer = 0;
	g_healthAnimTimer = 0;
	g_peakY = 0;
	g_dead = false;
	g_handSwing = 0;
	
	g_timeOfDay = 0.25f;      // 正午
	g_globalLight = 1.0f;
	
	bagOpen = false;
	tableOpen = false;
	chestOpen = false;
	furnaceOpen = false;
}
