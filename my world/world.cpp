#include "common.h"

inline int floordiv16(int x) { int q = x/16; if (x<0 && x%16) q--; return q; }
inline int mod16(int x) { int r = x%16; if (r<0) r+=16; return r; }
inline int64_t chunkKey(int cx, int cz) { return ((int64_t)cx << 32) | (uint32_t)cz; }

static uint32_t hashU32(uint32_t x) {
	x = ((x>>16)^x) * 0x45d9f3b;
	x = ((x>>16)^x) * 0x45d9f3b;
	x = (x>>16)^x;
	return x;
}
static float hash2f(int x, int z, uint32_t seed) {
	uint32_t h = (uint32_t)x * 0x1f1f1f1fu ^ (uint32_t)z * 0x7f4a7c15u ^ seed;
	h = hashU32(h);
	return (h & 0xFFFFFF) / (float)0xFFFFFF;
}
static float smoothNoise2(float x, float z, uint32_t seed) {
	int xi = (int)floorf(x), zi = (int)floorf(z);
	float xf = x-xi, zf = z-zi;
	float u = xf*xf*(3-2*xf), v = zf*zf*(3-2*zf);
	float a = hash2f(xi,zi,seed), b = hash2f(xi+1,zi,seed);
	float c = hash2f(xi,zi+1,seed), d = hash2f(xi+1,zi+1,seed);
	return (a*(1-u)+b*u)*(1-v) + (c*(1-u)+d*u)*v;
}
static float terrainHeight(int wx, int wz, uint32_t seed) {
	float h = 20.0f;
	h += smoothNoise2(wx*0.02f, wz*0.02f, seed)     * 14.0f;
	h += smoothNoise2(wx*0.08f, wz*0.08f, seed + 1) *  4.0f;
	h += smoothNoise2(wx*0.22f, wz*0.22f, seed + 2) *  1.5f;
	return h;
}

// 3D 噪声（矿洞用）
static float smoothNoise3(float x, float y, float z, uint32_t seed) {
	int xi = (int)floorf(x), yi = (int)floorf(y), zi = (int)floorf(z);
	float xf = x-xi, yf = y-yi, zf = z-zi;
	float u = xf*xf*(3-2*xf), v = yf*yf*(3-2*yf), w = zf*zf*(3-2*zf);
	auto h = [&](int a,int b,int c){
		uint32_t hh = (uint32_t)a*73856093u ^ (uint32_t)b*19349663u ^ (uint32_t)c*83492791u ^ seed;
		hh = ((hh>>16)^hh) * 0x45d9f3b; hh = ((hh>>16)^hh) * 0x45d9f3b; hh = (hh>>16)^hh;
		return (hh & 0xFFFFFF) / (float)0xFFFFFF;
	};
	float c000=h(xi,yi,zi),     c100=h(xi+1,yi,zi);
	float c010=h(xi,yi+1,zi),   c110=h(xi+1,yi+1,zi);
	float c001=h(xi,yi,zi+1),   c101=h(xi+1,yi,zi+1);
	float c011=h(xi,yi+1,zi+1), c111=h(xi+1,yi+1,zi+1);
	float x00 = c000*(1-u)+c100*u;
	float x10 = c010*(1-u)+c110*u;
	float x01 = c001*(1-u)+c101*u;
	float x11 = c011*(1-u)+c111*u;
	float y0 = x00*(1-v)+x10*v;
	float y1 = x01*(1-v)+x11*v;
	return y0*(1-w)+y1*w;
}

