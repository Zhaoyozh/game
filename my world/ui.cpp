// ============================================================================
//  ui.cpp — 渲染 / 生物精灵 / 远程玩家 / HUD / 界面
// ============================================================================
#include "common.h"

static void appendGoto(string& out, int row, int col) {
	char buf[40];
	sprintf(buf, "\x1b[%d;%dH", row, col);
	out += buf;
}

// ============================================================
//  屏幕投影
// ============================================================
struct ScreenProj {
	bool  visible = false;
	int   sx = 0, sy = 0;
	float camZ = 0;
};

static ScreenProj projectToScreen(const Vec3& worldPos,
								  const Vec3& camPos,
								  const Vec3& forward,
								  const Vec3& right,
								  const Vec3& up,
								  float halfW, float halfH,
								  int W, int H) {
	ScreenProj p;
	Vec3 rel = worldPos - camPos;
	float camZ = rel.dot(forward);
	if (camZ < 0.1f || camZ > 120.0f) return p;
	float camX = rel.dot(right);
	float camY = rel.dot(up);
	float ndcX = camX / (camZ * halfW);
	float ndcY = camY / (camZ * halfH);
	if (ndcX < -1.6f || ndcX > 1.6f || ndcY < -1.6f || ndcY > 1.6f) return p;
	p.visible = true;
	p.sx = (int)((ndcX + 1.0f) * 0.5f * W);
	p.sy = (int)((1.0f - (ndcY + 1.0f) * 0.5f) * H);
	p.camZ = camZ;
	return p;
}

static int worldPixelsW(float worldSize, float camZ, float halfW, int W) {
	float r = worldSize * W / (2.0f * camZ * halfW);
	if (r < 1) r = 1;
	if (r > 80) r = 80;
	return (int)r;
}
static int worldPixelsH(float worldSize, float camZ, float halfH, int H) {
	float r = worldSize * H / (2.0f * camZ * halfH);
	if (r < 1) r = 1;
	if (r > 60) r = 60;
	return (int)r;
}

// ============================================================
//  像素绘制
// ============================================================
static void putPixel(int px, int py, uint8_t r, uint8_t g, uint8_t b, float camZ,
					 int W, int H,
					 vector<float>&   zBuf,
					 vector<uint8_t>& rBuf,
					 vector<uint8_t>& gBuf,
					 vector<uint8_t>& bBuf) {
	if (px < 0 || px >= W || py < 0 || py >= H) return;
	int idx = py * W + px;
	if (camZ - 0.3f < zBuf[idx]) {
		rBuf[idx] = r; gBuf[idx] = g; bBuf[idx] = b;
		zBuf[idx] = camZ - 0.3f;
	}
}
static void putRect(int x0, int y0, int x1, int y1,
					uint8_t r, uint8_t g, uint8_t b, float camZ,
					int W, int H,
					vector<float>&   zBuf,
					vector<uint8_t>& rBuf,
					vector<uint8_t>& gBuf,
					vector<uint8_t>& bBuf) {
	if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
	if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }
	for (int y = y0; y <= y1; ++y)
		for (int x = x0; x <= x1; ++x)
			putPixel(x, y, r, g, b, camZ, W, H, zBuf, rBuf, gBuf, bBuf);
}

