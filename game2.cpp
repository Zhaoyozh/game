#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib,"ws2_32.lib")

#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 9001
#define BUFSZ       4096

int sock = INVALID_SOCKET;

volatile LONG roomId   = 0;
volatile LONG inGame   = 0;
volatile LONG myUid    = -1;
volatile LONG loggedIn = 0;
volatile LONG hasField = 0;

char lastField[BUFSZ] = {0};

/* ---------- 颜色 ---------- */
void setColor(int c) {
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (WORD)c);
}
#define C_TITLE (FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_INTENSITY)
#define C_HEAD  (FOREGROUND_GREEN|FOREGROUND_INTENSITY)
#define C_OK    (FOREGROUND_GREEN)
#define C_ERR   (FOREGROUND_RED|FOREGROUND_INTENSITY)
#define C_SYS   (FOREGROUND_BLUE|FOREGROUND_GREEN|FOREGROUND_INTENSITY)
#define C_MENU  (FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_INTENSITY)
#define C_ROOM  (FOREGROUND_RED|FOREGROUND_BLUE|FOREGROUND_INTENSITY)
#define C_TGT   (FOREGROUND_RED|FOREGROUND_INTENSITY)
#define C_CRS   (FOREGROUND_GREEN|FOREGROUND_INTENSITY)
#define C_GRID  (FOREGROUND_INTENSITY)

void clr() { system("cls"); }

void sendLine(const char *s) {
	if (sock < 0) return;
	send(sock, s, (int)strlen(s), 0);
	send(sock, "\n", 1, 0);
}

void banner() {
	clr();
	setColor(C_MENU);
	printf("  +==============================+\n");
	printf("  |   **  C 游戏平台  v1.0  **   |\n");
	printf("  |      狼人杀 / 转盘 / 射击     |\n");
	printf("  +==============================+\n\n");
	setColor(C_TITLE);
}

/* ---------- 靶场绘制 ---------- */
void drawField(const char *data) {
	int fx[10], fy[10], fc = 0;
	int pid[10], px[10], py[10], pc = 0;
	
	char *copy = (char*)malloc(strlen(data) + 1);
	if (!copy) return;
	strcpy(copy, data);
	
	char *p = strtok(copy, "|");
	if (p) p = strtok(NULL, "|");
	while (p) {
		if (p[0] == 'T' && p[1] == ',') {
			int x, y;
			if (sscanf(p + 2, "%d,%d", &x, &y) == 2 && fc < 10) {
				fx[fc] = x; fy[fc] = y; fc++;
			}
		} else if (p[0] == 'P' && p[1] == ',') {
			int id, x, y;
			if (sscanf(p + 2, "%d,%d,%d", &id, &x, &y) == 3 && pc < 10) {
				pid[pc] = id; px[pc] = x; py[pc] = y; pc++;
			}
		}
		p = strtok(NULL, "|");
	}
	free(copy);
	
	setColor(C_MENU);
	printf("\n     ");
	for (int x = 0; x < 20; x++) printf("%d", x / 10);
	printf("\n     ");
	for (int x = 0; x < 20; x++) printf("%d", x % 10);
	printf("\n");
	
	for (int y = 0; y < 20; y++) {
		setColor(C_MENU);
		printf("  %2d ", y);
		for (int x = 0; x < 20; x++) {
			int isT = 0, isC = 0;
			for (int k = 0; k < fc; k++)
				if (fx[k] == x && fy[k] == y) { isT = 1; break; }
			for (int k = 0; k < pc; k++)
				if (px[k] == x && py[k] == y) { isC = 1; break; }
			
			if (isT && isC) {
				setColor(FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY);
				printf("!");
			} else if (isT) {
				setColor(C_TGT);  printf("O");
			} else if (isC) {
				setColor(C_CRS);  printf("+");
			} else {
				setColor(C_GRID); printf(".");
			}
		}
		printf("\n");
	}
	setColor(C_TITLE);
	printf("\n  图例: O=靶子  +=准心  !=重叠\n");
}

