#pragma once
#undef UNICODE
#define WIN32_LEAN_AND_MEAN
#define WINSOCK_MINOR_VERSION	2
#define WINSOCK_MAJOR_VERSION	2
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <iostream>
#include <memory>
#include <vector>
#include <mmreg.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")


struct AudioInitPacket {
	WAVEFORMATEX audioFMT;
	DWORD fileType;
	DWORD audioByteSze; // get server to send audio section size from readchunk meth in examnle
	int duration;
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

struct Settings
{
	std::string Hostname;
	std::string Port;
};



class ClientSocket{
public:
	ClientSocket(Settings settings);
	~ClientSocket();
	int connectToServer();
	void close();
	struct PacketContents receive(int size);
	int sendData(const char * writeBuffer, int writeLength);
	SOCKET getSocket(void) {return (clientSocket);}

private:
	SOCKET clientSocket;
	struct sockaddr_in serverAddress;
	WSADATA wsaData;
	std::string serverIpAddress;
	u_short port;
};

