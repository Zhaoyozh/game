#include "common.h"

RGB getItemColor(int type) {
	switch (type) {
		case 1:  return { 90, 190,  80};
		case 2:  return {115,  80,  55};
		case 3:  return {135, 135, 135};
		case 4:  return {125,  85,  50};
		case 5:  return { 55, 150,  55};
		case 6:  return {170, 130,  85};
		case 7:  return {150, 110,  60};
		case 8:  return {140, 100,  60};
		case 9: case 10: case 11: return {160, 120, 60};
		case 12: case 13: case 14: return {170, 170, 170};
		case 15: return {220,  40,  40};
		case 16: return {150, 110,  60};
		case 17: return { 90,  90,  90};
		case 18: return {170, 130, 120};
		case 19: return { 40,  40,  40};
		case 20: return {220, 220, 220};
		case 21: case 22: case 23: return {200, 200, 200};
		case 24: return {180, 140,  70};
		case 25: return {190, 190, 190};
		case 26: return {230, 230, 230};
		case 27: return {190,  50,  50};
		case 28: return {120,  60,  30};
		case 29: return {200,  90,  90};
		case 30: return {140,  70,  40};
		case 31: return {235, 190, 170};
		case 32: return {200, 140,  80};
		case 33: return {230, 130, 130};
		case 34: return {160,  90,  50};
		case 35: return {245, 245, 245};
		case 36: return {180,  40,  40};
		case 37: return { 50,  50,  55};
	}
	return {0,0,0};
}

RGB getVoxelColor(int block, int face) {
	switch (block) {
	case 1:
		if      (face == 2) return { 90, 190,  80};
		else if (face == 3) return {100,  60,  40};
		else                return {115,  80,  55};
		case 2: return {115,  80,  55};
		case 3: return {135, 135, 135};
		case 4: return {125,  85,  50};
		case 5: return { 55, 150,  55};
		case 6: return {170, 130,  85};
		case 8: return {140, 100,  60};
		case 16:return {150, 110,  60};
		case 17:return { 90,  90,  90};
		case 18:return {170, 130, 120};
		case 19:return { 40,  40,  40};
		case 35:return {235, 235, 235};
		case 36:return {180,  40,  40};
		case 37:return { 50,  50,  55};
	}
	return {0,0,0};
}

float faceLight(int face) {
	switch (face) {
		case 2: return 1.00f;
		case 0: case 4: return 0.82f;
		case 1: case 5: return 0.70f;
		case 3: return 0.50f;
	}
	return 1.0f;
}

bool canPlace(int t) { return (t >= 1 && t <= 6) || t == 8 || t == 16 || t == 17 || t == 35 || t == 36; }
bool isTool(int t)   { return (t >= 9 && t <= 14) || (t >= 21 && t <= 23); }
bool isFood(int t)   { return t == 15 || (t >= 27 && t <= 34); }
bool isFuel(int t)   { return t == 19 || t == 4 || t == 6; }
bool isSword(int t)  { return t == 24 || t == 25 || t == 26; }

int toolMaxDurability(int t) {
	if (t >= 9 && t <= 11)   return TOOL_DUR_WOOD;
	if (t >= 12 && t <= 14)  return TOOL_DUR_STONE;
	if (t >= 21 && t <= 23)  return TOOL_DUR_IRON;
	if (t == 24) return 80;
	if (t == 25) return 160;
	if (t == 26) return 300;
	return 0;
}

int getFoodValue(int t) {
	if (t == 15) return 6;
	if (t >= 27 && t <= 30) return 3;
	if (t >= 31 && t <= 34) return 8;
	return 0;
}

int getSmeltResult(int inputType) {
	if (inputType == 18) return 20;
	if (inputType == 27) return 28;
	if (inputType == 29) return 30;
	if (inputType == 31) return 32;
	if (inputType == 33) return 34;
	return 0;
}