// ============================================================
//  动物精灵
// ============================================================
static void drawAnimalSprite(const Animal& a,
							 const Vec3& camPos, const Vec3& forward,
							 const Vec3& right, const Vec3& up,
							 float halfW, float halfH,
							 int W, int H,
							 vector<float>& zBuf,
							 vector<uint8_t>& rBuf,
							 vector<uint8_t>& gBuf,
							 vector<uint8_t>& bBuf) {
	ScreenProj p = projectToScreen(a.pos, camPos, forward, right, up, halfW, halfH, W, H);
	if (!p.visible) return;
	
	float bodyW, bodyH;
	switch (a.type) {
		case 0: bodyW = 0.55f; bodyH = 0.50f; break;
		case 1: bodyW = 0.50f; bodyH = 0.48f; break;
		case 2: bodyW = 0.22f; bodyH = 0.28f; break;
		case 3: bodyW = 0.50f; bodyH = 0.42f; break;
		default: bodyW = 0.5f; bodyH = 0.5f;
	}
	int halfPx = worldPixelsW(bodyW, p.camZ, halfW, W);
	int fullH  = worldPixelsH(bodyH * 2.0f, p.camZ, halfH, H);
	if (halfPx < 2) halfPx = 2;
	if (fullH  < 3) fullH  = 3;
	
	int sx = p.sx, sy = p.sy;
	
	if (a.hurtTimer > 0) {
		putRect(sx - halfPx, sy - fullH, sx + halfPx, sy,
				255, 255, 255, p.camZ, W, H, zBuf, rBuf, gBuf, bBuf);
		return;
	}
	
	uint8_t bodyR, bodyG, bodyB;
	uint8_t headR, headG, headB;
	uint8_t legR,  legG,  legB;
	switch (a.type) {
		case 0: bodyR=115;bodyG=70;bodyB=40; headR=85;headG=50;headB=30; legR=70;legG=42;legB=25; break;
		case 1: bodyR=240;bodyG=240;bodyB=232; headR=200;headG=200;headB=190; legR=60;legG=60;legB=60; break;
		case 2: bodyR=245;bodyG=220;bodyB=100; headR=235;headG=200;headB=85; legR=240;legG=155;legB=55; break;
		case 3: bodyR=240;bodyG=158;bodyB=168; headR=220;headG=128;headB=148; legR=200;legG=120;legB=130; break;
		default: bodyR=bodyG=bodyB=128; headR=headG=headB=100; legR=legG=legB=80;
	}
	
	int legH = fullH / 5; if (legH < 1) legH = 1;
	int legW = halfPx / 5; if (legW < 1) legW = 1;
	int legY0 = sy - legH;
	int legOffsets[4] = { -halfPx + 1, -halfPx / 3, halfPx / 3 - legW, halfPx - 1 - legW };
	for (int i = 0; i < 4; ++i)
		putRect(sx + legOffsets[i], legY0,
				sx + legOffsets[i] + legW - 1, sy,
				legR, legG, legB, p.camZ, W, H, zBuf, rBuf, gBuf, bBuf);
	
	int bodyY1 = legY0;
	int bodyY0 = legY0 - (fullH * 3 / 5);
	if (bodyY0 > bodyY1) bodyY0 = bodyY1;
	putRect(sx - halfPx, bodyY0, sx + halfPx, bodyY1,
			bodyR, bodyG, bodyB, p.camZ, W, H, zBuf, rBuf, gBuf, bBuf);
	
	int headY1 = bodyY0;
	int headY0 = sy - fullH;
	if (headY0 > headY1) headY0 = headY1;
	int headHalf = halfPx * 7 / 10;
	if (headHalf < 2) headHalf = 2;
	putRect(sx - headHalf, headY0, sx + headHalf, headY1,
			headR, headG, headB, p.camZ, W, H, zBuf, rBuf, gBuf, bBuf);
	
	int eyeY = headY0 + (headY1 - headY0) / 2;
	if (headHalf >= 2) {
		putPixel(sx - headHalf + 1, eyeY, 10, 10, 10, p.camZ, W, H, zBuf, rBuf, gBuf, bBuf);
		putPixel(sx + headHalf - 1, eyeY, 10, 10, 10, p.camZ, W, H, zBuf, rBuf, gBuf, bBuf);
	}
	
	if (a.type == 0) {
		if (halfPx > 3 && (bodyY1 - bodyY0) > 2) {
			putRect(sx - halfPx + 2, bodyY0 + 1, sx - halfPx + 4, bodyY0 + 2,
					245, 245, 245, p.camZ, W, H, zBuf, rBuf, gBuf, bBuf);
			putRect(sx + halfPx - 4, bodyY0 + 2, sx + halfPx - 2, bodyY0 + 3,
					245, 245, 245, p.camZ, W, H, zBuf, rBuf, gBuf, bBuf);
		}
	} else if (a.type == 2) {
		if (headHalf >= 1) {
			putPixel(sx,     headY0 - 1, 210, 40, 40, p.camZ, W, H, zBuf, rBuf, gBuf, bBuf);
			putPixel(sx - 1, headY0 - 1, 210, 40, 40, p.camZ, W, H, zBuf, rBuf, gBuf, bBuf);
			putPixel(sx + 1, headY0 - 1, 210, 40, 40, p.camZ, W, H, zBuf, rBuf, gBuf, bBuf);
		}
		if (headHalf >= 2)
			putPixel(sx, headY1, 240, 150, 40, p.camZ, W, H, zBuf, rBuf, gBuf, bBuf);
	} else if (a.type == 3) {
		if (headHalf >= 2)
			putRect(sx - 1, headY1 - 1, sx + 1, headY1,
					190, 100, 120, p.camZ, W, H, zBuf, rBuf, gBuf, bBuf);
	}
}

// ============================================================
//  僵尸精灵
// ============================================================
static void drawZombieSprite(const Mob& z,
							 const Vec3& camPos, const Vec3& forward,
							 const Vec3& right, const Vec3& up,
							 float halfW, float halfH,
							 int W, int H,
							 vector<float>& zBuf,
							 vector<uint8_t>& rBuf,
							 vector<uint8_t>& gBuf,
							 vector<uint8_t>& bBuf) {
	Vec3 feet = {z.pos.x, z.pos.y, z.pos.z};
	ScreenProj p = projectToScreen(feet, camPos, forward, right, up, halfW, halfH, W, H);
	if (!p.visible) return;
	
	int halfPx = worldPixelsW(0.35f, p.camZ, halfW, W);
	int fullH  = worldPixelsH(1.8f,  p.camZ, halfH, H);
	if (halfPx < 3) halfPx = 3;
	if (fullH  < 6) fullH  = 6;
	
	int sx = p.sx, sy = p.sy;
	
	if (z.hurtTimer > 0) {
		putRect(sx - halfPx, sy - fullH, sx + halfPx, sy,
				255, 240, 240, p.camZ, W, H, zBuf, rBuf, gBuf, bBuf);
		return;
	}
	
	uint8_t bodyR=70, bodyG=130, bodyB=70;
	uint8_t headR=95, headG=155, headB=95;
	uint8_t legR=55,  legG=100, legB=55;
	uint8_t shirtR=60, shirtG=80, shirtB=60;
	
	int legH = fullH / 4;
	int legW = halfPx / 3; if (legW < 1) legW = 1;
	int legY0 = sy - legH;
	putRect(sx - halfPx + 1, legY0, sx - halfPx + legW, sy,
			legR, legG, legB, p.camZ, W, H, zBuf, rBuf, gBuf, bBuf);
	putRect(sx + halfPx - legW, legY0, sx + halfPx - 1, sy,
			legR, legG, legB, p.camZ, W, H, zBuf, rBuf, gBuf, bBuf);
	
	int bodyY1 = legY0;
	int bodyY0 = legY0 - (fullH * 2 / 5);
	putRect(sx - halfPx, bodyY0, sx + halfPx, bodyY1,
			shirtR, shirtG, shirtB, p.camZ, W, H, zBuf, rBuf, gBuf, bBuf);
	
	int armW = halfPx / 2; if (armW < 1) armW = 1;
	int armY = bodyY0 + 1;
	putRect(sx - halfPx - armW, armY, sx - halfPx - 1, armY + (bodyY1 - bodyY0) / 2,
			bodyR, bodyG, bodyB, p.camZ, W, H, zBuf, rBuf, gBuf, bBuf);
	putRect(sx + halfPx + 1, armY, sx + halfPx + armW, armY + (bodyY1 - bodyY0) / 2,
			bodyR, bodyG, bodyB, p.camZ, W, H, zBuf, rBuf, gBuf, bBuf);
	
	int headY1 = bodyY0;
	int headY0 = sy - fullH;
	putRect(sx - halfPx, headY0, sx + halfPx, headY1,
			headR, headG, headB, p.camZ, W, H, zBuf, rBuf, gBuf, bBuf);
	
	int eyeY = headY0 + (headY1 - headY0) / 2;
	if (halfPx >= 2) {
		putPixel(sx - halfPx + 1, eyeY, 30, 0, 0, p.camZ, W, H, zBuf, rBuf, gBuf, bBuf);
		putPixel(sx + halfPx - 1, eyeY, 30, 0, 0, p.camZ, W, H, zBuf, rBuf, gBuf, bBuf);
	}
}

