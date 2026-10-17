#include "common.h"
#include "multiplayer.h"

#define USE_LOGIN 0

// ============================================================
//  登录 / 注册
// ============================================================
int loginSystem(string& outUser) {
	hideCursor(false);
	syncSystemCursor(true);
	enableConsoleInput(true);
	
	ifstream f; ofstream of; string s, mm; int n = 0;
	{
		f.open("cd.world");
		if (!f.is_open()) { of.open("cd.world"); of << "0\n"; of.close(); }
		else f.close();
	}
	while (1) {
		f.open("cd.world"); f >> n;
		if (n < 0 || n > 10) n = 0;
		clearScreen();
		cout << "\n\n";
		cout << "  ================================\n";
		cout << "        我  的  世  界\n";
		cout << "  ================================\n\n";
		cout << "   姓名: "; cin >> s;
		cout << "   密码: "; cin >> mm;
		int xj = 1;
		for (int i = 1; i <= n; ++i) {
			string a, b; f >> a >> b;
			if (s == a) {
				xj = 0;
				if (mm == b) { cout << "\n   登录成功！\n"; xj = -1; break; }
				else cout << "\n   密码错误\n";
			}
		}
		if (xj == -1) { outUser = s; f.close(); Sleep(500); return 1; }
		else if (xj == 1) {
			if (n == 10) cout << "   用户已满\n";
			else {
				f.close();
				f.open("cd.world"); f >> n;
				string a[11], b[11];
				for (int i = 1; i <= n; ++i) f >> a[i] >> b[i];
				f.close();
				of.open("cd.world");
				of << ++n << "\n";
				for (int i = 1; i <= n - 1; ++i) of << a[i] << " " << b[i] << "\n";
				of << s << " " << mm << "\n";
				of.close();
				cout << "   用户已创建\n";
				outUser = s; Sleep(800); return 1;
			}
		}
		f.close(); Sleep(800);
	}
}

// ============================================================
//  主菜单
// ============================================================
int showMenu(const string& user) {
	int sel = 0;
	hideCursor(false); syncSystemCursor(true); enableConsoleInput(true);
	while (true) {
		clearScreen();
		cout << "\n\n";
		cout << "                       #################\n";
		cout << "                      #我   的   世   界#\n";
		cout << "                       #################\n\n\n";
		cout << (sel == 0 ? "  > " : "    ") << "---   单 人 游 戏   ---\n\n";
		cout << (sel == 1 ? "  > " : "    ") << "---   联 机 大 厅   ---\n\n";
		cout << (sel == 2 ? "  > " : "    ") << "---   退    出     ---\n\n\n";
		cout << "  当前用户: " << user << "\n";
		cout << "  ↑/↓ 选择   Enter 确认\n";
		cout.flush();
		static bool upPrev = false, dnPrev = false, enPrev = false;
		bool upNow = (GetAsyncKeyState(VK_UP)     & 0x8000) != 0;
		bool dnNow = (GetAsyncKeyState(VK_DOWN)   & 0x8000) != 0;
		bool enNow = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
		if (upNow && !upPrev) sel = (sel + 2) % 3;
		if (dnNow && !dnPrev) sel = (sel + 1) % 3;
		upPrev = upNow; dnPrev = dnNow;
		if (enNow && !enPrev) {
			enPrev = enNow;
			if (sel == 0) return 0;
			if (sel == 1) return 1;
			if (sel == 2) return 2;
		}
		enPrev = enNow;
		Sleep(30);
	}
}

// ============================================================
//  联机大厅
// ============================================================
int showMultiplayerMenu() {
	int sel = 0;
	hideCursor(false); syncSystemCursor(true); enableConsoleInput(true);
	while (true) {
		clearScreen();
		cout << "\n\n";
		cout << "              联  机  大  厅\n\n\n";
		cout << (sel == 0 ? "  > " : "    ") << "---   创 建 服 务 器  ---\n\n";
		cout << (sel == 1 ? "  > " : "    ") << "---   搜 索 服 务 器  ---\n\n";
		cout << (sel == 2 ? "  > " : "    ") << "---   返 回 主 菜 单  ---\n\n\n";
		cout << "  ↑/↓ 选择   Enter 确认\n";
		cout.flush();
		static bool upPrev = false, dnPrev = false, enPrev = false;
		bool upNow = (GetAsyncKeyState(VK_UP)     & 0x8000) != 0;
		bool dnNow = (GetAsyncKeyState(VK_DOWN)   & 0x8000) != 0;
		bool enNow = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
		if (upNow && !upPrev) sel = (sel + 2) % 3;
		if (dnNow && !dnPrev) sel = (sel + 1) % 3;
		upPrev = upNow; dnPrev = dnNow;
		if (enNow && !enPrev) {
			enPrev = enNow;
			return sel;
		}
		enPrev = enNow;
		Sleep(30);
	}
}

