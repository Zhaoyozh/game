#include "common.h"
#include "multiplayer.h"

const float GRAVITY   = -0.020f;
const float JUMP_VEL  =  0.30f;
const float MAX_FALL  = -0.80f;
const float MOB_GRAVITY = -0.025f;

// ============================================================
//  判断位置是否"暗"
// ============================================================
static bool isDarkSpot(int x, int y, int z) {
	for (int dy = 1; dy <= 6; ++dy) {
		if (getBlock(x, y + dy, z) != 0) return true;
	}
	if (g_globalLight < 0.45f) return true;
	return false;
}

// ============================================================
//  僵尸 AI
// ============================================================
static void updateZombies(const Vec3& playerEye, float dt) {
	if (g_zombies.size() < 5) {
		if (rand() % 400 < 3) {
			float ang = (rand() % 1000) / 1000.0f * 6.283f;
			float dist = 10 + (rand() % 12);
			int mx = (int)(playerEye.x + cosf(ang) * dist);
			int mz = (int)(playerEye.z + sinf(ang) * dist);
			
			int standY = -1;
			int py = (int)playerEye.y;
			for (int y = py + 6; y >= 1; --y) {
				if (getBlock(mx, y, mz) != 0) { standY = y + 1; break; }
			}
			if (standY > 0 &&
				getBlock(mx, standY, mz) == 0 &&
				getBlock(mx, standY + 1, mz) == 0 &&
				isDarkSpot(mx, standY, mz)) {
				Mob m;
				m.pos = {(float)mx + 0.5f, (float)standY, (float)mz + 0.5f};
				m.velY = 0; m.hp = 20;
				m.hurtTimer = 0; m.attackCd = 0; m.life = 0; m.alive = true;
				g_zombies.push_back(m);
			}
		}
	}
	
	for (auto& z : g_zombies) {
		if (!z.alive) continue;
		if (z.hurtTimer > 0) z.hurtTimer -= dt;
		if (z.attackCd > 0) z.attackCd -= dt;
		
		bool covered = false;
		for (int dy = 1; dy <= 6; ++dy) {
			if (getBlock((int)z.pos.x, (int)z.pos.y + dy, (int)z.pos.z) != 0) {
				covered = true; break;
			}
		}
		if (!covered && g_globalLight > 0.7f) {
			z.life += dt * 3;
			if (z.life >= 1.0f) { z.life = 0; z.hp -= 1; z.hurtTimer = 0.3f; }
			if (z.hp <= 0) z.alive = false;
			continue;
		} else {
			z.life = 0;
		}
		
		float dx = playerEye.x - z.pos.x;
		float dz = playerEye.z - z.pos.z;
		float dd = sqrtf(dx*dx + dz*dz);
		float moveX = 0, moveZ = 0;
		if (dd > 0.8f && dd < 14.0f) {
			float spd = 0.045f;
			moveX = dx / dd * spd;
			moveZ = dz / dd * spd;
		}
		Vec3 testPos = z.pos;
		testPos.x += moveX;
		bool okX = true;
		for (int dy = 0; dy < 2; ++dy)
			if (getBlock((int)floorf(testPos.x), (int)floorf(z.pos.y + dy), (int)floorf(z.pos.z)) != 0) { okX = false; break; }
		if (okX) z.pos.x = testPos.x;
		testPos = z.pos;
		testPos.z += moveZ;
		bool okZ = true;
		for (int dy = 0; dy < 2; ++dy)
			if (getBlock((int)floorf(z.pos.x), (int)floorf(z.pos.y + dy), (int)floorf(testPos.z)) != 0) { okZ = false; break; }
		if (okZ) z.pos.z = testPos.z;
		
		z.velY += MOB_GRAVITY;
		if (z.velY < -0.6f) z.velY = -0.6f;
		float ny = z.pos.y + z.velY;
		if (getBlock((int)floorf(z.pos.x), (int)floorf(ny), (int)floorf(z.pos.z)) != 0) {
			if (z.velY < 0) { z.pos.y = floorf(z.pos.y) + 1.0f; z.velY = 0; }
			else z.velY = 0;
		} else z.pos.y = ny;
		
		float dxp = z.pos.x - playerEye.x;
		float dzp = z.pos.z - playerEye.z;
		float dyp = z.pos.y - (playerEye.y - 1.62f);
		if (z.attackCd <= 0 && fabsf(dyp) < 2.0f && sqrtf(dxp*dxp + dzp*dzp) < 1.2f) {
			g_health -= 2;
			if (g_health < 0) g_health = 0;
			g_healthAnimTimer = 1.0f;
			z.attackCd = 1.5f;
		}
	}
	
	vector<Mob> keep;
	for (auto& z : g_zombies) if (z.alive) keep.push_back(z);
	g_zombies.swap(keep);
}