// ============================================================
//  远程玩家（联机）
// ============================================================
static void drawRemotePlayer(const RemotePlayer& rp,
							 const Vec3& camPos, const Vec3& forward,
							 const Vec3& right, const Vec3& up,
							 float halfW, float halfH,
							 int W, int H,
							 vector<float>& zBuf,
							 vector<uint8_t>& rBuf,
							 vector<uint8_t>& gBuf,
							 vector<uint8_t>& bBuf) {
	if (rp.id == g_myPlayerId) return;
	
	ScreenProj p = projectToScreen(rp.pos, camPos, forward, right, up, halfW, halfH, W, H);
	if (!p.visible) return;
	
	int halfPx = worldPixelsW(0.35f, p.camZ, halfW, W);
	int fullH  = worldPixelsH(1.8f,  p.camZ, halfH, H);
	if (halfPx < 3) halfPx = 3;
	if (fullH  < 6) fullH  = 6;
	
	int sx = p.sx, sy = p.sy;
	
	// 腿
	int legH = fullH / 4;
	int legW = halfPx / 3; if (legW < 1) legW = 1;
	putRect(sx - halfPx + 1, sy - legH, sx - halfPx + legW, sy,
			40, 50, 90, p.camZ, W, H, zBuf, rBuf, gBuf, bBuf);
	putRect(sx + halfPx - legW, sy - legH, sx + halfPx - 1, sy,
			40, 50, 90, p.camZ, W, H, zBuf, rBuf, gBuf, bBuf);
	
	// 身体（蓝衣）
	int bodyY1 = sy - legH;
	int bodyY0 = bodyY1 - (fullH * 2 / 5);
	putRect(sx - halfPx, bodyY0, sx + halfPx, bodyY1,
			50, 90, 200, p.camZ, W, H, zBuf, rBuf, gBuf, bBuf);
	
	// 手臂
	int armW = halfPx / 2; if (armW < 1) armW = 1;
	int armY = bodyY0 + 1;
	putRect(sx - halfPx - armW, armY, sx - halfPx - 1, armY + (bodyY1 - bodyY0) / 2,
			60, 120, 220, p.camZ, W, H, zBuf, rBuf, gBuf, bBuf);
	putRect(sx + halfPx + 1, armY, sx + halfPx + armW, armY + (bodyY1 - bodyY0) / 2,
			60, 120, 220, p.camZ, W, H, zBuf, rBuf, gBuf, bBuf);
	
	// 头（肤色）
	int headY1 = bodyY0;
	int headY0 = sy - fullH;
	putRect(sx - halfPx, headY0, sx + halfPx, headY1,
			220, 175, 140, p.camZ, W, H, zBuf, rBuf, gBuf, bBuf);
	
	// 眼睛
	int eyeY = headY0 + (headY1 - headY0) / 2;
	if (halfPx >= 2) {
		putPixel(sx - halfPx + 1, eyeY, 20, 20, 20, p.camZ, W, H, zBuf, rBuf, gBuf, bBuf);
		putPixel(sx + halfPx - 1, eyeY, 20, 20, 20, p.camZ, W, H, zBuf, rBuf, gBuf, bBuf);
	}
}

// ============================================================
//  格子文本
// ============================================================
string slotTextFrom(const ItemStack& s, bool highlight) {
	string out;
	out += highlight ? "\x1b[97m[" : "\x1b[90m[";
	if (s.empty()) out += "   ";
	else {
		RGB c = getItemColor(s.type);
		const char* icon = "   ";
		if (isSword(s.type)) icon = " / ";
		else if (isTool(s.type)) {
			if (s.type == 9 || s.type == 12 || s.type == 21) icon = " A ";
			else if (s.type == 10 || s.type == 13 || s.type == 22) icon = " P ";
			else icon = " S ";
		}
		else if (s.type == 7)  icon = " | ";
		else if (s.type == 15) icon = " o ";
		else if (s.type == 19) icon = " * ";
		else if (s.type == 20) icon = " = ";
		else if (s.type == 18) icon = " O ";
		else if (s.type == 17) icon = " ^ ";
		else if (s.type == 16) icon = " # ";
		else if (s.type == 8)  icon = " T ";
		else if (s.type == 27) icon = " b ";
		else if (s.type == 28) icon = " B ";
		else if (s.type == 29) icon = " m ";
		else if (s.type == 30) icon = " M ";
		else if (s.type == 31) icon = " c ";
		else if (s.type == 32) icon = " C ";
		else if (s.type == 33) icon = " p ";
		else if (s.type == 34) icon = " P ";
		else if (s.type == 35) icon = " W ";
		else if (s.type == 36) icon = " B ";
		else if (s.type == 37) icon = " X ";
		
		char buf[128];
		sprintf(buf, "\x1b[48;2;%d;%d;%dm\x1b[97m%s\x1b[0m",
				c.r, c.g, c.b, icon);
		out += buf;
	}
	out += ' ';
	if (s.empty()) out += "   ";
	else { char buf[8]; sprintf(buf, "%3d", s.count); out += "\x1b[97m"; out += buf; }
	out += "\x1b[90m]";
	out += (highlight ? "\x1b[97m" : "\x1b[39m");
	out += "\x1b[0m";
	return out;
}
string itemSlotText(int idx, bool highlight) {
	if (idx < 0 || idx >= INV_SIZE) {
		ItemStack e;
		return slotTextFrom(e, highlight);
	}
	return slotTextFrom(inventory[idx], highlight);
}

