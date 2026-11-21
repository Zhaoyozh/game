#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#pragma comment(lib,"ws2_32.lib")

#define PORT    9001
#define MAXU    500
#define MAXR    32
#define MAXP    10
#define BUFSZ   4096

typedef struct { char name[32], pass[32], last[16]; int cards, sock; } User;

typedef struct {
	int uid, sock;
	int role, alive;
	int gold, bet, guess;
	int score, shot;
	int acted;
	int cx, cy;
} RP;

typedef struct {
	int id, mode, host, state, n, over;
	RP  p[MAXP];
	int round;
	int wolfTgt, seerTgt, witchSave, witchPoison, guardTgt, knightTgt;
	int votes[MAXP];
	int pool;
	int tx[10], ty[10], tc;
} Room;

User users[MAXU]; int nu = 0;
Room rooms[MAXR]; int nr = 0;
CRITICAL_SECTION cs;
int nextRid = 1;

/* ==================== 用户文件 ==================== */
void getToday(char *s) {
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	sprintf(s, "%04d-%02d-%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
}

int onlineCount() {
	int c = 0;
	for (int i = 0; i < nu; i++) if (users[i].sock >= 0) c++;
	return c;
}
int roomOnlineCount(Room *r) {
	int c = 0;
	for (int i = 0; i < r->n; i++) if (r->p[i].sock >= 0) c++;
	return c;
}

/* 关键修复：逐行解析，不跨行，兼容缺 last 的旧格式 */
void loadUsers() {
	FILE *f = fopen("users.txt", "r");
	if (!f) {
		printf("[服务端] users.txt 不存在，将新建空用户表\n");
		return;
	}
	char line[256];
	int line_no = 0, skipped = 0;
	while (nu < MAXU && fgets(line, sizeof(line), f)) {
		line_no++;
		line[strcspn(line, "\r\n")] = 0;
		if (line[0] == '\0') continue;
		
		char name[32] = {0}, pass[32] = {0}, last[16] = {0};
		int cards = 0;
		int n = sscanf(line, "%31s %31s %d %15s", name, pass, &cards, last);
		
		if (n < 3) {
			printf("[服务端] users.txt 第 %d 行格式错误，已跳过: %s\n", line_no, line);
			skipped++;
			continue;
		}
		if (n < 4) last[0] = '\0';
		
		strcpy(users[nu].name, name);
		strcpy(users[nu].pass, pass);
		users[nu].cards = cards;
		strcpy(users[nu].last, last);
		users[nu].sock = -1;
		nu++;
	}
	fclose(f);
	printf("[服务端] 成功加载 %d 个账号（跳过 %d 行无效数据）\n", nu, skipped);
}

void saveUsers() {
	FILE *f = fopen("users.txt", "w");
	if (!f) return;
	for (int i = 0; i < nu; i++)
		fprintf(f, "%s %s %d %s\n",
				users[i].name, users[i].pass, users[i].cards, users[i].last);
	fclose(f);
}

int findUser(const char *n) {
	for (int i = 0; i < nu; i++) if (strcmp(users[i].name, n) == 0) return i;
	return -1;
}
int findInRoom(Room *r, int uid) {
	for (int i = 0; i < r->n; i++) if (r->p[i].uid == uid) return i;
	return -1;
}
int findInRoomByName(Room *r, const char *name) {
	for (int i = 0; i < r->n; i++)
		if (strcmp(users[r->p[i].uid].name, name) == 0) return i;
	return -1;
}
Room *findRoom(int id) {
	for (int i = 0; i < nr; i++) if (rooms[i].id == id) return &rooms[i];
	return NULL;
}

/* ==================== 网络辅助 ==================== */
void sendTo(int s, const char *m) { if (s >= 0) send(s, m, (int)strlen(m), 0); }
void sendRoom(Room *r, const char *m) {
	for (int i = 0; i < r->n; i++)
		if (r->p[i].sock >= 0) sendTo(r->p[i].sock, m);
}
void sendRoomInfo(Room *r) {
	const char *mn[] = {"", "狼人杀", "转盘", "射击"};
	char buf[1024];
	sprintf(buf, "ROOM|%d|%s|房主:%s|在线/总:%d/%d|状态:%s",
			r->id, mn[r->mode], users[r->host].name,
			roomOnlineCount(r), r->n, r->state ? "游戏中" : "等待");
	sendRoom(r, buf);
	char pl[1024] = "PLAYERS";
	for (int i = 0; i < r->n; i++) {
		char t[64];
		sprintf(t, "|%s%s",
				users[r->p[i].uid].name,
				r->p[i].sock < 0 ? "(离线)" : "");
		strcat(pl, t);
	}
	sendRoom(r, pl);
}
void broadcastRoomList(int s) {
	char buf[2048] = "ROOMLIST";
	const char *mn[] = {"", "狼人杀", "转盘", "射击"};
	for (int i = 0; i < nr; i++) {
		if (rooms[i].state) continue;
		char t[128];
		sprintf(t, "|%d,%s,%s,%d/%d",
				rooms[i].id, mn[rooms[i].mode],
				users[rooms[i].host].name,
				roomOnlineCount(&rooms[i]), rooms[i].n);
		strcat(buf, t);
	}
	sendTo(s, buf);
}

/* ==================== 销毁房间 ==================== */
void destroyRoom(Room *r, const char *reason) {
	char buf[256];
	sprintf(buf, "SYS|房间已解散：%s", reason);
	sendRoom(r, buf);
	int idx = (int)(r - rooms);
	if (idx < nr - 1) rooms[idx] = rooms[nr - 1];
	nr--;
}

/* ==================== 狼人杀 ==================== */
void wolfSync(Room *r) {
	const char *rn[] = {"", "狼人", "预言家", "女巫", "守卫", "骑士", "平民"};
	char buf[512];
	for (int i = 0; i < r->n; i++) {
		if (r->p[i].sock < 0) continue;
		sprintf(buf, "SYS|你是 %s，角色：%s，状态：%s",
				users[r->p[i].uid].name, rn[r->p[i].role],
				r->p[i].alive ? "存活" : "死亡");
		sendTo(r->p[i].sock, buf);
	}
}
void wolfNightStart(Room *r) {
	r->wolfTgt = -1; r->seerTgt = -1;
	r->witchSave = -1; r->witchPoison = -1;
	r->guardTgt = -1; r->knightTgt = -1;
	for (int i = 0; i < r->n; i++) { r->p[i].acted = 0; r->votes[i] = -1; }
}
void wolfInit(Room *r) {
	int wc = (r->n <= 5) ? 1 : 2;
	int idx[MAXP];
	for (int i = 0; i < r->n; i++) idx[i] = i;
	for (int i = r->n - 1; i > 0; i--) {
		int j = rand() % (i + 1);
		int t = idx[i]; idx[i] = idx[j]; idx[j] = t;
	}
	int k = 0;
	for (int i = 0; i < wc; i++) r->p[idx[k++]].role = 1;
	r->p[idx[k++]].role = 2;
	r->p[idx[k++]].role = 3;
	if (r->n > 5) { r->p[idx[k++]].role = 4; r->p[idx[k++]].role = 5; }
	while (k < r->n) r->p[idx[k++]].role = 6;
	for (int i = 0; i < r->n; i++) r->p[i].alive = 1;
	r->round = 1; r->state = 1; r->over = 0;
	wolfNightStart(r);
	sendRoom(r, "SYS|游戏开始！角色已私发。");
	wolfSync(r);
	sendRoom(r, "SYS|第 1 天夜晚，狼人/预言家/女巫/守卫/骑士请行动。");
}
void wolfCheckWin(Room *r) {
	int wolves = 0, gods = 0, civs = 0;
	for (int i = 0; i < r->n; i++) {
		if (!r->p[i].alive) continue;
		if (r->p[i].role == 1) wolves++;
		else if (r->p[i].role == 6) civs++;
		else gods++;
	}
	if (wolves == 0) {
		sendRoom(r, "SYS|========== 好人阵营胜利！ ==========");
		r->state = 2; r->over = 1; return;
	}
	if (civs == 0 || gods == 0) {
		sendRoom(r, "SYS|========== 狼人阵营胜利！ ==========");
		r->state = 2; r->over = 1; return;
	}
}
void wolfResolveNight(Room *r) {
	char b[512];
	int dead = -1;
	if (r->wolfTgt >= 0 && r->p[r->wolfTgt].alive) {
		int saved = 0;
		if (r->witchSave == r->wolfTgt) saved = 1;
		if (r->guardTgt == r->wolfTgt) saved = 1;
		if (!saved) dead = r->wolfTgt;
	}
	if (r->witchPoison >= 0 && r->p[r->witchPoison].alive) {
		r->p[r->witchPoison].alive = 0;
		sprintf(b, "SYS|女巫毒死了 %s", users[r->p[r->witchPoison].uid].name);
		sendRoom(r, b);
	}
	if (dead >= 0 && r->p[dead].alive) {
		r->p[dead].alive = 0;
		sprintf(b, "SYS|昨晚 %s 被狼人杀死", users[r->p[dead].uid].name);
		sendRoom(r, b);
	} else if (dead < 0) {
		sendRoom(r, "SYS|昨晚是平安夜");
	}
	if (r->knightTgt >= 0 && r->p[r->knightTgt].alive && r->p[r->knightTgt].role == 1) {
		r->p[r->knightTgt].alive = 0;
		sprintf(b, "SYS|骑士 %s 决斗杀死狼人", users[r->p[r->knightTgt].uid].name);
		sendRoom(r, b);
	}
	wolfCheckWin(r);
	if (r->over) return;
	r->round++; r->state = 1;
	for (int i = 0; i < r->n; i++) { r->p[i].acted = 0; r->votes[i] = -1; }
	sprintf(b, "SYS|第 %d 天白天，请投票放逐一名玩家。", r->round);
	sendRoom(r, b);
}
void wolfDayVote(Room *r, int voterIdx, int tgt) {
	if (voterIdx < 0 || tgt < 0 || tgt >= r->n) return;
	if (!r->p[voterIdx].alive) return;
	r->votes[voterIdx] = tgt;
	r->p[voterIdx].acted = 1;
	for (int i = 0; i < r->n; i++)
		if (r->p[i].alive && !r->p[i].acted) return;
	int cnt[MAXP] = {0};
	for (int i = 0; i < r->n; i++)
		if (r->votes[i] >= 0) cnt[r->votes[i]]++;
	int mx = 0, who = -1;
	for (int i = 0; i < r->n; i++)
		if (cnt[i] > mx) { mx = cnt[i]; who = i; }
	char b[256];
	if (who >= 0) {
		r->p[who].alive = 0;
		sprintf(b, "SYS|%s 被投票放逐", users[r->p[who].uid].name);
	} else {
		strcpy(b, "SYS|投票平局，无人被放逐");
	}
	sendRoom(r, b);
	wolfCheckWin(r);
	if (r->over) return;
	r->round++;
	wolfNightStart(r);
	sprintf(b, "SYS|第 %d 天夜晚，请行动。", r->round);
	sendRoom(r, b);
}

/* ==================== 转盘 ==================== */
void ttInit(Room *r) {
	for (int i = 0; i < r->n; i++) {
		r->p[i].gold = 100; r->p[i].bet = 0;
		r->p[i].guess = 0;  r->p[i].acted = 0;
	}
	r->pool = 0; r->round = 1; r->state = 1; r->over = 0;
	sendRoom(r, "SYS|========== 转盘游戏开始 ==========");
	char b[256];
	sprintf(b, "SYS|第 %d 轮：请猜数字(1-50)并下注。", r->round);
	sendRoom(r, b);
}
void ttCheck(Room *r) {
	for (int i = 0; i < r->n; i++)
		if (r->p[i].gold > 0 && !r->p[i].acted) return;
	int target = rand() % 50 + 1;
	char b[512];
	sprintf(b, "SYS|开奖：%d", target);
	sendRoom(r, b);
	for (int i = 0; i < r->n; i++) {
		if (r->p[i].guess == target) {
			r->p[i].gold += r->pool;
			sprintf(b, "SYS|%s 猜中，赢得奖池 %d 金币！",
					users[r->p[i].uid].name, r->pool);
			sendRoom(r, b);
		}
	}
	r->pool = 0;
	int alive = 0, last = -1;
	for (int i = 0; i < r->n; i++)
		if (r->p[i].gold > 0) { alive++; last = i; }
	if (alive <= 1) {
		sprintf(b, "SYS|========== %s 获得最终胜利！ ==========",
				alive ? users[r->p[last].uid].name : "无人");
		sendRoom(r, b);
		r->state = 2; r->over = 1; return;
	}
	r->round++; r->pool = 0;
	for (int i = 0; i < r->n; i++) {
		r->p[i].bet = 0; r->p[i].guess = 0; r->p[i].acted = 0;
	}
	sprintf(b, "SYS|第 %d 轮：请猜数字(1-50)并下注。", r->round);
	sendRoom(r, b);
}

/* ==================== 射击 ==================== */
void shSyncField(Room *r) {
	char buf[4096] = "FIELD";
	for (int k = 0; k < r->tc; k++)
		if (r->tx[k] >= 0) {
			char t[16]; sprintf(t, "|T,%d,%d", r->tx[k], r->ty[k]); strcat(buf, t);
		}
	for (int i = 0; i < r->n; i++) {
		char t[32]; sprintf(t, "|P,%d,%d,%d", r->p[i].uid, r->p[i].cx, r->p[i].cy);
		strcat(buf, t);
	}
	sendRoom(r, buf);
}
void shInit(Room *r) {
	for (int i = 0; i < r->n; i++) {
		r->p[i].score = 0; r->p[i].shot = 0;
		r->p[i].cx = -1;   r->p[i].cy = -1;
	}
	r->round = 1; r->state = 1; r->over = 0;
	for (int k = 0; k < 10; k++) { r->tx[k] = -1; r->ty[k] = -1; }
	r->tc = 0;
	for (int k = 0; k < 5; k++) {
		r->tx[k] = rand() % 20; r->ty[k] = rand() % 20; r->tc++;
	}
	sendRoom(r, "SYS|========== 射击游戏开始 ==========");
	shSyncField(r);
	sendRoom(r, "SYS|输入 SHOOT|x|y 射击，先到 100 分胜。");
}
void shCheck(Room *r) {
	for (int i = 0; i < r->n; i++)
		if (r->p[i].sock >= 0 && !r->p[i].shot) return;
	for (int i = 0; i < r->n; i++)
		if (r->p[i].score >= 100) {
			char b[512];
			sprintf(b, "SYS|========== %s 射击胜利！ ==========",
					users[r->p[i].uid].name);
			sendRoom(r, b);
			r->state = 2; r->over = 1; return;
		}
	r->round++;
	for (int k = 0; k < 10; k++) { r->tx[k] = -1; r->ty[k] = -1; }
	r->tc = 0;
	for (int k = 0; k < 5; k++) {
		r->tx[k] = rand() % 20; r->ty[k] = rand() % 20; r->tc++;
	}
	for (int i = 0; i < r->n; i++) r->p[i].shot = 0;
	sendRoom(r, "SYS|新一轮靶子刷新");
	shSyncField(r);
}

/* ==================== 开始游戏 ==================== */
void startGame(Room *r) {
	if (r->mode == 1) {
		if (r->n < 4 || r->n > 10) { sendRoom(r, "ERR|狼人杀需要 4-10 人"); return; }
		wolfInit(r);
	} else if (r->mode == 2) {
		if (r->n < 5 || r->n > 10) { sendRoom(r, "ERR|转盘需要 5-10 人"); return; }
		ttInit(r);
	} else if (r->mode == 3) {
		if (r->n < 2 || r->n > 10) { sendRoom(r, "ERR|射击需要 2-10 人"); return; }
		shInit(r);
	}
}

/* ==================== 命令处理 ==================== */
void handleCmd(int sock, char *line, int *uidOut) {
	char *cmd = strtok(line, "|");
	if (!cmd) return;
	int uid = *uidOut;
	User *me = (uid >= 0) ? &users[uid] : NULL;
	
	/* 注册 = 自动登录 */
	if (strcmp(cmd, "REG") == 0) {
		char *n = strtok(NULL, "|"), *p = strtok(NULL, "|");
		if (!n || !p) { sendTo(sock, "ERR|参数错误"); return; }
		if (findUser(n) >= 0) { sendTo(sock, "ERR|用户名已存在"); return; }
		if (nu >= MAXU) { sendTo(sock, "ERR|用户满了"); return; }
		
		strcpy(users[nu].name, n);
		strcpy(users[nu].pass, p);
		users[nu].cards = 10;
		getToday(users[nu].last);
		users[nu].sock = sock;
		*uidOut = nu;
		nu++;
		saveUsers();
		
		printf("[服务端] 新用户 %s 注册成功，账号总数=%d，在线=%d\n",
			   users[nu - 1].name, nu, onlineCount());
		
		char b[160];
		sprintf(b, "OK|注册成功，已自动登录|房卡:%d", users[nu - 1].cards);
		sendTo(sock, b);
		return;
	}
	
	/* 登录 */
	if (strcmp(cmd, "LOGIN") == 0) {
		char *n = strtok(NULL, "|"), *p = strtok(NULL, "|");
		int i = findUser(n);
		if (i < 0 || strcmp(users[i].pass, p) != 0) {
			sendTo(sock, "ERR|用户名或密码错误"); return;
		}
		if (users[i].sock >= 0) {
			sendTo(sock, "ERR|该账号已在别处登录"); return;
		}
		users[i].sock = sock;
		char td[16]; getToday(td);
		if (strcmp(users[i].last, td) != 0) {
			users[i].cards += 5;
			strcpy(users[i].last, td);
			saveUsers();
			sendTo(sock, "SYS|每日登录奖励：+5 房卡");
		}
		*uidOut = i;
		
		printf("[服务端] 用户 %s 登录，在线=%d/%d\n",
			   users[i].name, onlineCount(), nu);
		
		char b[128];
		sprintf(b, "OK|登录成功|房卡:%d", users[i].cards);
		sendTo(sock, b);
		return;
	}
	
	if (!me) { sendTo(sock, "ERR|请先登录"); return; }
	
	if (strcmp(cmd, "LOGOUT") == 0) {
		me->sock = -1;
		*uidOut = -1;
		printf("[服务端] 用户 %s 登出，在线=%d/%d\n",
			   me->name, onlineCount(), nu);
		sendTo(sock, "OK|已登出");
		return;
	}
	if (strcmp(cmd, "ROOMS") == 0) { broadcastRoomList(sock); return; }
	
	if (strcmp(cmd, "CREATE") == 0) {
		char *m = strtok(NULL, "|");
		int mode = atoi(m);
		if (mode < 1 || mode > 3) { sendTo(sock, "ERR|玩法错误"); return; }
		if (me->cards <= 0)       { sendTo(sock, "ERR|房卡不足"); return; }
		if (nr >= MAXR)           { sendTo(sock, "ERR|房间满了"); return; }
		
		Room *r = &rooms[nr++];
		memset(r, 0, sizeof(Room));
		r->id = nextRid++;
		r->mode = mode;
		r->host = uid;
		r->state = 0;
		r->n = 1;
		r->over = 0;
		r->p[0].uid = uid;
		r->p[0].sock = sock;
		r->p[0].cx = r->p[0].cy = -1;
		me->cards--;
		saveUsers();
		
		char b[256];
		sprintf(b, "OK|房间创建成功，ID=%d，剩余房卡=%d", r->id, me->cards);
		sendTo(sock, b);
		sendRoomInfo(r);
		return;
	}
	
	if (strcmp(cmd, "JOIN") == 0) {
		int rid = atoi(strtok(NULL, "|"));
		Room *r = findRoom(rid);
		if (!r)           { sendTo(sock, "ERR|房间不存在"); return; }
		if (r->state)     { sendTo(sock, "ERR|游戏已开始"); return; }
		if (r->n >= MAXP) { sendTo(sock, "ERR|房间已满"); return; }
		r->p[r->n].uid = uid;
		r->p[r->n].sock = sock;
		r->p[r->n].role = 0;
		r->p[r->n].gold = 100;
		r->p[r->n].score = 0;
		r->p[r->n].cx = r->p[r->n].cy = -1;
		r->n++;
		sendTo(sock, "OK|加入成功");
		char b[128];
		sprintf(b, "SYS|%s 加入了房间", me->name);
		sendRoom(r, b);
		sendRoomInfo(r);
		return;
	}
	
	Room *r = NULL;
	for (int i = 0; i < nr; i++)
		if (findInRoom(&rooms[i], uid) >= 0) { r = &rooms[i]; break; }
	if (!r) { sendTo(sock, "ERR|你不在任何房间"); return; }
	
	if (strcmp(cmd, "REFRESH") == 0) { sendRoomInfo(r); return; }
	
	if (strcmp(cmd, "LEAVE") == 0) {
		if (r->host == uid) { destroyRoom(r, "房主离开"); return; }
		int idx = findInRoom(r, uid);
		r->p[idx] = r->p[r->n - 1];
		r->n--;
		sendTo(sock, "OK|已离开房间");
		char b[128];
		sprintf(b, "SYS|%s 离开了房间", me->name);
		sendRoom(r, b);
		sendRoomInfo(r);
		return;
	}
	
	if (strcmp(cmd, "KICK") == 0) {
		if (r->host != uid) { sendTo(sock, "ERR|你不是房主"); return; }
		char *us = strtok(NULL, "|");
		int kuid = findUser(us);
		if (kuid < 0) { sendTo(sock, "ERR|用户不存在"); return; }
		int idx = findInRoom(r, kuid);
		if (idx < 0) { sendTo(sock, "ERR|玩家不在房间"); return; }
		if (kuid == uid) { destroyRoom(r, "房主离开"); return; }
		sendTo(users[kuid].sock, "SYS|你被房主移出房间");
		r->p[idx] = r->p[r->n - 1];
		r->n--;
		sendRoomInfo(r);
		return;
	}
	
	if (strcmp(cmd, "DISSOLVE") == 0) {
		if (r->host != uid) { sendTo(sock, "ERR|你不是房主"); return; }
		destroyRoom(r, "房主解散"); return;
	}
	
	if (strcmp(cmd, "START") == 0) {
		if (r->host != uid) { sendTo(sock, "ERR|你不是房主"); return; }
		if (r->state)       { sendTo(sock, "ERR|游戏已开始"); return; }
		startGame(r);
		return;
	}
	
	int pi = findInRoom(r, uid);
	if (pi < 0) return;
	
	/* 狼人杀 */
	if (r->mode == 1 && r->state == 1 && !r->over) {
		if (strcmp(cmd, "WOLF") == 0) {
			if (r->p[pi].role != 1) return;
			char *t = strtok(NULL, "|");
			int idx = findInRoomByName(r, t);
			if (idx >= 0 && r->p[idx].alive) r->wolfTgt = idx;
			r->p[pi].acted = 1;
		} else if (strcmp(cmd, "SEER") == 0) {
			if (r->p[pi].role != 2) return;
			char *t = strtok(NULL, "|");
			int idx = findInRoomByName(r, t);
			if (idx >= 0) {
				char b[128];
				sprintf(b, "SYS|查验：%s 是%s", t,
						r->p[idx].role == 1 ? "狼人" : "好人");
				sendTo(sock, b);
				r->seerTgt = idx;
			}
			r->p[pi].acted = 1;
		} else if (strcmp(cmd, "WITCH") == 0) {
			if (r->p[pi].role != 3) return;
			char *s  = strtok(NULL, "|");
			char *p2 = strtok(NULL, "|");
			if (s && strcmp(s, "0") != 0) {
				int idx = findInRoomByName(r, s);
				if (idx >= 0) r->witchSave = idx;
			}
			if (p2 && strcmp(p2, "0") != 0) {
				int idx = findInRoomByName(r, p2);
				if (idx >= 0) r->witchPoison = idx;
			}
			r->p[pi].acted = 1;
		} else if (strcmp(cmd, "GUARD") == 0) {
			if (r->p[pi].role != 4) return;
			char *t = strtok(NULL, "|");
			int idx = findInRoomByName(r, t);
			if (idx >= 0) r->guardTgt = idx;
			r->p[pi].acted = 1;
		} else if (strcmp(cmd, "KNIGHT") == 0) {
			if (r->p[pi].role != 5) return;
			char *t = strtok(NULL, "|");
			int idx = findInRoomByName(r, t);
			if (idx >= 0) r->knightTgt = idx;
			r->p[pi].acted = 1;
		} else if (strcmp(cmd, "VOTE") == 0) {
			char *t = strtok(NULL, "|");
			int idx = findInRoomByName(r, t);
			if (idx >= 0) wolfDayVote(r, pi, idx);
			return;
		}
		
		int done = 1;
		for (int i = 0; i < r->n; i++) {
			if (!r->p[i].alive) continue;
			int rr = r->p[i].role;
			if ((rr == 1 || rr == 2 || rr == 3 || rr == 4 || rr == 5) && !r->p[i].acted) {
				done = 0; break;
			}
		}
		if (done) wolfResolveNight(r);
		return;
	}
	
	/* 转盘 */
	if (r->mode == 2 && r->state == 1 && !r->over) {
		if (r->p[pi].gold <= 0) { sendTo(sock, "ERR|你没金币了"); return; }
		if (strcmp(cmd, "GUESS") == 0) {
			char *g = strtok(NULL, "|");
			char *b = strtok(NULL, "|");
			int guess = atoi(g), bet = atoi(b);
			if (guess < 1 || guess > 50 || bet <= r->p[pi].bet || bet > r->p[pi].gold) {
				sendTo(sock, "ERR|下注无效"); return;
			}
			r->p[pi].gold -= (bet - r->p[pi].bet);
			r->pool      += (bet - r->p[pi].bet);
			r->p[pi].bet = bet;
			r->p[pi].guess = guess;
			r->p[pi].acted = 1;
			char bb[128];
			sprintf(bb, "SYS|%s 猜 %d 下注 %d", users[uid].name, guess, bet);
			sendRoom(r, bb);
			ttCheck(r);
		} else if (strcmp(cmd, "QUIT") == 0) {
			r->p[pi].acted = 1;
			r->p[pi].gold = 0;
			sendRoom(r, "SYS|有玩家退出本轮");
			ttCheck(r);
		}
		return;
	}
	
	/* 射击 */
	if (r->mode == 3 && r->state == 1 && !r->over) {
		if (strcmp(cmd, "SHOOT") == 0) {
			char *x = strtok(NULL, "|");
			char *y = strtok(NULL, "|");
			int px = atoi(x), py = atoi(y);
			if (px < 0 || px >= 20 || py < 0 || py >= 20) {
				sendTo(sock, "ERR|坐标越界"); return;
			}
			r->p[pi].cx = px; r->p[pi].cy = py;
			int hit = 0;
			for (int k = 0; k < r->tc; k++)
				if (r->tx[k] == px && r->ty[k] == py) {
					r->tx[k] = -1; r->ty[k] = -1; hit = 1; break;
				}
			if (hit) {
				r->p[pi].score++;
				char b[128];
				sprintf(b, "SYS|%s 命中 +1 分（当前 %d）",
						users[uid].name, r->p[pi].score);
				sendRoom(r, b);
			} else {
				sendTo(sock, "SYS|未命中");
			}
			r->p[pi].shot = 1;
			shSyncField(r);
			shCheck(r);
		}
		return;
	}
}

/* ==================== 客户端线程 ==================== */
DWORD WINAPI clientThread(LPVOID arg) {
	SOCKET s = (SOCKET)(intptr_t)arg;
	int uid = -1;
	char buf[BUFSZ], line[BUFSZ];
	
	while (1) {
		int n = recv(s, buf, BUFSZ - 1, 0);
		if (n <= 0) break;
		buf[n] = '\0';
		char *p = buf;
		while (p && *p) {
			char *nl = strchr(p, '\n');
			if (nl) *nl = '\0';
			if (strlen(p) > 0) {
				EnterCriticalSection(&cs);
				strcpy(line, p);
				handleCmd(s, line, &uid);
				LeaveCriticalSection(&cs);
			}
			if (!nl) break;
			p = nl + 1;
		}
	}
	
	EnterCriticalSection(&cs);
	if (uid >= 0) {
		users[uid].sock = -1;
		printf("[服务端] 用户 %s 断开连接，在线=%d/%d\n",
			   users[uid].name, onlineCount(), nu);
		Room *r = NULL;
		int idx = -1;
		for (int i = 0; i < nr; i++) {
			int k = findInRoom(&rooms[i], uid);
			if (k >= 0) { r = &rooms[i]; idx = k; break; }
		}
		if (r) {
			if (r->host == uid) {
				destroyRoom(r, "房主掉线");
			} else {
				r->p[idx].sock = -1;
				char b[128];
				sprintf(b, "SYS|%s 掉线，站在原位等待游戏结束", users[uid].name);
				sendRoom(r, b);
				sendRoomInfo(r);
			}
		}
	}
	LeaveCriticalSection(&cs);
	closesocket(s);
	return 0;
}

/* ==================== 主函数 ==================== */
int main(void) {
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
	InitializeCriticalSection(&cs);
	srand((unsigned)time(NULL));
	
	loadUsers();
	printf("[服务端] 监听端口 %d，现有账号 %d 个\n", PORT, nu);
	
	SOCKET ls = socket(AF_INET, SOCK_STREAM, 0);
	int reuse = 1;
	setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));
	
	struct sockaddr_in a;
	memset(&a, 0, sizeof(a));
	a.sin_family      = AF_INET;
	a.sin_port        = htons(PORT);
	a.sin_addr.s_addr = htonl(INADDR_ANY);
	
	bind(ls, (struct sockaddr*)&a, sizeof(a));
	listen(ls, 10);
	printf("[服务端] 监听中...\n");
	
	while (1) {
		struct sockaddr_in cli;
		int len = sizeof(cli);
		SOCKET c = accept(ls, (struct sockaddr*)&cli, &len);
		if (c == INVALID_SOCKET) continue;
		printf("[服务端] 新连接接入，当前连接数=%d\n", onlineCount() + 1);
		CreateThread(NULL, 0, clientThread, (LPVOID)(intptr_t)c, 0, NULL);
	}
	WSACleanup();
	return 0;
}