// ============================================================
//  动物 AI
// ============================================================
static void updateAnimals(const Vec3& playerEye, float dt) {
	if (g_globalLight > 0.5f && g_animals.size() < 8) {
		if (rand() % 1500 < 1) {
			float ang = (rand() % 1000) / 1000.0f * 6.283f;
			float dist = 10 + (rand() % 12);
			int mx = (int)(playerEye.x + cosf(ang) * dist);
			int mz = (int)(playerEye.z + sinf(ang) * dist);
			int my = -1;
			for (int y = CHUNK_SY - 2; y >= 0; --y)
				if (getBlock(mx, y, mz) != 0) { my = y + 1; break; }
			if (my > 0 && getBlock(mx, my - 1, mz) == 1) {
				Animal a;
				a.pos = {(float)mx + 0.5f, (float)my, (float)mz + 0.5f};
				a.velY = 0;
				a.type = rand() % 4;
				a.hp = (a.type == 2) ? 4 : (a.type == 1 ? 8 : 10);
				a.hurtTimer = 0;
				a.wanderTimer = 0;
				a.dirX = 0; a.dirZ = 0;
				a.alive = true;
				g_animals.push_back(a);
			}
		}
	}
	
	for (auto& a : g_animals) {
		if (!a.alive) continue;
		if (a.hurtTimer > 0) a.hurtTimer -= dt;
		
		a.wanderTimer -= dt;
		if (a.wanderTimer <= 0) {
			a.wanderTimer = 2.0f + (rand() % 300) / 100.0f;
			if (rand() % 3 == 0) {
				a.dirX = 0; a.dirZ = 0;
			} else {
				float ang = (rand() % 1000) / 1000.0f * 6.283f;
				a.dirX = cosf(ang);
				a.dirZ = sinf(ang);
			}
		}
		
		float spd = (a.type == 2) ? 0.03f : 0.025f;
		float moveX = a.dirX * spd;
		float moveZ = a.dirZ * spd;
		
		Vec3 testPos = a.pos;
		testPos.x += moveX;
		bool okX = true;
		for (int dy = 0; dy < 2; ++dy)
			if (getBlock((int)floorf(testPos.x), (int)floorf(a.pos.y + dy), (int)floorf(a.pos.z)) != 0) { okX = false; break; }
		if (okX) a.pos.x = testPos.x;
		else { a.dirX = -a.dirX; a.wanderTimer = 0; }
		
		testPos = a.pos;
		testPos.z += moveZ;
		bool okZ = true;
		for (int dy = 0; dy < 2; ++dy)
			if (getBlock((int)floorf(a.pos.x), (int)floorf(a.pos.y + dy), (int)floorf(testPos.z)) != 0) { okZ = false; break; }
		if (okZ) a.pos.z = testPos.z;
		else { a.dirZ = -a.dirZ; a.wanderTimer = 0; }
		
		a.velY += MOB_GRAVITY;
		if (a.velY < -0.6f) a.velY = -0.6f;
		float ny = a.pos.y + a.velY;
		if (getBlock((int)floorf(a.pos.x), (int)floorf(ny), (int)floorf(a.pos.z)) != 0) {
			if (a.velY < 0) { a.pos.y = floorf(a.pos.y) + 1.0f; a.velY = 0; }
			else a.velY = 0;
		} else a.pos.y = ny;
	}
	
	vector<Animal> keep;
	for (auto& a : g_animals) if (a.alive) keep.push_back(a);
	g_animals.swap(keep);
}