// ============================================================
//  整帧渲染
// ============================================================
void renderFrame(const Vec3& camPos, const Vec3& camTarget) {
	int W = SCR_W, H = SCR_H;
	if (W < 20 || H < 10) return;
	
	int visW = getConsoleVisibleWidth();
	int visH = getConsoleVisibleHeight();
	
	Vec3 forward = (camTarget - camPos).normalize();
	Vec3 worldUp = {0, 1, 0};
	Vec3 right = worldUp.cross(forward);
	if (right.x*right.x + right.y*right.y + right.z*right.z < 1e-6f)
		right = Vec3{1, 0, 0};
	right = right.normalize();
	Vec3 up = forward.cross(right).normalize();
	
	float fov = 70.0f * 3.14159265f / 180.0f;
	float halfW = tanf(fov * 0.5f);
	float halfH = halfW * (float)H / W * 2.0f;
	
	const int chX = W / 2, chY = H / 2;
	
	vector<float>   zBuf(W * H, 1e30f);
	vector<uint8_t> rBuf(W * H, 0);
	vector<uint8_t> gBuf(W * H, 0);
	vector<uint8_t> bBuf(W * H, 0);
	vector<uint8_t> chBuf(W * H, 0);
	
	for (int y = 0; y < H; ++y)
		for (int x = 0; x < W; ++x) {
			float u = (2.0f*(x+0.5f)/W - 1.0f) * halfW;
			float v = (1.0f - 2.0f*(y+0.5f)/H) * halfH;
			Vec3 rayDir = (forward + right*u + up*v).normalize();
			RayHit hit = raycastDDA(camPos, rayDir, 120.0f);
			int idx = y*W + x;
			if (hit.hit) {
				RGB c = getVoxelColor(hit.blockType, hit.face);
				float light = faceLight(hit.face);
				float fog = 1.0f - min(1.0f, hit.t/120.0f) * 0.45f;
				float depthDark = 0.25f + 0.75f * min(1.0f, (float)hit.by / 40.0f);
				float timeLight = g_globalLight;
				float ddx = hit.bx + 0.5f - camPos.x;
				float ddz = hit.bz + 0.5f - camPos.z;
				float distToPlayer = sqrtf(ddx*ddx + ddz*ddz);
				float nearLight = 1.0f;
				if (distToPlayer < 6.0f) nearLight = 1.0f + (6.0f - distToPlayer) * 0.08f;
				float totalLight = light * fog * depthDark * timeLight * nearLight;
				if (totalLight > 1.5f) totalLight = 1.5f;
				
				float fr = c.r*totalLight, fg = c.g*totalLight, fb = c.b*totalLight;
				if (hit.bx == g_miningX && hit.by == g_miningY && hit.bz == g_miningZ
					&& g_miningProgress > 0.01f) {
					float hx = camPos.x + rayDir.x * hit.t;
					float hy = camPos.y + rayDir.y * hit.t;
					float hz = camPos.z + rayDir.z * hit.t;
					uint32_t hh = ((uint32_t)(hx*25)) ^ ((uint32_t)(hy*25)*73856093u)
					^ ((uint32_t)(hz*25)*19349663u);
					hh = ((hh>>16)^hh) * 0x45d9f3b;
					hh = ((hh>>16)^hh) * 0x45d9f3b;
					hh = (hh>>16)^hh;
					float vv = (hh & 0xFF) / 255.0f;
					if (vv < g_miningProgress * 1.05f) { fr *= 0.22f; fg *= 0.22f; fb *= 0.22f; }
				}
				rBuf[idx] = (uint8_t)min(255.0f, fr);
				gBuf[idx] = (uint8_t)min(255.0f, fg);
				bBuf[idx] = (uint8_t)min(255.0f, fb);
				zBuf[idx] = hit.t;
			} else {
				float t = (float)y / H;
				float dayR = 110 + 70*(1-t),  dayG = 160 + 60*(1-t),  dayB = 255;
				float nightR = 15 + 20*(1-t),  nightG = 15 + 25*(1-t),  nightB = 40 + 40*(1-t);
				float dawnR = 220 + 20*(1-t),  dawnG = 100 + 60*(1-t),  dawnB = 60 + 40*(1-t);
				
				float L = g_globalLight;
				float dawn = 0.0f;
				float tod = g_timeOfDay;
				if (tod > 0.18f && tod < 0.32f) dawn = 1.0f - fabsf((tod - 0.25f) * 12.5f);
				if (tod > 0.68f && tod < 0.82f) dawn = 1.0f - fabsf((tod - 0.75f) * 12.5f);
				
				float sr = nightR + (dayR - nightR) * (L - 0.2f) / 0.8f;
				float sg = nightG + (dayG - nightG) * (L - 0.2f) / 0.8f;
				float sb = nightB + (dayB - nightB) * (L - 0.2f) / 0.8f;
				if (dawn > 0) {
					sr = sr * (1 - dawn) + dawnR * dawn;
					sg = sg * (1 - dawn) + dawnG * dawn;
					sb = sb * (1 - dawn) + dawnB * dawn;
				}
				rBuf[idx] = (uint8_t)min(255.0f, sr);
				gBuf[idx] = (uint8_t)min(255.0f, sg);
				bBuf[idx] = (uint8_t)min(255.0f, sb);
				zBuf[idx] = 1e30f;
			}
			if (x == chX && y == chY) chBuf[idx] = 1;
		}
	
	// 掉落物
	for (auto& e : g_items) {
		Vec3 rel = e.pos - camPos;
		float camDist = rel.dot(forward);
		if (camDist < 0.1f || camDist > 100.0f) continue;
		float camX = rel.dot(right), camY = rel.dot(up), camZ = camDist;
		float ndcX = camX / (camZ*halfW), ndcY = camY / (camZ*halfH);
		if (ndcX < -1.3f || ndcX > 1.3f || ndcY < -1.3f || ndcY > 1.3f) continue;
		int sx = (int)((ndcX+1.0f)*0.5f*W);
		int sy = (int)((1.0f-(ndcY+1.0f)*0.5f)*H);
		int sizeX = (int)(0.25f*2.0f / (camZ*halfW*2.0f/W));
		int sizeY = (int)(0.25f*2.0f / (camZ*halfH*2.0f/H));
		if (sizeX < 1) sizeX = 1;
		if (sizeX > 6) sizeX = 6;
		if (sizeY < 1) sizeY = 1;
		if (sizeY > 6) sizeY = 6;
		RGB c = getItemColor(e.type);
		float pulse = 0.85f + 0.15f * sinf(e.spin);
		uint8_t rr = (uint8_t)min(255.0f, c.r*pulse);
		uint8_t gg = (uint8_t)min(255.0f, c.g*pulse);
		uint8_t bb = (uint8_t)min(255.0f, c.b*pulse);
		for (int dy = -sizeY/2; dy <= sizeY/2; ++dy)
			for (int dx = -sizeX/2; dx <= sizeX/2; ++dx) {
				int px = sx+dx, py = sy+dy;
				if (px < 0 || px >= W || py < 0 || py >= H) continue;
				int idx = py*W + px;
				if (camZ - 0.3f < zBuf[idx]) {
					rBuf[idx]=rr; gBuf[idx]=gg; bBuf[idx]=bb;
					zBuf[idx]=camZ-0.3f;
				}
			}
	}
	
	// 僵尸
	for (auto& z : g_zombies) {
		if (!z.alive) continue;
		drawZombieSprite(z, camPos, forward, right, up, halfW, halfH, W, H,
						 zBuf, rBuf, gBuf, bBuf);
	}
	
	// 动物
	for (auto& a : g_animals) {
		if (!a.alive) continue;
		drawAnimalSprite(a, camPos, forward, right, up, halfW, halfH, W, H,
						 zBuf, rBuf, gBuf, bBuf);
	}
	
	// 远程玩家
	if (g_isMultiplayer) {
		for (auto& rp : g_remotePlayers) {
			if (!rp.active) continue;
			drawRemotePlayer(rp, camPos, forward, right, up, halfW, halfH, W, H,
							 zBuf, rBuf, gBuf, bBuf);
		}
	}
	
	// ============================================================
	//  输出
	// ============================================================
	string out;
	out.reserve((size_t)W * H * 8 + H * 24 + 4096);
	
	static int lastW = -1, lastH = -1;
	if (visW != lastW || visH != lastH) {
		out += "\x1b[2J";
		lastW = visW;
		lastH = visH;
	}
	out += "\x1b[?7l";
	
	for (int y = 0; y < H; ++y) {
		appendGoto(out, y + 1, 1);
		int lr = -1, lg = -1, lb = -1;
		for (int x = 0; x < W; ++x) {
			int idx = y * W + x;
			int r = rBuf[idx], g = gBuf[idx], b = bBuf[idx];
			if (r != lr || g != lg || b != lb) {
				out += "\x1b[48;2;";
				out += to_string(r); out += ';';
				out += to_string(g); out += ';';
				out += to_string(b); out += 'm';
				lr = r; lg = g; lb = b;
			}
			if (chBuf[idx]) out += "\x1b[97m+\x1b[39m";
			else            out += ' ';
		}
		out += "\x1b[0m\x1b[K";
	}
	for (int y = H; y < visH; ++y) {
		appendGoto(out, y + 1, 1);
		out += "\x1b[0m\x1b[K";
	}
	
	int healthRow = visH - 1;
	int hotbarRow = visH;
	
	appendGoto(out, healthRow, 1);
	out += "\x1b[0m\x1b[K  ";
	out += "\x1b[97m生命\x1b[39m [";
	for (int i = 0; i < MAX_HEALTH / 2; ++i) {
		int hp = g_health - i * 2;
		if (hp >= 2) {
			if (g_healthAnimTimer > 0 && (int)(g_healthAnimTimer * 12) % 2 == 0)
				out += "\x1b[97m#\x1b[39m";
			else
				out += "\x1b[91m#\x1b[39m";
		} else if (hp == 1) {
			out += "\x1b[91m+\x1b[39m";
		} else {
			out += "\x1b[90m-\x1b[39m";
		}
	}
	out += "] ";
	{ char buf[16]; sprintf(buf, "%2d/%d", g_health, MAX_HEALTH); out += buf; }
	
	out += "  \x1b[97m饱食\x1b[39m [";
	for (int i = 0; i < MAX_FOOD / 2; ++i) {
		int fd = g_food - i * 2;
		if (fd >= 2) out += "\x1b[93m#\x1b[39m";
		else if (fd == 1) out += "\x1b[93m+\x1b[39m";
		else out += "\x1b[90m-\x1b[39m";
	}
	out += "] ";
	{ char buf[16]; sprintf(buf, "%2d/%d", g_food, MAX_FOOD); out += buf; }
	
	{
		float tod = g_timeOfDay;
		float hours24 = fmodf(tod * 24.0f + 6.0f, 24.0f);
		int hh = (int)hours24;
		int mm = (int)((hours24 - hh) * 60);
		char buf[32];
		sprintf(buf, "  时间 %02d:%02d ", hh, mm);
		out += buf;
		if (g_globalLight > 0.7f) out += "\x1b[93m[日]\x1b[39m";
		else                      out += "\x1b[94m[夜]\x1b[39m";
	}
	{
		out += "  \x1b[97m手持:\x1b[39m ";
		ItemStack& sel = inventory[hotbarSelected];
		if (sel.empty()) out += "(空手)";
		else {
			out += blockNamesCN[sel.type];
			if (sel.count > 1) {
				out += " x";
				out += to_string(sel.count);
			}
		}
	}
	if (g_isMultiplayer) {
		char buf[32];
		sprintf(buf, "  \x1b[95m[联机 %dP]\x1b[39m", (int)g_remotePlayers.size());
		out += buf;
	}
	
	appendGoto(out, hotbarRow, 1);
	out += "\x1b[0m\x1b[K  ";
	for (int i = 0; i < 9; ++i) {
		out += itemSlotText(i, i == hotbarSelected);
		out += ' ';
	}
	out += "\x1b[K";
	
	{
		int type = inventory[hotbarSelected].type;
		RGB c;
		if (type == 0) c = {210, 170, 140};
		else           c = getItemColor(type);
		
		int swingPixels = (int)(g_handSwing * 3.0f);
		if (swingPixels < 0) swingPixels = 0;
		
		int startY = visH - 10 + swingPixels;
		int startX = visW - 8;
		if (startY < 1) startY = 1;
		if (startX < 1) startX = 1;
		if (startY + 6 > visH) startY = visH - 6;
		if (startX + 6 > visW) startX = visW - 6;
		
		uint8_t dr = (uint8_t)(c.r * 0.55f);
		uint8_t dg = (uint8_t)(c.g * 0.55f);
		uint8_t db = (uint8_t)(c.b * 0.55f);
		char buf2[64];
		for (int yy = 0; yy < 6; ++yy) {
			appendGoto(out, startY + yy, startX);
			for (int xx = 0; xx < 6; ++xx) {
				bool edge = (yy == 0 || yy == 5 || xx == 0 || xx == 5);
				if (edge) sprintf(buf2, "\x1b[48;2;%d;%d;%dm ", dr, dg, db);
				else      sprintf(buf2, "\x1b[48;2;%d;%d;%dm ", c.r, c.g, c.b);
				out += buf2;
			}
			out += "\x1b[0m";
		}
	}
	
	out += "\x1b[?7h";
	fwrite(out.data(), 1, out.size(), stdout);
	fflush(stdout);
}