/* ---------- 接收线程 ---------- */
DWORD WINAPI recvThread(LPVOID arg) {
	char buf[BUFSZ];
	while (sock != INVALID_SOCKET) {
		int n = recv(sock, buf, BUFSZ - 1, 0);
		if (n <= 0) break;
		buf[n] = '\0';
		
		char *p = buf;
		while (p && *p) {
			char *nl = strchr(p, '\n');
			if (nl) *nl = '\0';
			
			if (strlen(p) > 0) {
				
				if (strncmp(p, "FIELD|", 6) == 0) {
					strcpy(lastField, p);
					InterlockedExchange(&hasField, 1);
					if (inGame) {
						clr();
						drawField(lastField);
						setColor(C_HEAD);
						printf("\n射击: SHOOT|x|y (0-19)  退出: 0\n> ");
						setColor(C_TITLE);
					}
				}
				else if (strncmp(p, "OK|", 3) == 0) {
					setColor(C_OK);
					printf("[OK] %s\n", p + 3);
					setColor(C_TITLE);
					if (strstr(p, "登录成功") || strstr(p, "已自动登录"))
						InterlockedExchange(&loggedIn, 1);
				}
				else if (strncmp(p, "ERR|", 4) == 0) {
					setColor(C_ERR);
					printf("[错误] %s\n", p + 4);
					setColor(C_TITLE);
				}
				else if (strncmp(p, "SYS|", 4) == 0) {
					setColor(C_SYS);
					printf("%s\n", p + 4);
					setColor(C_TITLE);
					if (strstr(p, "==========") && strstr(p, "胜利")) {
						InterlockedExchange(&inGame, 0);
						InterlockedExchange(&hasField, 0);
					}
					if (strstr(p, "房间已解散") || strstr(p, "移出房间")) {
						InterlockedExchange(&roomId, 0);
						InterlockedExchange(&inGame, 0);
						InterlockedExchange(&hasField, 0);
					}
				}
				else if (strncmp(p, "ROOM|", 5) == 0) {
					InterlockedExchange(&roomId, 1);
					setColor(C_ROOM);
					printf("\n[房间] ");
					char *q = strtok(p + 5, "|");
					while (q) { printf("%s ", q); q = strtok(NULL, "|"); }
					printf("\n");
					setColor(C_TITLE);
				}
				else if (strncmp(p, "PLAYERS", 7) == 0) {
					setColor(C_ROOM);
					printf("[玩家] ");
					char *q = strtok(p, "|");
					while ((q = strtok(NULL, "|"))) printf("%s  ", q);
					printf("\n");
					setColor(C_TITLE);
				}
				else if (strncmp(p, "ROOMLIST", 8) == 0) {
					setColor(C_ROOM);
					printf("\n[房间列表]\n");
					setColor(C_TITLE);
					char *q = strtok(p, "|");
					int idx = 1;
					while ((q = strtok(NULL, "|")))
						printf("  %d) %s\n", idx++, q);
				}
				else {
					printf("%s\n", p);
				}
			}
			if (!nl) break;
			p = nl + 1;
		}
	}
	printf("\n[网络] 与服务器断开\n");
	return 0;
}

/* ---------- 登录/注册 ---------- */
void doLogin() {
	char u[32], p[32], cmd[128];
	
	while (!loggedIn) {
		banner();
		setColor(C_HEAD);
		printf("1. 登录  2. 注册  0. 退出\n");
		setColor(C_TITLE);
		
		int c;
		scanf("%d", &c); getchar();
		if (c == 0) exit(0);
		
		printf("用户名: ");
		fgets(u, 32, stdin);
		u[strcspn(u, "\n")] = 0;
		
		printf("密码:   ");
		fgets(p, 32, stdin);
		p[strcspn(p, "\n")] = 0;
		
		if (c == 1) sprintf(cmd, "LOGIN|%s|%s", u, p);
		else        sprintf(cmd, "REG|%s|%s",   u, p);
		
		sendLine(cmd);
		
		for (int i = 0; i < 30 && !loggedIn; i++) Sleep(100);
		
		if (!loggedIn) {
			setColor(C_ERR);
			printf("\n[登录未成功] 请重试\n");
			setColor(C_TITLE);
			Sleep(800);
		}
	}
}