void addItem(int type, int count) {
	if (type <= 0 || count <= 0) return;
	if (isTool(type) || isSword(type)) {
		for (int k = 0; k < count; ++k) {
			for (int i = 0; i < 36; ++i) {
				if (inventory[i].empty()) {
					inventory[i].type = type;
					inventory[i].count = 1;
					inventory[i].durability = toolMaxDurability(type);
					break;
				}
			}
		}
		return;
	}
	for (int i = 0; i < 36; ++i) {
		if (inventory[i].type == type && inventory[i].count < 64) {
			int space = 64 - inventory[i].count;
			int mv = space < count ? space : count;
			inventory[i].count += mv;
			count -= mv;
			if (count <= 0) return;
		}
	}
	for (int i = 0; i < 36; ++i) {
		if (inventory[i].empty()) {
			inventory[i].type = type;
			inventory[i].count = count;
			return;
		}
	}
}

float getBlockHardness(int t) {
	switch (t) {
		case 1: case 2: case 5: return 0.6f;
		case 3:                 return 2.0f;
		case 4:                 return 1.5f;
		case 6: case 16:        return 1.0f;
		case 8:                 return 1.2f;
		case 17:                return 2.2f;
		case 18:                return 3.0f;
		case 19:                return 2.5f;
		case 35:                return 0.8f;
		case 36:                return 0.8f;
		case 37:                return 9999.0f;
	}
	return 1.0f;
}
float getToolMultiplier(int blockType, int toolType) {
	switch (toolType) {
		case 9:  if (blockType==4||blockType==6||blockType==8||blockType==16||blockType==17) return 4.0f; break;
		case 10: if (blockType==3||blockType==17||blockType==18||blockType==19) return 4.0f; break;
		case 11: if (blockType==1||blockType==2) return 4.0f; break;
		case 12: if (blockType==4||blockType==6||blockType==8||blockType==16||blockType==17) return 6.0f; break;
		case 13: if (blockType==3||blockType==17||blockType==18||blockType==19) return 6.0f; break;
		case 14: if (blockType==1||blockType==2) return 6.0f; break;
		case 21: if (blockType==4||blockType==6||blockType==8||blockType==16||blockType==17) return 8.0f; break;
		case 22: if (blockType==3||blockType==17||blockType==18||blockType==19) return 8.0f; break;
		case 23: if (blockType==1||blockType==2) return 8.0f; break;
	}
	return 1.0f;
}
float getDigTime(int blockType, int toolType) {
	return getBlockHardness(blockType) / getToolMultiplier(blockType, toolType);
}

struct Recipe { int in[9]; int outType; int outCount; };
static Recipe recipes[] = {
	{{4,-1,-1, -1,-1,-1, -1,-1,-1}, 6, 4},
	{{6,-1,-1, 6,-1,-1, -1,-1,-1}, 7, 4},
	{{-1,6,-1, -1,6,-1, -1,-1,-1}, 7, 4},
	{{-1,-1,6, -1,-1,6, -1,-1,-1}, 7, 4},
	{{6,6,-1, 6,6,-1, -1,-1,-1}, 8, 1},
	{{3,3,3, 3,-1,3, 3,3,3}, 17, 1},
	{{6,6,6, -1,7,-1, -1,7,-1}, 10, 1},
	{{6,6,-1, 6,7,-1, -1,7,-1}, 9, 1},
	{{-1,6,-1, -1,7,-1, -1,7,-1}, 11, 1},
	{{3,3,3, -1,7,-1, -1,7,-1}, 13, 1},
	{{3,3,-1, 3,7,-1, -1,7,-1}, 12, 1},
	{{-1,3,-1, -1,7,-1, -1,7,-1}, 14, 1},
	{{6,6,6, 6,-1,6, 6,6,6}, 16, 1},
	{{20,20,20, -1,7,-1, -1,7,-1}, 22, 1},
	{{20,20,-1, 20,7,-1, -1,7,-1}, 21, 1},
	{{-1,20,-1, -1,7,-1, -1,7,-1}, 23, 1},
	{{6,-1,-1, 6,-1,-1, 7,-1,-1}, 24, 1},
	{{3,-1,-1, 3,-1,-1, 7,-1,-1}, 25, 1},
	{{20,-1,-1, 20,-1,-1, 7,-1,-1}, 26, 1},
	{{35,35,35, 6,6,6, -1,-1,-1}, 36, 1},
};
const int NUM_RECIPES = sizeof(recipes)/sizeof(recipes[0]);