void renderUI() { }
void renderHand(int type) { (void)type; }

// ============================================================
//  命中检测
// ============================================================
int hitBagSlot(int mx, int my) {
	for (int r = 0; r < 3; ++r) {
		if (my != 4 + r) continue;
		for (int c = 0; c < 9; ++c) {
			int x = 2 + c*10;
			if (mx >= x && mx < x + 9) return 9 + r*9 + c;
		}
	}
	if (my == 13) {
		for (int c = 0; c < 9; ++c) {
			int x = 2 + c*10;
			if (mx >= x && mx < x + 9) return c;
		}
	}
	if (my == 9) {
		if (mx >= 2  && mx < 11) return 36;
		if (mx >= 12 && mx < 21) return 37;
		if (mx >= 32 && mx < 41) return 40;
	}
	if (my == 10) {
		if (mx >= 2  && mx < 11) return 38;
		if (mx >= 12 && mx < 21) return 39;
	}
	return -1;
}
int hitTableSlot(int mx, int my) {
	for (int r = 0; r < 3; ++r) {
		if (my != 4 + r) continue;
		for (int c = 0; c < 3; ++c) {
			int x = 2 + c*10;
			if (mx >= x && mx < x + 9) return 41 + r*3 + c;
		}
	}
	if (my == 4 && mx >= 44 && mx < 53) return 50;
	for (int r = 0; r < 3; ++r) {
		if (my != 9 + r) continue;
		for (int c = 0; c < 9; ++c) {
			int x = 2 + c*10;
			if (mx >= x && mx < x + 9) return 9 + r*9 + c;
		}
	}
	if (my == 14) {
		for (int c = 0; c < 9; ++c) {
			int x = 2 + c*10;
			if (mx >= x && mx < x + 9) return c;
		}
	}
	return -1;
}
int hitChestSlot(int mx, int my) {
	for (int r = 0; r < 3; ++r) {
		if (my != 4 + r) continue;
		for (int c = 0; c < 9; ++c) {
			int x = 2 + c*10;
			if (mx >= x && mx < x + 9) return -1000 - (r*9 + c);
		}
	}
	for (int r = 0; r < 3; ++r) {
		if (my != 9 + r) continue;
		for (int c = 0; c < 9; ++c) {
			int x = 2 + c*10;
			if (mx >= x && mx < x + 9) return 9 + r*9 + c;
		}
	}
	if (my == 14) {
		for (int c = 0; c < 9; ++c) {
			int x = 2 + c*10;
			if (mx >= x && mx < x + 9) return c;
		}
	}
	return -1;
}
int hitFurnaceSlot(int mx, int my) {
	if (my == 4) {
		if (mx >= 2  && mx < 11) return -2001;
		if (mx >= 22 && mx < 31) return -2002;
	}
	if (my == 5) {
		if (mx >= 2  && mx < 11) return -2003;
	}
	for (int r = 0; r < 3; ++r) {
		if (my != 9 + r) continue;
		for (int c = 0; c < 9; ++c) {
			int x = 2 + c*10;
			if (mx >= x && mx < x + 9) return 9 + r*9 + c;
		}
	}
	if (my == 14) {
		for (int c = 0; c < 9; ++c) {
			int x = 2 + c*10;
			if (mx >= x && mx < x + 9) return c;
		}
	}
	return -1;
}