void generateChunk(Chunk* c, int cx, int cz) {
	for (int lx = 0; lx < CHUNK_SX; ++lx)
		for (int lz = 0; lz < CHUNK_SZ; ++lz) {
			int wx = cx*CHUNK_SX+lx, wz = cz*CHUNK_SZ+lz;
			int h = (int)terrainHeight(wx, wz, g_seed);
			if (h < 1) h = 1;
			if (h > CHUNK_SY-12) h = CHUNK_SY-12;
			
			// 地表
			for (int y = 0; y < h; ++y) {
				if      (y == 0)   c->blocks[lx][y][lz] = 37;   // 基岩
				else if (y == h-1) c->blocks[lx][y][lz] = 1;
				else if (y > h-4)  c->blocks[lx][y][lz] = 2;
				else               c->blocks[lx][y][lz] = 3;
			}
			
			// 矿洞
			for (int y = 3; y < h - 3; ++y) {
				if (c->blocks[lx][y][lz] != 3) continue;
				float n = smoothNoise3(wx*0.08f, y*0.14f, wz*0.08f, g_seed + 900);
				if (n > 0.70f) {
					c->blocks[lx][y][lz] = 0;
				}
			}
			
			// 矿石
			for (int y = 2; y < h - 3; ++y) {
				if (c->blocks[lx][y][lz] != 3) continue;
				float r = hash2f(wx * 7 + y * 13, wz * 11 + y * 17, g_seed + 500);
				if (y >= 3 && y <= 45 && r < 0.008f) {
					c->blocks[lx][y][lz] = 19;
				} else if (y >= 2 && y <= 28 && r > 0.997f) {
					c->blocks[lx][y][lz] = 18;
				}
			}
			
			// 树
			if (hash2f(wx, wz, g_seed+100) < 0.015f && h+7 < CHUNK_SY) {
				for (int t = 0; t < 4; ++t) c->blocks[lx][h+t][lz] = 4;
				int topY = h+4;
				for (int dx = -2; dx <= 2; ++dx)
					for (int dz = -2; dz <= 2; ++dz)
						for (int dy = -1; dy <= 2; ++dy) {
							int nx = lx+dx, nz = lz+dz, ny = topY+dy;
							if (nx<0||nx>=CHUNK_SX||nz<0||nz>=CHUNK_SZ||ny<0||ny>=CHUNK_SY) continue;
							if (abs(dx)+abs(dz)+abs(dy) <= 3 && c->blocks[nx][ny][nz] == 0)
								c->blocks[nx][ny][nz] = 5;
						}
			}
		}
	c->generated = true;
}

Chunk* getChunk(int cx, int cz) {
	int64_t k = chunkKey(cx, cz);
	auto it = g_chunks.find(k);
	if (it != g_chunks.end()) return it->second;
	Chunk* c = new Chunk();
	memset(c->blocks, 0, sizeof(c->blocks));
	c->generated = false;
	g_chunks[k] = c;
	return c;
}
uint8_t getBlock(int x, int y, int z) {
	if (y < 0 || y >= CHUNK_SY) return 0;
	int cx = floordiv16(x), cz = floordiv16(z);
	Chunk* c = getChunk(cx, cz);
	if (!c->generated) generateChunk(c, cx, cz);
	return c->blocks[mod16(x)][y][mod16(z)];
}
void setBlock(int x, int y, int z, uint8_t v) {
	if (y < 0 || y >= CHUNK_SY) return;
	int cx = floordiv16(x), cz = floordiv16(z);
	Chunk* c = getChunk(cx, cz);
	if (!c->generated) generateChunk(c, cx, cz);
	c->blocks[mod16(x)][y][mod16(z)] = v;
}
void modifyBlock(int x, int y, int z, uint8_t v) {
	setBlock(x, y, z, v);
	g_changes.push_back({x, y, z, v});
}
void clearChunks() {
	for (auto& kv : g_chunks) delete kv.second;
	g_chunks.clear();
}

RayHit raycastDDA(const Vec3& origin, const Vec3& dir, float maxDist) {
	RayHit r; r.hit = false; r.t = maxDist; r.blockType = 0;
	int bx=(int)floorf(origin.x), by=(int)floorf(origin.y), bz=(int)floorf(origin.z);
	int sx=(dir.x>0)-(dir.x<0), sy=(dir.y>0)-(dir.y<0), sz=(dir.z>0)-(dir.z<0);
	float tMaxX = (sx==0)?1e30f:((sx>0?(bx+1-origin.x):(origin.x-bx))/fabsf(dir.x));
	float tMaxY = (sy==0)?1e30f:((sy>0?(by+1-origin.y):(origin.y-by))/fabsf(dir.y));
	float tMaxZ = (sz==0)?1e30f:((sz>0?(bz+1-origin.z):(origin.z-bz))/fabsf(dir.z));
	float tDX = (sx==0)?1e30f:1.0f/fabsf(dir.x);
	float tDY = (sy==0)?1e30f:1.0f/fabsf(dir.y);
	float tDZ = (sz==0)?1e30f:1.0f/fabsf(dir.z);
	float t=0; int face=0, safety=0;
	int ccx=0x7FFFFFFF, ccz=0x7FFFFFFF;
	Chunk* cur = nullptr;
	while (t < maxDist && safety++ < 512) {
		int cx = floordiv16(bx), cz = floordiv16(bz);
		if (cx != ccx || cz != ccz) {
			cur = getChunk(cx, cz);
			if (!cur->generated) generateChunk(cur, cx, cz);
			ccx = cx; ccz = cz;
		}
		if (by >= 0 && by < CHUNK_SY) {
			uint8_t b = cur->blocks[mod16(bx)][by][mod16(bz)];
			if (b != 0) {
				r.hit = true; r.t = t;
				r.bx = bx; r.by = by; r.bz = bz;
				r.face = face; r.blockType = b;
				return r;
			}
		}
		if (tMaxX < tMaxY && tMaxX < tMaxZ) {
			t = tMaxX; tMaxX += tDX; bx += sx;
			face = (sx > 0) ? 1 : 0;
		} else if (tMaxY < tMaxZ) {
			t = tMaxY; tMaxY += tDY; by += sy;
			face = (sy > 0) ? 3 : 2;
		} else {
			t = tMaxZ; tMaxZ += tDZ; bz += sz;
			face = (sz > 0) ? 5 : 4;
		}
	}
	return r;
}

