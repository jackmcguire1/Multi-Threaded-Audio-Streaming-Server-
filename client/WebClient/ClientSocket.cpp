#include "Client.h"
using namespace std;



ClientSocket::ClientSocket(Settings settings)
{
	clientSocket = -1;
	serverIpAddress = settings.Hostname;
	port = (unsigned short)strtoul(settings.Port.c_str(), NULL, 0);
}

ClientSocket::~ClientSocket()
{
	WSACleanup();
	closesocket(clientSocket);
}

int ClientSocket::connectToServer()
{	
	WSAStartup(MAKEWORD(WINSOCK_MAJOR_VERSION, WINSOCK_MINOR_VERSION), &wsaData);
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(port);
	serverAddress.sin_addr.s_addr = inet_addr(serverIpAddress.c_str());
	clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

	if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
		return 0;
	}

	return 1;
}

PacketContents ClientSocket::receive(int size)
{
	std::vector<char> recvData(size);
	int bytesReceived = 0;
	if (this != NULL)
	{
		while ((bytesReceived = recv(clientSocket, recvData.data(), recvData.size(), 0)) != recvData.size())
		{
			if (bytesReceived == -1)
			{
				if (WSAGetLastError() == WSAEMSGSIZE) // server has more data to send than the buffer can get in one call
				{
					continue; // iterate again to get more data
				}
			}
			else
			{
				// if iResult is numeric then it has retrieved what it needed and no more is currently available so if we call recv() again it will block.
				//printf("Bytes received: %d\n", bytesReceived);
				break;
			}
		}
	
	PacketContents data;
	data.content = recvData;
	data.byteSize = bytesReceived;
	return data;
	}
}

int ClientSocket::sendData(const char * writeBuffer, int writeLength)
{
	if (clientSocket != SOCKET_ERROR)
	{
		int bytesSent = 0;
		while (bytesSent = send(clientSocket, writeBuffer, writeLength, 0) != writeLength)
		{
		}
		return bytesSent;
	}
	return 0;
}

void ClientSocket::close()
{
	shutdown(clientSocket, SD_SEND);
	closesocket(clientSocket);
}