// ============================================================
//  点击逻辑
// ============================================================
void clickSlot(int idx) {
	if (idx < 0 || idx >= INV_SIZE) return;
	ItemStack& s = inventory[idx];
	if (cursorItem.empty()) { if (!s.empty()) { cursorItem = s; s.clear(); } }
	else {
		if (s.empty()) { s = cursorItem; cursorItem.clear(); }
		else if (s.type == cursorItem.type && !isTool(s.type) && !isSword(s.type)) {
			int space = 64 - s.count;
			int mv = space < cursorItem.count ? space : cursorItem.count;
			s.count += mv; cursorItem.count -= mv;
			if (cursorItem.count <= 0) cursorItem.clear();
		} else { ItemStack tmp = s; s = cursorItem; cursorItem = tmp; }
	}
}
void clickSlotRight(int idx) {
	if (idx < 0 || idx >= INV_SIZE) return;
	ItemStack& s = inventory[idx];
	if (cursorItem.empty()) {
		if (!s.empty()) {
			if (isTool(s.type) || isSword(s.type)) { cursorItem = s; s.clear(); }
			else {
				int half = (s.count + 1) / 2;
				cursorItem.type = s.type; cursorItem.count = half; cursorItem.durability = s.durability;
				s.count -= half; if (s.count <= 0) s.clear();
			}
		}
	} else {
		if (s.empty()) {
			s.type = cursorItem.type; s.count = 1; s.durability = cursorItem.durability;
			cursorItem.count--; if (cursorItem.count <= 0) cursorItem.clear();
		} else if (s.type == cursorItem.type && !isTool(s.type) && !isSword(s.type)) {
			if (s.count < 64) { s.count++; cursorItem.count--; if (cursorItem.count <= 0) cursorItem.clear(); }
		}
	}
}
void clickChestSlot(int idx) {
	ItemStack* s = nullptr;
	if (idx <= -1000) { int i = -1000 - idx; if (i < 0 || i >= (int)g_openChestData.slots.size()) return; s = &g_openChestData.slots[i]; }
	else if (idx >= 0 && idx < INV_SIZE) s = &inventory[idx];
	else return;
	if (cursorItem.empty()) { if (!s->empty()) { cursorItem = *s; s->clear(); } }
	else {
		if (s->empty()) { *s = cursorItem; cursorItem.clear(); }
		else if (s->type == cursorItem.type && !isTool(s->type) && !isSword(s->type)) {
			int space = 64 - s->count;
			int mv = space < cursorItem.count ? space : cursorItem.count;
			s->count += mv; cursorItem.count -= mv;
			if (cursorItem.count <= 0) cursorItem.clear();
		} else { ItemStack tmp = *s; *s = cursorItem; cursorItem = tmp; }
	}
}
void clickChestSlotRight(int idx) {
	ItemStack* s = nullptr;
	if (idx <= -1000) { int i = -1000 - idx; if (i < 0 || i >= (int)g_openChestData.slots.size()) return; s = &g_openChestData.slots[i]; }
	else if (idx >= 0 && idx < INV_SIZE) s = &inventory[idx];
	else return;
	if (cursorItem.empty()) {
		if (!s->empty()) {
			if (isTool(s->type) || isSword(s->type)) { cursorItem = *s; s->clear(); }
			else {
				int half = (s->count + 1) / 2;
				cursorItem.type = s->type; cursorItem.count = half; cursorItem.durability = s->durability;
				s->count -= half; if (s->count <= 0) s->clear();
			}
		}
	} else {
		if (s->empty()) { s->type = cursorItem.type; s->count = 1; s->durability = cursorItem.durability; cursorItem.count--; if (cursorItem.count <= 0) cursorItem.clear(); }
		else if (s->type == cursorItem.type && !isTool(s->type) && !isSword(s->type)) {
			if (s->count < 64) { s->count++; cursorItem.count--; if (cursorItem.count <= 0) cursorItem.clear(); }
		}
	}
}
void clickFurnaceSlot(int idx) {
	ItemStack* s = nullptr;
	if (idx == -2001) s = &g_openFurnaceData.input;
	else if (idx == -2002) s = &g_openFurnaceData.output;
	else if (idx == -2003) s = &g_openFurnaceData.fuel;
	else if (idx >= 0 && idx < INV_SIZE) s = &inventory[idx];
	else return;
	if (cursorItem.empty()) { if (!s->empty()) { cursorItem = *s; s->clear(); } }
	else {
		if (s->empty()) { *s = cursorItem; cursorItem.clear(); }
		else if (s->type == cursorItem.type && !isTool(s->type) && !isSword(s->type)) {
			int space = 64 - s->count;
			int mv = space < cursorItem.count ? space : cursorItem.count;
			s->count += mv; cursorItem.count -= mv;
			if (cursorItem.count <= 0) cursorItem.clear();
		} else { ItemStack tmp = *s; *s = cursorItem; cursorItem = tmp; }
	}
}
void clickFurnaceSlotRight(int idx) {
	ItemStack* s = nullptr;
	if (idx == -2001) s = &g_openFurnaceData.input;
	else if (idx == -2002) s = &g_openFurnaceData.output;
	else if (idx == -2003) s = &g_openFurnaceData.fuel;
	else if (idx >= 0 && idx < INV_SIZE) s = &inventory[idx];
	else return;
	if (cursorItem.empty()) {
		if (!s->empty()) {
			if (isTool(s->type) || isSword(s->type)) { cursorItem = *s; s->clear(); }
			else {
				int half = (s->count + 1) / 2;
				cursorItem.type = s->type; cursorItem.count = half; cursorItem.durability = s->durability;
				s->count -= half; if (s->count <= 0) s->clear();
			}
		}
	} else {
		if (s->empty()) { s->type = cursorItem.type; s->count = 1; s->durability = cursorItem.durability; cursorItem.count--; if (cursorItem.count <= 0) cursorItem.clear(); }
		else if (s->type == cursorItem.type && !isTool(s->type) && !isSword(s->type)) {
			if (s->count < 64) { s->count++; cursorItem.count--; if (cursorItem.count <= 0) cursorItem.clear(); }
		}
	}
}