const float ITEM_SIZE = 0.25f;
const float ITEM_GRAVITY = -0.022f;
const float PICKUP_RANGE = 1.5f;
const float PICKUP_DELAY = 0.40f;
const float ITEM_LIFETIME = 120.0f;

void spawnItemDrop(int x, int y, int z, int type, int count) {
	ItemEntity e;
	e.pos = {x+0.5f, y+0.5f, z+0.5f};
	float rx = ((rand()%100)/100.0f - 0.5f) * 0.10f;
	float rz = ((rand()%100)/100.0f - 0.5f) * 0.10f;
	e.vel = {rx, 0.18f, rz};
	e.type = type; e.count = count;
	e.spin = 0; e.life = 0;
	e.onGround = false; e.picked = false;
	g_items.push_back(e);
}
static bool itemCollidesAt(const Vec3& p) {
	float x0=p.x-ITEM_SIZE, x1=p.x+ITEM_SIZE;
	float y0=p.y-ITEM_SIZE, y1=p.y+ITEM_SIZE;
	float z0=p.z-ITEM_SIZE, z1=p.z+ITEM_SIZE;
	int ix0=(int)floorf(x0), ix1=(int)floorf(x1-1e-4f);
	int iy0=(int)floorf(y0), iy1=(int)floorf(y1-1e-4f);
	int iz0=(int)floorf(z0), iz1=(int)floorf(z1-1e-4f);
	for (int x=ix0;x<=ix1;++x)
		for (int y=iy0;y<=iy1;++y)
			for (int z=iz0;z<=iz1;++z)
				if (getBlock(x,y,z) != 0) return true;
	return false;
}
static float itemSweepAxis(Vec3& p, int axis, float delta) {
	if (fabsf(delta) < 1e-6f) return 0;
	Vec3 t = p;
	if      (axis==0) t.x += delta;
	else if (axis==1) t.y += delta;
	else              t.z += delta;
	if (!itemCollidesAt(t)) { p = t; return delta; }
	float lo=0, hi=delta;
	for (int i = 0; i < 10; ++i) {
		float mid = (lo+hi)*0.5f;
		Vec3 tt = p;
		if      (axis==0) tt.x += mid;
		else if (axis==1) tt.y += mid;
		else              tt.z += mid;
		if (itemCollidesAt(tt)) hi = mid; else lo = mid;
	}
	Vec3 tt = p;
	if      (axis==0) tt.x += lo;
	else if (axis==1) tt.y += lo;
	else              tt.z += lo;
	p = tt;
	return lo;
}
void updateItems(const Vec3& playerEye, float dt) {
	const float DRAG = 0.92f;
	for (auto& e : g_items) {
		if (e.picked) continue;
		e.life += dt;
		if (e.life > ITEM_LIFETIME) { e.picked = true; continue; }
		e.vel.y += ITEM_GRAVITY;
		if (e.vel.y < -1.0f) e.vel.y = -1.0f;
		float mx=e.vel.x, my=e.vel.y, mz=e.vel.z;
		float gx = itemSweepAxis(e.pos, 0, mx);
		float gz = itemSweepAxis(e.pos, 2, mz);
		float gy = itemSweepAxis(e.pos, 1, my);
		if (fabsf(gx) < fabsf(mx)-1e-4f) e.vel.x = 0;
		if (fabsf(gz) < fabsf(mz)-1e-4f) e.vel.z = 0;
		if (e.vel.y <= 0 && fabsf(gy) < fabsf(my)-1e-4f) {
			e.onGround = true; e.vel.y = 0;
			e.vel.x *= DRAG; e.vel.z *= DRAG;
		} else e.onGround = false;
		e.spin += dt * 4.0f;
		if (e.life > PICKUP_DELAY) {
			float footY = playerEye.y - 1.62f;
			float topY = footY + 1.80f;
			bool inV = (e.pos.y + 0.2f >= footY && e.pos.y - 0.2f <= topY);
			float ddx = e.pos.x - playerEye.x;
			float ddz = e.pos.z - playerEye.z;
			float horiz = sqrtf(ddx*ddx + ddz*ddz);
			if (inV && horiz < PICKUP_RANGE) { addItem(e.type, e.count); e.picked = true; }
		}
	}
	vector<ItemEntity> keep;
	for (auto& e : g_items) if (!e.picked) keep.push_back(e);
	g_items.swap(keep);
}

