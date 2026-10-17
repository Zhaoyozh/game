#include "multiplayer.h"

static bool g_wsaInit = false;
static bool g_serverRunning = false;
static bool g_clientConnected = false;

static SOCKET g_serverListen = INVALID_SOCKET;
static SOCKET g_serverUdp    = INVALID_SOCKET;
static vector<SOCKET> g_serverClients;
static vector<int>    g_serverClientIds;

static SOCKET g_clientSock = INVALID_SOCKET;
static int    g_clientId   = -1;

// ---------------- 工具 ----------------
string mpGetLocalIP() {
	char hostname[256];
	if (gethostname(hostname, sizeof(hostname)) != 0) return "127.0.0.1";
	struct hostent* he = gethostbyname(hostname);
	if (!he) return "127.0.0.1";
	struct in_addr** addr_list = (struct in_addr**)he->h_addr_list;
	for (int i = 0; addr_list[i]; ++i) {
		string ip = inet_ntoa(*addr_list[i]);
		if (ip != "127.0.0.1") return ip;
	}
	return "127.0.0.1";
}

bool mpInit() {
	if (g_wsaInit) return true;
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;
	g_wsaInit = true;
	return true;
}

void mpShutdown() {
	mpServerStop();
	mpDisconnect();
	if (g_wsaInit) {
		WSACleanup();
		g_wsaInit = false;
	}
}