void updateFurnace(FurnaceData& fd, float dt) {
	if (fd.fuelRemain > 0) fd.fuelRemain -= dt;
	int smeltResult = getSmeltResult(fd.input.type);
	bool hasRecipe = (smeltResult > 0 && fd.input.count > 0);
	bool canOutput = (fd.output.empty() ||
					  (fd.output.type == smeltResult && fd.output.count < 64));
	if (hasRecipe && canOutput) {
		if (fd.fuelRemain <= 0) {
			if (isFuel(fd.fuel.type) && fd.fuel.count > 0) {
				fd.fuel.count--;
				if (fd.fuel.count <= 0) fd.fuel.clear();
				fd.fuelRemain = 10.0f;
			}
		}
		if (fd.fuelRemain > 0) {
			fd.cookProgress += dt / 2.0f;
			if (fd.cookProgress >= 1.0f) {
				fd.cookProgress = 0;
				fd.input.count--;
				if (fd.input.count <= 0) fd.input.clear();
				if (fd.output.empty()) { fd.output.type = smeltResult; fd.output.count = 1; }
				else fd.output.count++;
			}
		}
	} else fd.cookProgress = 0;
}

// ============================================================
//  界面
// ============================================================
static void beginScreen(string& out) {
	out += "\x1b[2J"; out += "\x1b[?7l";
	appendGoto(out, 1, 1);
}
static void endScreen(string& out) { out += "\x1b[?7h"; }