// ============================================================
//  主循环
// ============================================================
void runGame() {
	clearScreen();
	hideCursor(true);
	syncSystemCursor(false);
	enableConsoleInput(false);
	
	resetAll();
	clearChunks();
	
	Vec3 camPos;
	float yaw = 0, pitch = -0.10f;
	bool loaded = loadGame(camPos, yaw, pitch);
	
	g_timeOfDay = 0.25f;
	
	if (!loaded) {
		g_seed = (uint32_t)(time(nullptr) ^ GetTickCount() ^ GetCurrentProcessId()
							^ (uint32_t)(uintptr_t)&camPos);
		int sx = 0, sz = 0;
		int feetY = CHUNK_SY - 1;
		for (int y = CHUNK_SY-1; y >= 0; --y)
			if (getBlock(sx, y, sz) != 0) { feetY = y + 1; break; }
		camPos = {(float)sx + 0.5f, (float)feetY + 1.62f, (float)sz + 0.5f};
		yaw = 0.0f; pitch = -0.10f;
		g_peakY = camPos.y;
	}
	
	// ---- 联机初始化 ----
	if (g_isMultiplayer) {
		if (g_myPlayerId == 0 && g_remotePlayers.empty()) {
			RemotePlayer me;
			me.id = 0;
			me.pos = {0, 0, 0};
			me.yaw = 0; me.pitch = 0;
			me.active = true;
			g_remotePlayers.push_back(me);
		}
	}
	
	// 预刷 4 只动物
	for (int i = 0; i < 4; ++i) {
		float ang = (rand() % 1000) / 1000.0f * 6.283f;
		float dist = 15 + (rand() % 10);
		int mx = (int)(camPos.x + cosf(ang) * dist);
		int mz = (int)(camPos.z + sinf(ang) * dist);
		int my = -1;
		for (int y = CHUNK_SY - 2; y >= 0; --y)
			if (getBlock(mx, y, mz) != 0) { my = y + 1; break; }
		if (my > 0 && getBlock(mx, my - 1, mz) == 1) {
			Animal a;
			a.pos = {(float)mx + 0.5f, (float)my, (float)mz + 0.5f};
			a.velY = 0;
			a.type = rand() % 4;
			a.hp = (a.type == 2) ? 4 : (a.type == 1 ? 8 : 10);
			a.hurtTimer = 0;
			a.wanderTimer = (rand() % 300) / 100.0f;
			float ang2 = (rand() % 1000) / 1000.0f * 6.283f;
			a.dirX = cosf(ang2); a.dirZ = sinf(ang2);
			a.alive = true;
			g_animals.push_back(a);
		}
	}
	
	float vy = 0.0f;
	bool  onGround = false;
	
	int cx = GetSystemMetrics(SM_CXSCREEN)/2;
	int cy = GetSystemMetrics(SM_CYSCREEN)/2;
	SetCursorPos(cx, cy);
	
	bool prevLeft  = false;
	bool prevRight = false;
	bool prevE     = false;
	
	DWORD lastTick = GetTickCount();
	
	while (true) {
		int visW = getConsoleVisibleWidth();
		int visH = getConsoleVisibleHeight();
		SCR_W = visW - 2;
		SCR_H = visH - 8;
		if (SCR_W < 40) SCR_W = 40;
		if (SCR_H < 15) SCR_H = 15;
		if (SCR_W > 250) SCR_W = 250;
		if (SCR_H > 60)  SCR_H = 60;
		
		DWORD now = GetTickCount();
		float dt = (now - lastTick) / 1000.0f;
		lastTick = now;
		if (dt > 0.1f) dt = 0.1f;
		
		g_timeOfDay += dt / DAY_LENGTH;
		while (g_timeOfDay >= 1.0f) g_timeOfDay -= 1.0f;
		{
			float tod = g_timeOfDay;
			float L;
			if (tod < 0.25f)      L = 0.5f + tod * 2.0f;
			else if (tod < 0.5f)  L = 1.0f;
			else if (tod < 0.75f) L = 1.0f - (tod - 0.5f) * 3.2f;
			else                  L = 0.2f;
			if (L < 0.2f) L = 0.2f;
			if (L > 1.0f) L = 1.0f;
			g_globalLight = L;
		}
		
		if (bagOpen || tableOpen || chestOpen || furnaceOpen) {
			syncSystemCursor(true);
		} else {
			syncSystemCursor(false);
			SetCursor(NULL);
		}
		
		if (g_health <= 0) {
			remove(savePath().c_str());
			SetCursorPos(cx, cy);
			syncSystemCursor(true);
			hideCursor(false);
			clearScreen();
			cout << "\n\n\n";
			cout << "        ==============================\n";
			cout << "              你  死  了  ！\n";
			cout << "        ==============================\n\n";
			cout << "        存档已删除，但账号保留。\n\n";
			cout << "        按任意键返回菜单……\n";
			while (!_kbhit()) {}
			getch();
			return;
		}
		
		if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
			saveGame(camPos, yaw, pitch);
			break;
		}
		
		bool eNow = (GetAsyncKeyState('E') & 0x8000) != 0;
		if (eNow && !prevE) {
			if (furnaceOpen) {
				g_furnaces[g_openFurnace] = g_openFurnaceData;
				if (!cursorItem.empty()) { addItem(cursorItem.type, cursorItem.count); cursorItem.clear(); }
				furnaceOpen = false;
				hideCursor(true); syncSystemCursor(false); SetCursorPos(cx, cy);
			} else if (chestOpen) {
				g_chests[g_openChest] = g_openChestData;
				if (!cursorItem.empty()) { addItem(cursorItem.type, cursorItem.count); cursorItem.clear(); }
				chestOpen = false;
				hideCursor(true); syncSystemCursor(false); SetCursorPos(cx, cy);
			} else if (tableOpen) {
				for (int i = 0; i < 9; ++i)
					if (!inventory[41+i].empty()) {
						addItem(inventory[41+i].type, inventory[41+i].count);
						inventory[41+i].clear();
					}
				if (!cursorItem.empty()) { addItem(cursorItem.type, cursorItem.count); cursorItem.clear(); }
				tableOpen = false;
				hideCursor(true); syncSystemCursor(false); SetCursorPos(cx, cy);
			} else if (bagOpen) {
				if (!cursorItem.empty()) { addItem(cursorItem.type, cursorItem.count); cursorItem.clear(); }
				bagOpen = false;
				hideCursor(true); syncSystemCursor(false); SetCursorPos(cx, cy);
			} else {
				bagOpen = true;
				hideCursor(false); syncSystemCursor(true);
				POINT tl = {0, 0}; ClientToScreen(GetConsoleWindow(), &tl);
				SetCursorPos(tl.x + 40, tl.y + 80);
			}
			g_miningX = g_miningY = g_miningZ = -1;
			g_miningProgress = 0;
		}
		prevE = eNow;
		
		if (bagOpen) {
			updateCraft2x2();
			int mx, my; getMouseChar(mx, my);
			int hit = hitBagSlot(mx, my);
			bool clickNow  = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
			bool rclickNow = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
			if (clickNow && !prevLeft) {
				if (hit == 40) { int slots[4] = {36,37,38,39}; doCraftFrom(slots, 4); }
				else if (hit >= 0) clickSlot(hit);
			}
			if (rclickNow && !prevRight && hit >= 0 && hit != 40) clickSlotRight(hit);
			prevLeft  = clickNow;
			prevRight = rclickNow;
			renderBagScreen();
			Sleep(30);
			continue;
		}
		if (tableOpen) {
			updateCraft3x3();
			int mx, my; getMouseChar(mx, my);
			int hit = hitTableSlot(mx, my);
			bool clickNow  = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
			bool rclickNow = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
			if (clickNow && !prevLeft) {
				if (hit == 50) { int slots[9] = {41,42,43,44,45,46,47,48,49}; doCraftFrom(slots, 9); }
				else if (hit >= 0) clickSlot(hit);
			}
			if (rclickNow && !prevRight && hit >= 0 && hit != 50) clickSlotRight(hit);
			prevLeft  = clickNow;
			prevRight = rclickNow;
			renderTableScreen();
			Sleep(30);
			continue;
		}
		if (chestOpen) {
			int mx, my; getMouseChar(mx, my);
			int hit = hitChestSlot(mx, my);
			bool clickNow  = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
			bool rclickNow = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
			if (clickNow && !prevLeft && hit != -1)   clickChestSlot(hit);
			if (rclickNow && !prevRight && hit != -1) clickChestSlotRight(hit);
			prevLeft  = clickNow;
			prevRight = rclickNow;
			renderChestScreen();
			Sleep(30);
			continue;
		}
		if (furnaceOpen) {
			updateFurnace(g_openFurnaceData, dt);
			int mx, my; getMouseChar(mx, my);
			int hit = hitFurnaceSlot(mx, my);
			bool clickNow  = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
			bool rclickNow = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
			if (clickNow && !prevLeft && hit != -1)   clickFurnaceSlot(hit);
			if (rclickNow && !prevRight && hit != -1) clickFurnaceSlotRight(hit);
			prevLeft  = clickNow;
			prevRight = rclickNow;
			renderFurnaceScreen();
			Sleep(30);
			continue;
		}
		
		POINT pt;
		GetCursorPos(&pt);
		float dx = (float)(pt.x - cx);
		float dy = (float)(pt.y - cy);
		SetCursorPos(cx, cy);
		SetCursor(NULL);
		SetCursor(NULL);
		
		const float SENS = 0.0030f;
		yaw   += dx * SENS;
		pitch -= dy * SENS;
		const float LIMIT = 1.45f;
		if (pitch >  LIMIT) pitch =  LIMIT;
		if (pitch < -LIMIT) pitch = -LIMIT;
		
		Vec3 forward = {cosf(pitch)*sinf(yaw), sinf(pitch), cosf(pitch)*cosf(yaw)};
		Vec3 worldUp = {0, 1, 0};
		Vec3 right = worldUp.cross(forward);
		if (right.x*right.x + right.y*right.y + right.z*right.z < 1e-6f) right = Vec3{1,0,0};
		right = right.normalize();
		
		Vec3 flatFwd = {forward.x, 0, forward.z};
		float fl = sqrtf(flatFwd.x*flatFwd.x + flatFwd.z*flatFwd.z);
		if (fl > 1e-4f) { flatFwd.x/=fl; flatFwd.z/=fl; } else flatFwd = {0,0,0};
		Vec3 flatRight = {right.x, 0, right.z};
		fl = sqrtf(flatRight.x*flatRight.x + flatRight.z*flatRight.z);
		if (fl > 1e-4f) { flatRight.x/=fl; flatRight.z/=fl; } else flatRight = {0,0,0};
		
		float speed = 0.15f;
		if (GetAsyncKeyState(VK_CONTROL) & 0x8000) speed *= 1.9f;
		
		float mvx = 0, mvz = 0;
		if (GetAsyncKeyState('W') & 0x8000) { mvx += flatFwd.x;   mvz += flatFwd.z;   }
		if (GetAsyncKeyState('S') & 0x8000) { mvx -= flatFwd.x;   mvz -= flatFwd.z;   }
		if (GetAsyncKeyState('A') & 0x8000) { mvx -= flatRight.x; mvz -= flatRight.z; }
		if (GetAsyncKeyState('D') & 0x8000) { mvx += flatRight.x; mvz += flatRight.z; }
		
		float ml = sqrtf(mvx*mvx + mvz*mvz);
		if (ml > 1e-4f) { mvx = mvx/ml*speed; mvz = mvz/ml*speed; }
		else            { mvx = 0; mvz = 0; }
		
		vy += GRAVITY;
		if (vy < MAX_FALL) vy = MAX_FALL;
		if ((GetAsyncKeyState(VK_SPACE) & 0x8000) && onGround) { vy = JUMP_VEL; onGround = false; }
		
		if (onGround) g_peakY = camPos.y;
		else if (camPos.y > g_peakY) g_peakY = camPos.y;
		
		bool wasOnGround = onGround;
		Vec3 before = camPos;
		float gotX = sweepAxis(camPos, 0, mvx);
		if (fabsf(mvx) > 1e-6f && fabsf(gotX-mvx) > 0.005f && onGround) {
			Vec3 saved = camPos; camPos = before;
			if (!tryStepUp(camPos, mvx, 0)) camPos = saved;
		}
		Vec3 beforeZ = camPos;
		float gotZ = sweepAxis(camPos, 2, mvz);
		if (fabsf(mvz) > 1e-6f && fabsf(gotZ-mvz) > 0.005f && onGround) {
			Vec3 saved = camPos; camPos = beforeZ;
			if (!tryStepUp(camPos, 0, mvz)) camPos = saved;
		}
		sweepAxis(camPos, 1, vy);
		
		{
			Vec3 probe = camPos; probe.y -= 0.06f;
			if (playerCollides(probe) && vy <= 0) { onGround = true; vy = 0; }
			else                                  { onGround = false; }
		}
		
		if (!wasOnGround && onGround) {
			float fallDist = g_peakY - camPos.y;
			if (fallDist >= 4.0f) {
				int dmg = (int)((fallDist - 3.0f) * 2.0f);
				if (dmg < 2) dmg = 2;
				g_health -= dmg;
				if (g_health < 0) g_health = 0;
				g_healthAnimTimer = 1.5f;
			}
		}
		
		g_foodTimer += dt;
		if (g_foodTimer >= FOOD_DRAIN_INTERVAL) { g_foodTimer = 0; if (g_food > 0) g_food--; }
		if (g_food >= FOOD_HEAL_THRESHOLD && g_health < MAX_HEALTH) {
			g_healTimer += dt;
			if (g_healTimer >= FOOD_HEAL_INTERVAL) {
				g_healTimer = 0;
				g_health = min(MAX_HEALTH, g_health + 1);
				g_healthAnimTimer = 0.6f;
			}
		} else g_healTimer = 0;
		if (g_healthAnimTimer > 0) g_healthAnimTimer -= dt;
		if (g_healthAnimTimer < 0) g_healthAnimTimer = 0;
		
		updateItems(camPos, dt);
		updateZombies(camPos, dt);
		updateAnimals(camPos, dt);
		
		g_handSwing -= dt * 5.0f;
		if (g_handSwing < 0) g_handSwing = 0;
		
		for (int k = 0; k < 9; ++k)
			if (GetAsyncKeyState('1'+k) & 0x8000) hotbarSelected = k;
		
		bool leftNow  = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
		bool rightNow = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
		const float REACH = 6.0f;
		
		// 左键攻击
		if (leftNow && !prevLeft) {
			Vec3 ray = forward;
			bool hitEntity = false;
			
			for (auto& z : g_zombies) {
				if (!z.alive) continue;
				Vec3 mobCenter = {z.pos.x, z.pos.y + 0.9f, z.pos.z};
				Vec3 toMob = mobCenter - camPos;
				float t = toMob.dot(ray);
				if (t < 0 || t > REACH) continue;
				Vec3 closest = camPos + ray * t;
				Vec3 diff = closest - mobCenter;
				float distSq = diff.x*diff.x + diff.y*diff.y + diff.z*diff.z;
				if (distSq < 0.8f) {
					int dmg = 1;
					int toolT = inventory[hotbarSelected].type;
					if (isSword(toolT)) dmg = (toolT == 24) ? 4 : (toolT == 25 ? 5 : 7);
					else if (isTool(toolT)) dmg = 2;
					z.hp -= dmg;
					z.hurtTimer = 0.3f;
					g_handSwing = 1.5f;
					if (z.hp <= 0) {
						z.alive = false;
						spawnItemDrop((int)z.pos.x, (int)z.pos.y, (int)z.pos.z, 15, 1 + rand()%2);
					}
					hitEntity = true;
					break;
				}
			}
			
			if (!hitEntity) {
				for (auto& a : g_animals) {
					if (!a.alive) continue;
					float ah = (a.type == 2) ? 0.4f : 0.9f;
					Vec3 mobCenter = {a.pos.x, a.pos.y + ah, a.pos.z};
					Vec3 toMob = mobCenter - camPos;
					float t = toMob.dot(ray);
					if (t < 0 || t > REACH) continue;
					Vec3 closest = camPos + ray * t;
					Vec3 diff = closest - mobCenter;
					float distSq = diff.x*diff.x + diff.y*diff.y + diff.z*diff.z;
					if (distSq < 0.8f) {
						int dmg = 1;
						int toolT = inventory[hotbarSelected].type;
						if (isSword(toolT)) dmg = (toolT == 24) ? 4 : (toolT == 25 ? 5 : 7);
						else if (isTool(toolT)) dmg = 2;
						a.hp -= dmg;
						a.hurtTimer = 0.3f;
						g_handSwing = 1.5f;
						if (a.hp <= 0) {
							a.alive = false;
							int dropType = 0;
							if (a.type == 0) dropType = 27;
							if (a.type == 1) dropType = 29;
							if (a.type == 2) dropType = 31;
							if (a.type == 3) dropType = 33;
							int count = (a.type == 2) ? 1 : (1 + rand() % 2);
							spawnItemDrop((int)a.pos.x, (int)a.pos.y, (int)a.pos.z, dropType, count);
							if (a.type == 1)
								spawnItemDrop((int)a.pos.x, (int)a.pos.y, (int)a.pos.z, 35, 1 + rand() % 2);
						}
						hitEntity = true;
						break;
					}
				}
			}
			
			if (!hitEntity) g_handSwing = 1.5f;
		}
		
		// 长按挖掘
		if (leftNow) {
			RayHit hit = raycastDDA(camPos, forward, REACH);
			if (hit.hit && hit.blockType >= 1 && hit.blockType <= 36) {
				if (hit.bx == g_miningX && hit.by == g_miningY && hit.bz == g_miningZ) {
					int toolType = inventory[hotbarSelected].type;
					if (!isTool(toolType)) toolType = 0;
					float digTime = getDigTime(hit.blockType, toolType);
					g_miningProgress += dt / digTime;
					if (g_handSwing < 0.8f) g_handSwing = 0.8f;
					if (g_miningProgress >= 1.0f) {
						modifyBlock(hit.bx, hit.by, hit.bz, 0);
						if (hit.blockType == 5) {
							if (rand() % 100 < 25) spawnItemDrop(hit.bx, hit.by, hit.bz, 15, 1);
						} else if (hit.blockType == 16) {
							auto key = make_tuple(hit.bx, hit.by, hit.bz);
							auto it = g_chests.find(key);
							if (it != g_chests.end()) {
								for (auto& st : it->second.slots)
									if (!st.empty()) spawnItemDrop(hit.bx, hit.by, hit.bz, st.type, st.count);
								g_chests.erase(it);
							}
						} else if (hit.blockType == 17) {
							auto key = make_tuple(hit.bx, hit.by, hit.bz);
							auto it = g_furnaces.find(key);
							if (it != g_furnaces.end()) {
								if (!it->second.input.empty())  spawnItemDrop(hit.bx, hit.by, hit.bz, it->second.input.type,  it->second.input.count);
								if (!it->second.fuel.empty())   spawnItemDrop(hit.bx, hit.by, hit.bz, it->second.fuel.type,   it->second.fuel.count);
								if (!it->second.output.empty()) spawnItemDrop(hit.bx, hit.by, hit.bz, it->second.output.type, it->second.output.count);
								g_furnaces.erase(it);
							}
						} else {
							spawnItemDrop(hit.bx, hit.by, hit.bz, hit.blockType, 1);
						}
						ItemStack& sel = inventory[hotbarSelected];
						if ((isTool(sel.type) || isSword(sel.type)) && sel.durability > 0) {
							sel.durability--;
							if (sel.durability <= 0) sel.clear();
						}
						g_miningProgress = 0;
						g_miningX = g_miningY = g_miningZ = -1;
						g_handSwing = 1.5f;
					}
				} else {
					g_miningX = hit.bx; g_miningY = hit.by; g_miningZ = hit.bz;
					g_miningProgress = 0;
				}
			} else {
				g_miningX = g_miningY = g_miningZ = -1;
				g_miningProgress = 0;
			}
		} else {
			g_miningX = g_miningY = g_miningZ = -1;
			g_miningProgress = 0;
		}
		
		// 右键
		if (rightNow && !prevRight) {
			ItemStack& sel = inventory[hotbarSelected];
			
			{
				RayHit bedHit = raycastDDA(camPos, forward, REACH);
				if (bedHit.hit && bedHit.blockType == 36) {
					if (g_globalLight < 0.55f) {
						g_timeOfDay = 0.05f;
						g_globalLight = 1.0f;
						if (g_health < MAX_HEALTH) {
							g_health = min(MAX_HEALTH, g_health + 6);
							g_healthAnimTimer = 1.0f;
						}
						g_handSwing = 1.5f;
					}
					prevLeft = leftNow;
					prevRight = rightNow;
					continue;
				}
			}
			
			if (isFood(sel.type)) {
				if (g_food < MAX_FOOD) {
					int fv = getFoodValue(sel.type);
					g_food = min(MAX_FOOD, g_food + fv);
					sel.count--;
					if (sel.count <= 0) sel.clear();
				}
				g_handSwing = 1.5f;
			} else {
				RayHit hit = raycastDDA(camPos, forward, REACH);
				if (hit.hit && hit.blockType == 8) {
					tableOpen = true;
					hideCursor(false); syncSystemCursor(true);
					POINT tl = {0, 0}; ClientToScreen(GetConsoleWindow(), &tl);
					SetCursorPos(tl.x + 40, tl.y + 80);
					g_miningX = g_miningY = g_miningZ = -1;
					g_miningProgress = 0;
				} else if (hit.hit && hit.blockType == 16) {
					chestOpen = true;
					g_openChest = make_tuple(hit.bx, hit.by, hit.bz);
					auto it = g_chests.find(g_openChest);
					if (it != g_chests.end()) g_openChestData = it->second;
					else { g_openChestData = ChestData(); g_chests[g_openChest] = g_openChestData; }
					hideCursor(false); syncSystemCursor(true);
					POINT tl = {0, 0}; ClientToScreen(GetConsoleWindow(), &tl);
					SetCursorPos(tl.x + 40, tl.y + 80);
					g_miningX = g_miningY = g_miningZ = -1;
					g_miningProgress = 0;
				} else if (hit.hit && hit.blockType == 17) {
					furnaceOpen = true;
					g_openFurnace = make_tuple(hit.bx, hit.by, hit.bz);
					auto it = g_furnaces.find(g_openFurnace);
					if (it != g_furnaces.end()) g_openFurnaceData = it->second;
					else { g_openFurnaceData = FurnaceData(); g_furnaces[g_openFurnace] = g_openFurnaceData; }
					hideCursor(false); syncSystemCursor(true);
					POINT tl = {0, 0}; ClientToScreen(GetConsoleWindow(), &tl);
					SetCursorPos(tl.x + 40, tl.y + 80);
					g_miningX = g_miningY = g_miningZ = -1;
					g_miningProgress = 0;
				} else if (sel.empty()) {
				} else if (!canPlace(sel.type)) {
				} else if (hit.hit) {
					int nx = hit.bx, ny = hit.by, nz = hit.bz;
					switch (hit.face) {
						case 0: nx += 1; break;
						case 1: nx -= 1; break;
						case 2: ny += 1; break;
						case 3: ny -= 1; break;
						case 4: nz += 1; break;
						case 5: nz -= 1; break;
					}
					if (getBlock(nx, ny, nz) == 0 && ny >= 0 && ny < CHUNK_SY) {
						float px0, py0, pz0, px1, py1, pz1;
						playerBox(camPos, px0, py0, pz0, px1, py1, pz1);
						bool overlap = !(px1<=nx || px0>=nx+1 ||
										 py1<=ny || py0>=ny+1 ||
										 pz1<=nz || pz0>=nz+1);
						if (!overlap) {
							modifyBlock(nx, ny, nz, (uint8_t)sel.type);
							sel.count--;
							if (sel.count <= 0) sel.clear();
							g_handSwing = 1.5f;
						}
					}
				}
			}
		}
		prevLeft = leftNow;
		prevRight = rightNow;
		
		// ---- 联机同步 ----
		if (g_isMultiplayer) {
			if (g_myPlayerId == 0) {
				mpServerHostUpdate(camPos, yaw, pitch);
				mpServerTick();
			} else {
				static int sendTick = 0;
				sendTick++;
				if (sendTick % 2 == 0) {
					mpClientSendState(camPos, yaw, pitch);
				}
				mpClientTick();
			}
		}
		
		Vec3 camTarget = camPos + forward;
		renderFrame(camPos, camTarget);
		renderUI();
		renderHand(inventory[hotbarSelected].type);
		
		Sleep(20);
	}
	
	SetCursorPos(cx, cy);
	syncSystemCursor(true);
	hideCursor(false);
	clearScreen();
}
