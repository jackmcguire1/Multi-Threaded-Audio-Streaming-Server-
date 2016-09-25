#pragma once
#undef UNICODE
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <chrono>
#include "WaveFile.h"
#include <algorithm>
#define WINSOCK_MINOR_VERSION	2
#define WINSOCK_MAJOR_VERSION	2
#define numOfClients 2


struct AudioInitPacket {
	WAVEFORMATEX audioFMT;
	DWORD fileType;
	DWORD audioByteSze;
};

struct PacketContents {
	int byteSize;
	std::vector<char> content;
};

struct WaveFileNameList {
	char waveFileName[1000];
};

struct UserChoice {
	int userValue;
};

//serverSettings.ini datastore
struct Settings
{
	string Hostname, Port, audioFileDir;
};

class ServerSocket{
SOCKET listenSocket = INVALID_SOCKET;

int status;
public:
	ServerSocket(Settings settings);
	~ServerSocket();
	SOCKET getSocket();
	void startListen(SOCKET socket);
	void acceptConnection();
	struct PacketContents Receive(int clientId);
	int Send(const char * writeBuffer, int writeLength, SOCKET clientSocket);
	void createWsaData();
	void closeClientSocket(int clientSocket);
	void getFilesFromDir(const string directory);
	ServerSocket& operator= (ServerSocket const &);
	std::vector<int> getClientList(void) { return clientList; }


private:

	void ClientAudioStreamThread(int arg1);
	void recieveClient(int arg1);
	void sendWaveExFormat(int arg1, int arg2);
	void sendAudioData(int arg1, int arg2);

	fd_set read;
	struct timeval waitInterval;
	WSADATA wsaData;
	PADDRINFOA  AddrInfo;
	bool listening = false;
	std::vector<int> clientList;
	vector<unique_ptr<WaveFile>> waveFiles;
	mutex clientAudioStream;
	std::vector<std::thread> clientAudioStreams;
};