// ---------------- 服务端 ----------------
bool mpStartServer() {
	if (!mpInit()) return false;
	if (g_serverRunning) return true;
	
	g_serverListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (g_serverListen == INVALID_SOCKET) return false;
	
	int opt = 1;
	setsockopt(g_serverListen, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
	
	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(MP_PORT);
	
	if (bind(g_serverListen, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
		closesocket(g_serverListen); g_serverListen = INVALID_SOCKET; return false;
	}
	if (listen(g_serverListen, MP_MAX_CLIENTS) == SOCKET_ERROR) {
		closesocket(g_serverListen); g_serverListen = INVALID_SOCKET; return false;
	}
	u_long nb = 1;
	ioctlsocket(g_serverListen, FIONBIO, &nb);
	
	g_serverUdp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (g_serverUdp != INVALID_SOCKET) {
		int bopt = 1;
		setsockopt(g_serverUdp, SOL_SOCKET, SO_REUSEADDR, (char*)&bopt, sizeof(bopt));
		sockaddr_in uaddr = {};
		uaddr.sin_family = AF_INET;
		uaddr.sin_addr.s_addr = INADDR_ANY;
		uaddr.sin_port = htons(MP_PORT);
		bind(g_serverUdp, (sockaddr*)&uaddr, sizeof(uaddr));
		ioctlsocket(g_serverUdp, FIONBIO, &nb);
	}
	
	g_serverClients.clear();
	g_serverClientIds.clear();
	g_remotePlayers.clear();
	
	RemotePlayer me;
	me.id = 0;
	me.pos = {0, 0, 0};
	me.yaw = 0; me.pitch = 0;
	me.active = true;
	g_remotePlayers.push_back(me);
	
	g_serverRunning = true;
	return true;
}

void mpServerStop() {
	if (g_serverListen != INVALID_SOCKET) {
		closesocket(g_serverListen); g_serverListen = INVALID_SOCKET;
	}
	if (g_serverUdp != INVALID_SOCKET) {
		closesocket(g_serverUdp); g_serverUdp = INVALID_SOCKET;
	}
	for (SOCKET s : g_serverClients) closesocket(s);
	g_serverClients.clear();
	g_serverClientIds.clear();
	g_serverRunning = false;
	g_remotePlayers.clear();
}

bool mpServerIsRunning() { return g_serverRunning; }
int  mpServerClientCount() { return (int)g_serverClients.size(); }

void mpServerHostUpdate(const Vec3& pos, float yaw, float pitch) {
	for (auto& rp : g_remotePlayers) {
		if (rp.id == 0) {
			rp.pos = pos; rp.yaw = yaw; rp.pitch = pitch;
			return;
		}
	}
}

void mpServerTick() {
	if (!g_serverRunning) return;
	
	if (g_serverUdp != INVALID_SOCKET) {
		char buf[64];
		sockaddr_in from;
		int fromLen = sizeof(from);
		while (true) {
			int n = recvfrom(g_serverUdp, buf, sizeof(buf) - 1, 0, (sockaddr*)&from, &fromLen);
			if (n <= 0) break;
			buf[n] = 0;
			if (strncmp(buf, "MCSRV?", 6) == 0) {
				char resp[64];
				sprintf(resp, "MCSRV! %d %d", (int)g_serverClients.size() + 1, MP_MAX_CLIENTS + 1);
				sendto(g_serverUdp, resp, (int)strlen(resp), 0, (sockaddr*)&from, fromLen);
			}
		}
	}
	
	if ((int)g_serverClients.size() < MP_MAX_CLIENTS) {
		sockaddr_in caddr;
		int clen = sizeof(caddr);
		SOCKET cs = accept(g_serverListen, (sockaddr*)&caddr, &clen);
		if (cs != INVALID_SOCKET) {
			u_long nb = 1;
			ioctlsocket(cs, FIONBIO, &nb);
			int newId = 1;
			for (int i = 1; i <= MP_MAX_CLIENTS; ++i) {
				bool used = false;
				for (int id : g_serverClientIds) if (id == i) { used = true; break; }
				if (!used) { newId = i; break; }
			}
			g_serverClients.push_back(cs);
			g_serverClientIds.push_back(newId);
			
			RemotePlayer rp;
			rp.id = newId;
			rp.pos = {0, 0, 0};
			rp.yaw = 0; rp.pitch = 0;
			rp.active = true;
			g_remotePlayers.push_back(rp);
			
			char idMsg[5];
			idMsg[0] = 3;
			memcpy(idMsg + 1, &newId, 4);
			send(cs, idMsg, 5, 0);
		}
	}
	
	vector<int> dead;
	for (size_t i = 0; i < g_serverClients.size(); ++i) {
		SOCKET cs = g_serverClients[i];
		int cid = g_serverClientIds[i];
		char buf[128];
		int n = recv(cs, buf, sizeof(buf), 0);
		if (n == 0) { dead.push_back(cid); continue; }
		if (n < 0) continue;
		if (n >= 1 + 5 * 4 && buf[0] == 1) {
			float x, y, z, yw, pt;
			memcpy(&x,  buf + 1,  4);
			memcpy(&y,  buf + 5,  4);
			memcpy(&z,  buf + 9,  4);
			memcpy(&yw, buf + 13, 4);
			memcpy(&pt, buf + 17, 4);
			for (auto& rp : g_remotePlayers) {
				if (rp.id == cid) {
					rp.pos = {x, y, z};
					rp.yaw = yw; rp.pitch = pt;
					break;
				}
			}
		}
	}
	
	for (int deadId : dead) {
		for (size_t i = 0; i < g_serverClients.size(); ++i) {
			if (g_serverClientIds[i] == deadId) {
				closesocket(g_serverClients[i]);
				g_serverClients.erase(g_serverClients.begin() + i);
				g_serverClientIds.erase(g_serverClientIds.begin() + i);
				break;
			}
		}
		for (size_t i = 0; i < g_remotePlayers.size(); ++i) {
			if (g_remotePlayers[i].id == deadId) {
				g_remotePlayers.erase(g_remotePlayers.begin() + i);
				break;
			}
		}
	}
	
	if (!g_serverClients.empty()) {
		char out[1024];
		int idx = 0;
		out[idx++] = 2;
		out[idx++] = (char)g_remotePlayers.size();
		for (auto& rp : g_remotePlayers) {
			memcpy(out + idx, &rp.id, 4);      idx += 4;
			memcpy(out + idx, &rp.pos.x, 4);   idx += 4;
			memcpy(out + idx, &rp.pos.y, 4);   idx += 4;
			memcpy(out + idx, &rp.pos.z, 4);   idx += 4;
			memcpy(out + idx, &rp.yaw, 4);     idx += 4;
			memcpy(out + idx, &rp.pitch, 4);   idx += 4;
		}
		for (SOCKET cs : g_serverClients) send(cs, out, idx, 0);
	}
}

// ---------------- 客户端 ----------------
vector<ServerInfo> mpDiscoverServers(int timeoutMs) {
	vector<ServerInfo> result;
	if (!mpInit()) return result;
	
	SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (s == INVALID_SOCKET) return result;
	
	int bopt = 1;
	setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char*)&bopt, sizeof(bopt));
	
	u_long nb = 1;
	ioctlsocket(s, FIONBIO, &nb);
	
	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_BROADCAST;
	addr.sin_port = htons(MP_PORT);
	
	const char* msg = "MCSRV?";
	sendto(s, msg, 6, 0, (sockaddr*)&addr, sizeof(addr));
	
	DWORD start = GetTickCount();
	while (GetTickCount() - start < (DWORD)timeoutMs) {
		fd_set rd;
		FD_ZERO(&rd);
		FD_SET(s, &rd);
		timeval tv = {0, 50000};
		int r = select(0, &rd, NULL, NULL, &tv);
		if (r > 0) {
			char buf[128];
			sockaddr_in from;
			int fromLen = sizeof(from);
			int n = recvfrom(s, buf, sizeof(buf) - 1, 0, (sockaddr*)&from, &fromLen);
			if (n > 0) {
				buf[n] = 0;
				if (strncmp(buf, "MCSRV!", 6) == 0) {
					int players = 0, mx = 0;
					sscanf(buf + 6, "%d %d", &players, &mx);
					ServerInfo info;
					info.ip = inet_ntoa(from.sin_addr);
					info.players = players;
					info.maxPlayers = mx;
					bool dup = false;
					for (auto& x : result) if (x.ip == info.ip) { dup = true; break; }
					if (!dup) result.push_back(info);
				}
			}
		}
	}
	closesocket(s);
	return result;
}