const float PLAYER_HALF_W = 0.30f;
const float PLAYER_HEIGHT = 1.80f;
const float PLAYER_EYE    = 1.62f;
const float STEP_HEIGHT   = 1.05f;

void playerBox(const Vec3& eye, float& x0, float& y0, float& z0,
			   float& x1, float& y1, float& z1) {
	x0 = eye.x - PLAYER_HALF_W; x1 = eye.x + PLAYER_HALF_W;
	z0 = eye.z - PLAYER_HALF_W; z1 = eye.z + PLAYER_HALF_W;
	y0 = eye.y - PLAYER_EYE;    y1 = y0 + PLAYER_HEIGHT;
}
bool playerCollides(const Vec3& eye) {
	float x0,y0,z0,x1,y1,z1; playerBox(eye, x0,y0,z0,x1,y1,z1);
	int ix0=(int)floorf(x0), ix1=(int)floorf(x1-1e-4f);
	int iy0=(int)floorf(y0), iy1=(int)floorf(y1-1e-4f);
	int iz0=(int)floorf(z0), iz1=(int)floorf(z1-1e-4f);
	for (int x=ix0;x<=ix1;++x)
		for (int y=iy0;y<=iy1;++y)
			for (int z=iz0;z<=iz1;++z)
				if (getBlock(x,y,z) != 0) return true;
	return false;
}
float sweepAxis(Vec3& eye, int axis, float delta) {
	if (fabsf(delta) < 1e-6f) return 0;
	Vec3 test = eye;
	if      (axis==0) test.x += delta;
	else if (axis==1) test.y += delta;
	else              test.z += delta;
	if (!playerCollides(test)) { eye = test; return delta; }
	float lo=0, hi=delta;
	for (int i = 0; i < 16; ++i) {
		float mid = (lo+hi)*0.5f;
		Vec3 t = eye;
		if      (axis==0) t.x += mid;
		else if (axis==1) t.y += mid;
		else              t.z += mid;
		if (playerCollides(t)) hi = mid; else lo = mid;
	}
	Vec3 t = eye;
	if      (axis==0) t.x += lo;
	else if (axis==1) t.y += lo;
	else              t.z += lo;
	eye = t;
	return lo;
}
bool tryStepUp(Vec3& eye, float dx, float dz) {
	if (fabsf(dx) < 1e-6f && fabsf(dz) < 1e-6f) return false;
	if (!playerCollides({eye.x+dx, eye.y, eye.z+dz})) return false;
	Vec3 lifted = eye; lifted.y += STEP_HEIGHT;
	if (playerCollides(lifted)) return false;
	Vec3 moved = lifted; moved.x += dx; moved.z += dz;
	if (playerCollides(moved)) {
		Vec3 mx = {lifted.x+dx, lifted.y, lifted.z};
		Vec3 mz = {lifted.x, lifted.y, lifted.z+dz};
		bool okX = !playerCollides(mx), okZ = !playerCollides(mz);
		if (!okX && !okZ) return false;
		if (okX && !okZ) moved = mx;
		else if (okZ && !okX) moved = mz;
		else {
			Vec3 both = {lifted.x+dx, lifted.y, lifted.z+dz};
			moved = (!playerCollides(both)) ? both : mx;
		}
	}
	for (float drop = 0.05f; drop <= STEP_HEIGHT+0.1f; drop += 0.05f) {
		Vec3 test = {moved.x, moved.y-drop, moved.z};
		if (playerCollides(test)) {
			Vec3 good = {moved.x, moved.y-(drop-0.05f), moved.z};
			eye = good;
			return true;
		}
	}
	return false;
}