void renderBagScreen() {
	string out; beginScreen(out);
	auto line = [&out](const string& s) { out += s; out += "\x1b[K\n"; };
	line(""); line("  === 背包 (E 关闭) ==="); line("");
	line("  主背包:");
	for (int r = 0; r < 3; ++r) {
		string s = "  ";
		for (int c = 0; c < 9; ++c) { s += itemSlotText(9+r*9+c, false); s += ' '; }
		line(s);
	}
	line(""); line("  合成 (2x2):");
	line("  " + itemSlotText(36,false) + ' ' + itemSlotText(37,false)
		 + "     ->      " + itemSlotText(40,false));
	line("  " + itemSlotText(38,false) + ' ' + itemSlotText(39,false));
	line(""); line("  快捷栏:");
	{ string s = "  ";
		for (int c = 0; c < 9; ++c) { s += itemSlotText(c, false); s += ' '; }
		line(s); }
	line("");
	{ string s = "  光标物品: ";
		if (cursorItem.empty()) s += "(空)";
		else s += string(blockNamesCN[cursorItem.type]) + " x" + to_string(cursorItem.count);
		line(s); }
	line(""); line("  左键=整堆/交换   右键=拿一半/放一个");
	endScreen(out);
	fwrite(out.data(), 1, out.size(), stdout); fflush(stdout);
}

void renderTableScreen() {
	string out; beginScreen(out);
	auto line = [&out](const string& s) { out += s; out += "\x1b[K\n"; };
	line(""); line("  === 工作台 3x3 (E 关闭) ==="); line("");
	line("  合成:");
	line("  " + itemSlotText(41,false) + ' ' + itemSlotText(42,false) + ' ' + itemSlotText(43,false)
		 + "         ->          " + itemSlotText(50,false));
	line("  " + itemSlotText(44,false) + ' ' + itemSlotText(45,false) + ' ' + itemSlotText(46,false));
	line("  " + itemSlotText(47,false) + ' ' + itemSlotText(48,false) + ' ' + itemSlotText(49,false));
	line(""); line("  主背包:");
	for (int r = 0; r < 3; ++r) {
		string s = "  ";
		for (int c = 0; c < 9; ++c) { s += itemSlotText(9+r*9+c, false); s += ' '; }
		line(s);
	}
	line(""); line("  快捷栏:");
	{ string s = "  ";
		for (int c = 0; c < 9; ++c) { s += itemSlotText(c, false); s += ' '; }
		line(s); }
	line(""); line("  === 配方 ===");
	line("  木棍 : 2 木板竖排");
	line("  熔炉 : 8 石头围成圈");
	line("  床   : 3 羊毛横排 + 3 木板横排");
	line("  工具/剑/箱子/工作台（见之前配方）");
	{ string s = "  光标物品: ";
		if (cursorItem.empty()) s += "(空)";
		else s += string(blockNamesCN[cursorItem.type]) + " x" + to_string(cursorItem.count);
		line(s); }
	endScreen(out);
	fwrite(out.data(), 1, out.size(), stdout); fflush(stdout);
}

void renderChestScreen() {
	string out; beginScreen(out);
	auto line = [&out](const string& s) { out += s; out += "\x1b[K\n"; };
	line(""); line("  === 箱子 (E 关闭) ==="); line("");
	line("  箱子:");
	for (int r = 0; r < 3; ++r) {
		string s = "  ";
		for (int c = 0; c < 9; ++c) { s += slotTextFrom(g_openChestData.slots[r*9+c], false); s += ' '; }
		line(s);
	}
	line(""); line("  主背包:");
	for (int r = 0; r < 3; ++r) {
		string s = "  ";
		for (int c = 0; c < 9; ++c) { s += itemSlotText(9+r*9+c, false); s += ' '; }
		line(s);
	}
	line(""); line("  快捷栏:");
	{ string s = "  ";
		for (int c = 0; c < 9; ++c) { s += itemSlotText(c, false); s += ' '; }
		line(s); }
	line("");
	{ string s = "  光标物品: ";
		if (cursorItem.empty()) s += "(空)";
		else s += string(blockNamesCN[cursorItem.type]) + " x" + to_string(cursorItem.count);
		line(s); }
	endScreen(out);
	fwrite(out.data(), 1, out.size(), stdout); fflush(stdout);
}

void renderFurnaceScreen() {
	string out; beginScreen(out);
	auto line = [&out](const string& s) { out += s; out += "\x1b[K\n"; };
	line(""); line("  === 熔炉 (E 关闭) ==="); line("");
	line("  输入:");
	out += "  "; out += slotTextFrom(g_openFurnaceData.input, false); out += "\x1b[K\n";
	line("  燃料:");
	out += "  "; out += slotTextFrom(g_openFurnaceData.fuel, false); out += "\x1b[K\n";
	line("  输出:");
	out += "  "; out += slotTextFrom(g_openFurnaceData.output, false); out += "\x1b[K\n";
	line("");
	int prog = (int)(g_openFurnaceData.cookProgress * 20);
	if (prog < 0) prog = 0;
	if (prog > 20) prog = 20;
	string bar = "  烧炼: [";
	for (int i = 0; i < 20; ++i) bar += (i < prog ? '#' : '-');
	bar += "]";
	line(bar);
	line(""); line("  主背包:");
	for (int r = 0; r < 3; ++r) {
		string s = "  ";
		for (int c = 0; c < 9; ++c) { s += itemSlotText(9+r*9+c, false); s += ' '; }
		line(s);
	}
	line(""); line("  快捷栏:");
	{ string s = "  ";
		for (int c = 0; c < 9; ++c) { s += itemSlotText(c, false); s += ' '; }
		line(s); }
	line(""); line("  配方: 铁矿石→铁锭 / 生肉→熟肉");
	{ string s = "  光标物品: ";
		if (cursorItem.empty()) s += "(空)";
		else s += string(blockNamesCN[cursorItem.type]) + " x" + to_string(cursorItem.count);
		line(s); }
	endScreen(out);
	fwrite(out.data(), 1, out.size(), stdout); fflush(stdout);
}