// ============================================================
//  创建服务端
// ============================================================
int runCreateServer() {
	if (!mpStartServer()) {
		clearScreen();
		cout << "\n\n  启动服务端失败！可能端口被占用。\n\n  按任意键返回...\n";
		while (!_kbhit()) {}
		getch();
		return -1;
	}
	g_myPlayerId = 0;
	g_isMultiplayer = true;
	
	bool prevEnter = true;
	
	while (true) {
		clearScreen();
		cout << "\n\n";
		cout << "  ================================================\n";
		cout << "               服 务 端 已 启 动\n";
		cout << "  ================================================\n\n";
		cout << "    本机 IP  : " << mpGetLocalIP() << "\n";
		cout << "    端口     : " << MP_PORT << "\n";
		cout << "    房间人数 : " << (mpServerClientCount() + 1) << " / " << (MP_MAX_CLIENTS + 1) << "\n\n";
		cout << "    玩家列表:\n";
		cout << "      [主机] 我 (" << mpGetLocalIP() << ")\n";
		for (int i = 0; i < mpServerClientCount(); ++i)
			cout << "      [玩家 " << (i + 1) << "] 已加入\n";
		cout << "\n  ------------------------------------------------\n";
		cout << "    等待其他玩家加入……\n\n";
		cout << "    Enter = 开始游戏\n";
		cout << "    ESC   = 取消\n";
		cout.flush();
		
		mpServerTick();
		
		bool enterNow = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
		if (enterNow && !prevEnter) {
			prevEnter = enterNow;
			Sleep(200);
			return 0;
		}
		prevEnter = enterNow;
		
		if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
			Sleep(200);
			mpServerStop();
			g_isMultiplayer = false;
			return -1;
		}
		Sleep(50);
	}
}

// ============================================================
//  搜索并连接
// ============================================================
int runJoinServer() {
	clearScreen();
	cout << "\n\n  正在搜索局域网内的服务端...\n";
	cout.flush();
	
	auto servers = mpDiscoverServers(800);
	
	if (servers.empty()) {
		clearScreen();
		cout << "\n\n  未找到任何服务端。\n\n  请确认主机已启动「创建服务端」。\n\n  按任意键返回...\n";
		while (!_kbhit()) {}
		getch();
		return -1;
	}
	
	int sel = 0;
	bool prevUp = false, prevDn = false, prevEn = false;
	
	while (true) {
		clearScreen();
		cout << "\n\n";
		cout << "  === 找到 " << servers.size() << " 个服务端 ===\n\n";
		for (size_t i = 0; i < servers.size(); ++i) {
			cout << ((int)i == sel ? "  > " : "    ")
			<< servers[i].ip
			<< "    玩家 " << servers[i].players << "/" << servers[i].maxPlayers
			<< "\n\n";
		}
		cout << "  ↑/↓ 选择   Enter 连接   ESC 返回\n";
		cout.flush();
		
		bool upNow = (GetAsyncKeyState(VK_UP)     & 0x8000) != 0;
		bool dnNow = (GetAsyncKeyState(VK_DOWN)   & 0x8000) != 0;
		bool enNow = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
		
		if (upNow && !prevUp) sel = (sel + (int)servers.size() - 1) % (int)servers.size();
		if (dnNow && !prevDn) sel = (sel + 1) % (int)servers.size();
		prevUp = upNow; prevDn = dnNow;
		
		if (enNow && !prevEn) {
			prevEn = enNow;
			clearScreen();
			cout << "\n\n  正在连接到 " << servers[sel].ip << " ...\n";
			cout.flush();
			Sleep(300);
			if (mpConnectToServer(servers[sel].ip)) {
				cout << "\n  连接成功！\n";
				Sleep(500);
				return 0;
			} else {
				cout << "\n  连接失败！\n\n  按任意键返回...\n";
				while (!_kbhit()) {}
				getch();
				return -1;
			}
		}
		if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
			Sleep(200);
			return -1;
		}
		Sleep(30);
	}
}

// ============================================================
//  入口
// ============================================================
int main() {
	SetConsoleOutputCP(65001);
	SetConsoleCP(65001);
	enableAnsi();
	setupFixedConsole();
	srand((unsigned)time(nullptr));
	mpInit();
	
	clearScreen();
	cout << "\n  我 的 世 界\n\n  按任意键继续...\n";
	while (!_kbhit()) {}
	getch();
	
	string currentUser = "Guest";
#if USE_LOGIN
	loginSystem(currentUser);
	Sleep(500);
#endif
	g_currentUser = currentUser;
	
	while (true) {
		int r = showMenu(currentUser);
		if (r == 2) break;
		
		if (r == 0) {
			g_isMultiplayer = false;
			enableConsoleInput(false);
			runGame();
			hideCursor(false); syncSystemCursor(true); enableConsoleInput(true);
		}
		else if (r == 1) {
			int mp = showMultiplayerMenu();
			if (mp == 2) continue;
			if (mp == 0) {
				if (runCreateServer() == 0) {
					enableConsoleInput(false);
					runGame();
					mpServerStop();
					g_isMultiplayer = false;
					hideCursor(false); syncSystemCursor(true); enableConsoleInput(true);
				}
			}
			if (mp == 1) {
				if (runJoinServer() == 0) {
					enableConsoleInput(false);
					runGame();
					mpDisconnect();
					g_isMultiplayer = false;
					hideCursor(false); syncSystemCursor(true); enableConsoleInput(true);
				}
			}
		}
	}
	
	mpShutdown();
	clearScreen();
	cout << "  再见\n";
	return 0;
}