bool matchGrid(int* grid, const int* pattern) {
	for (int oy = -2; oy <= 2; ++oy)
		for (int ox = -2; ox <= 2; ++ox) {
			bool ok = true;
			int gHits = 0, pHits = 0;
			for (int gy = 0; gy < 3 && ok; ++gy)
				for (int gx = 0; gx < 3 && ok; ++gx) {
					int gv = grid[gy*3+gx];
					int px = gx-ox, py = gy-oy;
					int pv = -1;
					if (px>=0&&px<3&&py>=0&&py<3) pv = pattern[py*3+px];
					if (gv == -1) { if (pv != -1) ok = false; }
					else { gHits++; if (pv != gv) ok = false; else pHits++; }
				}
			if (ok && gHits == pHits && gHits > 0) return true;
		}
	return false;
}
bool matchRecipeGrid(int* grid, int& outType, int& outCount) {
	for (int k = 0; k < NUM_RECIPES; ++k)
		if (matchGrid(grid, recipes[k].in)) {
			outType = recipes[k].outType;
			outCount = recipes[k].outCount;
			return true;
		}
	return false;
}
void updateCraft2x2() {
	int grid[9] = {-1,-1,-1,-1,-1,-1,-1,-1,-1};
	grid[0] = inventory[36].empty() ? -1 : inventory[36].type;
	grid[1] = inventory[37].empty() ? -1 : inventory[37].type;
	grid[3] = inventory[38].empty() ? -1 : inventory[38].type;
	grid[4] = inventory[39].empty() ? -1 : inventory[39].type;
	int ot, oc;
	if (matchRecipeGrid(grid, ot, oc)) { inventory[40].type = ot; inventory[40].count = oc; }
	else inventory[40].clear();
}
void updateCraft3x3() {
	int grid[9];
	for (int i = 0; i < 9; ++i) grid[i] = inventory[41+i].empty() ? -1 : inventory[41+i].type;
	int ot, oc;
	if (matchRecipeGrid(grid, ot, oc)) { inventory[50].type = ot; inventory[50].count = oc; }
	else inventory[50].clear();
}
void outputToCursor(int ot, int oc) {
	if (cursorItem.empty()) {
		cursorItem.type = ot;
		cursorItem.count = oc;
		if (isTool(ot) || isSword(ot)) cursorItem.durability = toolMaxDurability(ot);
	} else cursorItem.count += oc;
}
void doCraftFrom(int* slots, int n) {
	int grid[9];
	for (int i = 0; i < 9; ++i) grid[i] = -1;
	if (n == 4) {
		grid[0] = inventory[slots[0]].empty() ? -1 : inventory[slots[0]].type;
		grid[1] = inventory[slots[1]].empty() ? -1 : inventory[slots[1]].type;
		grid[3] = inventory[slots[2]].empty() ? -1 : inventory[slots[2]].type;
		grid[4] = inventory[slots[3]].empty() ? -1 : inventory[slots[3]].type;
	} else {
		for (int i = 0; i < 9; ++i)
			grid[i] = inventory[slots[i]].empty() ? -1 : inventory[slots[i]].type;
	}
	int ot, oc;
	if (!matchRecipeGrid(grid, ot, oc)) return;
	if (!cursorItem.empty()) {
		if (cursorItem.type != ot) return;
		if (cursorItem.count + oc > 64) return;
	}
	for (int i = 0; i < n; ++i)
		if (!inventory[slots[i]].empty()) {
			inventory[slots[i]].count--;
			if (inventory[slots[i]].count <= 0) inventory[slots[i]].clear();
		}
	outputToCursor(ot, oc);
}
