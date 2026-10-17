#pragma once
#include "common.h"

const int MP_PORT = 27015;
const int MP_MAX_CLIENTS = 3;

struct ServerInfo {
	string ip;
	int    players;
	int    maxPlayers;
};

bool mpInit();
void mpShutdown();

bool mpStartServer();
void mpServerStop();
bool mpServerIsRunning();
int  mpServerClientCount();
void mpServerTick();
void mpServerHostUpdate(const Vec3& pos, float yaw, float pitch);

vector<ServerInfo> mpDiscoverServers(int timeoutMs);
bool mpConnectToServer(const string& ip);
void mpDisconnect();
bool mpIsConnected();
void mpClientTick();
void mpClientSendState(const Vec3& pos, float yaw, float pitch);

string mpGetLocalIP();