/* ---------- 主函数 ---------- */
int main(void) {
	SetConsoleTitleA("C 游戏平台");
	
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
	
	sock = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in a;
	memset(&a, 0, sizeof(a));
	a.sin_family = AF_INET;
	a.sin_port   = htons(SERVER_PORT);
	inet_pton(AF_INET, SERVER_IP, &a.sin_addr);
	
	if (connect(sock, (struct sockaddr*)&a, sizeof(a)) == SOCKET_ERROR) {
		printf("连接服务器失败\n");
		return 1;
	}
	
	CreateThread(NULL, 0, recvThread, NULL, 0, NULL);
	Sleep(300);
	
	doLogin();
	Sleep(300);
	
	while (1) {
		if (roomId == 0) {
			banner();
			setColor(C_HEAD);
			printf("  [房间大厅]\n");
			setColor(C_TITLE);
			printf("1. 刷新房间列表\n2. 创建房间\n3. 加入房间\n4. 退出登录\n");
			
			int c;
			scanf("%d", &c); getchar();
			
			char buf[256];
			if (c == 1) sendLine("ROOMS");
			else if (c == 2) {
				printf("玩法: 1=狼人杀 2=转盘 3=射击  : ");
				int m; scanf("%d", &m); getchar();
				sprintf(buf, "CREATE|%d", m);
				sendLine(buf);
			} else if (c == 3) {
				printf("房间ID: ");
				int rid; scanf("%d", &rid); getchar();
				sprintf(buf, "JOIN|%d", rid);
				sendLine(buf);
			} else if (c == 4) {
				sendLine("LOGOUT");
			}
			Sleep(300);
		}
		else if (!inGame) {
			banner();
			setColor(C_HEAD);
			printf("  [房间内]\n");
			setColor(C_TITLE);
			printf("1. 刷新\n2. 开始游戏(房主)\n3. 踢人(房主)\n4. 解散(房主)\n5. 离开房间\n");
			
			int c;
			scanf("%d", &c); getchar();
			
			char buf[256];
			if (c == 1) sendLine("REFRESH");
			else if (c == 2) sendLine("START");
			else if (c == 3) {
				printf("用户名: ");
				char n[32]; fgets(n, 32, stdin);
				n[strcspn(n, "\n")] = 0;
				sprintf(buf, "KICK|%s", n);
				sendLine(buf);
			}
			else if (c == 4) sendLine("DISSOLVE");
			else if (c == 5) {
				sendLine("LEAVE");
				InterlockedExchange(&roomId, 0);
			}
			Sleep(200);
		}
		else {
			/* 游戏中 */
			if (hasField) {
				clr();
				drawField(lastField);
			} else {
				banner();
			}
			setColor(C_HEAD);
			printf("\n[游戏] 可用命令:\n");
			printf("  狼人杀: WOLF|名字  SEER|名字  WITCH|救谁|毒谁(0=不用)\n");
			printf("          GUARD|名字  KNIGHT|名字  VOTE|名字\n");
			printf("  转盘:   GUESS|1-50|下注   QUIT\n");
			printf("  射击:   SHOOT|x|y (0-19)\n");
			printf("  输入 0 退出等待: > ");
			setColor(C_TITLE);
			
			char l[256];
			fgets(l, 256, stdin);
			l[strcspn(l, "\n")] = 0;
			if (strcmp(l, "0") != 0) sendLine(l);
		}
	}
	
	WSACleanup();
	return 0;
}