bool mpConnectToServer(const string& ip) {
	if (!mpInit()) return false;
	mpDisconnect();
	
	g_clientSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (g_clientSock == INVALID_SOCKET) return false;
	
	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(MP_PORT);
	addr.sin_addr.s_addr = inet_addr(ip.c_str());
	
	if (connect(g_clientSock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
		closesocket(g_clientSock);
		g_clientSock = INVALID_SOCKET;
		return false;
	}
	
	u_long nb = 1;
	ioctlsocket(g_clientSock, FIONBIO, &nb);
	
	g_clientConnected = true;
	g_clientId = -1;
	
	DWORD start = GetTickCount();
	while (GetTickCount() - start < 500) {
		char buf[64];
		int n = recv(g_clientSock, buf, sizeof(buf), 0);
		if (n >= 5 && buf[0] == 3) {
			memcpy(&g_clientId, buf + 1, 4);
			break;
		}
		Sleep(20);
	}
	
	g_remotePlayers.clear();
	g_myPlayerId = (g_clientId > 0) ? g_clientId : 1;
	g_isMultiplayer = true;
	return true;
}

void mpDisconnect() {
	if (g_clientSock != INVALID_SOCKET) {
		closesocket(g_clientSock);
		g_clientSock = INVALID_SOCKET;
	}
	g_clientConnected = false;
	g_clientId = -1;
	if (!g_serverRunning) {
		g_remotePlayers.clear();
		g_isMultiplayer = false;
	}
}

bool mpIsConnected() { return g_clientConnected; }

void mpClientTick() {
	if (!g_clientConnected) return;
	
	while (true) {
		char buf[1024];
		int n = recv(g_clientSock, buf, sizeof(buf), 0);
		if (n <= 0) {
			if (n == 0) { mpDisconnect(); }
			break;
		}
		if (n >= 2 && buf[0] == 2) {
			int cnt = (unsigned char)buf[1];
			int idx = 2;
			g_remotePlayers.clear();
			for (int i = 0; i < cnt; ++i) {
				if (idx + 24 > n) break;
				RemotePlayer rp;
				memcpy(&rp.id,    buf + idx, 4); idx += 4;
				memcpy(&rp.pos.x, buf + idx, 4); idx += 4;
				memcpy(&rp.pos.y, buf + idx, 4); idx += 4;
				memcpy(&rp.pos.z, buf + idx, 4); idx += 4;
				memcpy(&rp.yaw,   buf + idx, 4); idx += 4;
				memcpy(&rp.pitch, buf + idx, 4); idx += 4;
				rp.active = true;
				g_remotePlayers.push_back(rp);
			}
		}
	}
}

void mpClientSendState(const Vec3& pos, float yaw, float pitch) {
	if (!g_clientConnected) return;
	char buf[1 + 5 * 4];
	buf[0] = 1;
	memcpy(buf + 1,  &pos.x, 4);
	memcpy(buf + 5,  &pos.y, 4);
	memcpy(buf + 9,  &pos.z, 4);
	memcpy(buf + 13, &yaw,   4);
	memcpy(buf + 17, &pitch, 4);
	send(g_clientSock, buf, sizeof(buf), 0);
}
