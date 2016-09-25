#pragma once
#include "Server.h"

using namespace std;

void RandomDelay()
{
	this_thread::sleep_for(chrono::milliseconds(rand() % 150 + 50));
}

/*Send the newly connected client the Audio Track List*/
void ServerSocket::ClientAudioStreamThread(int clientId)
{
	bool done = false;
	while (!done)
	{
		RandomDelay();
		clientAudioStream.lock();
		if (clientId == SOCKET_ERROR)
		{
			done = true;
			break;
		}
		else
		{
			WaveFileNameList waveFileList;
			vector<char> wavFileNameListWrapper(sizeof(WaveFileNameList));
			for (int i = 0; i < waveFiles.size(); i++)
			{
				strcpy(waveFileList.waveFileName, waveFiles[i]->getFileName().c_str());
				memcpy(wavFileNameListWrapper.data(), &waveFileList, sizeof(waveFileList));
				Send(wavFileNameListWrapper.data(), wavFileNameListWrapper.size(), clientId);
			}
			RandomDelay();
			vector<char> empty(101);
			Send(empty.data(), empty.size(), clientId);
			done = true;
		}
		clientAudioStream.unlock();
		clientAudioStreams.push_back(std::thread([=] {recieveClient(clientId); }));
	}
}

//send the client the WAVEFORMATEX and total size of the song they have chosen to listen to
void ServerSocket::sendWaveExFormat(int clientId, int waveFileIndex)
{
	bool done = false;
	bool isPresent = false;
	while (!done)
	{
		RandomDelay();
		clientAudioStream.lock();
		if (clientId == SOCKET_ERROR)
		{
			done = true;
			break;
		}
		AudioInitPacket audioFmtPacket;
		audioFmtPacket.audioFMT = waveFiles.data()[waveFileIndex]->getWaveFMT();
		audioFmtPacket.fileType = waveFiles.data()[waveFileIndex]->getFileType();
		audioFmtPacket.audioByteSze = waveFiles.data()[waveFileIndex]->getDataBufferSize();
		vector<AudioInitPacket> audioFormatPacket;
		audioFormatPacket.push_back(audioFmtPacket);
		std::vector<char> audioFmtData(sizeof(AudioInitPacket));
		memcpy(audioFmtData.data(), &audioFormatPacket[0], sizeof(audioFmtPacket));
		audioFormatPacket.clear();
		audioFormatPacket = vector<AudioInitPacket>();
		Send(audioFmtData.data(), audioFmtData.size(), clientId);
		audioFmtData = vector<char>();
		clientAudioStream.unlock();
		clientAudioStreams.push_back(std::thread([=] {sendAudioData(clientId,waveFileIndex); }));
		//clientAudioStreams.back().join();
		done = true;
	}

}

//send the audio data of a chosen song
void ServerSocket::sendAudioData(int clientId, int waveFileIndex)
{
	bool done = false;
	while (!done)
	{
		RandomDelay();
		clientAudioStream.lock();
		if (clientId == SOCKET_ERROR)
		{
			done = true;
			break;
		}
		std::vector<char> audioData(waveFiles.data()[waveFileIndex]->getDataBufferSize());
		memcpy(audioData.data(), waveFiles.data()[waveFileIndex]->getReadData().data(), waveFiles.data()[waveFileIndex]->getDataBufferSize());
		Send(audioData.data(), audioData.size(), clientId);
		audioData.clear();
		audioData = vector<char>();
		FD_SET(clientId, &read);
		done = true;
		clientAudioStream.unlock();
		clientAudioStreams.push_back(std::thread([=] {recieveClient(clientId); }));
	}
}

//recieve the chosen command from a client
// they either chooose a pecific song for playback or are disconnecting
void ServerSocket::recieveClient(int clientId)
{
	vector<UserChoice> waveFileSelected(1);
	bool done = false;
	while (!done)
	{
		RandomDelay();
		if (clientId == SOCKET_ERROR)
		{
			clientList.erase(std::remove(clientList.begin(), clientList.end(), clientId), clientList.end());
			done = true;
			break;
		}

		waitInterval.tv_sec = 60;
		waitInterval.tv_usec = 10;
		vector<PacketContents> clientWavFileChoice(1);
		bool userValueRecieved = false;
		while (!userValueRecieved)
		{
			if (FD_ISSET(clientId, &read))
			{
				FD_CLR(clientId, &read);
				userValueRecieved = true;
				clientWavFileChoice[0] = Receive(clientId);
			}
		}
	
		clientAudioStream.lock();
		
		memcpy(&waveFileSelected[0], clientWavFileChoice[0].content.data(), clientWavFileChoice[0].byteSize);
		clientWavFileChoice.clear();
		clientWavFileChoice = vector<PacketContents>();
		cout << "User " << clientId << " Sent value:- " << waveFileSelected[0].userValue << endl;

		//Depending on value provided by client, either remove client from list or continue processing
		done = true;
	}
	if (waveFileSelected[0].userValue == 1000)
	{
		waveFileSelected.clear();
		waveFileSelected = vector<UserChoice>();
		closeClientSocket(clientId);
		clientList.erase(std::remove(clientList.begin(), clientList.end(), clientId), clientList.end());
		clientAudioStream.unlock();
	}
	else
	{
		clientAudioStream.unlock();
		clientAudioStreams.push_back(std::thread([=] {sendWaveExFormat(clientId, waveFileSelected[0].userValue); }));
		waveFileSelected.clear();
		waveFileSelected = vector<UserChoice>();
		clientAudioStreams.back().join();
	}
}

//Socket Address Info
struct addrinfo * host_serv(const char *host, const char *serv)
{
	struct addrinfo hints, *data;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_IP;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo(host, serv, &hints, &data))
	{
		WSACleanup();
	}

	return (data);
};


void freeAddrResource(PADDRINFOA res)
{
	freeaddrinfo(res);
}

//create wsa data, sockaddr, wavFiles
ServerSocket::ServerSocket(Settings settings)
{

	createWsaData();
	AddrInfo = host_serv(settings.Hostname.c_str(), settings.Port.c_str());//create socket Addr info using serverSettings.ini
	listenSocket = socket(AddrInfo->ai_family, AddrInfo->ai_socktype, AddrInfo->ai_protocol);
	getFilesFromDir(settings.audioFileDir);
	if (waveFiles.size() != 0)
	{
		FD_ZERO(&read);
		startListen(listenSocket);
	}
	else
	{
		cout << "No Audio Files Found" << endl;
	}
}

ServerSocket::~ServerSocket() {

	freeAddrResource(AddrInfo);
	closesocket(listenSocket);
	WSACleanup();
	for (int i = 0; i < waveFiles.size(); i++)
	{
		waveFiles[i].release();
	}
	waveFiles.clear();
	clientAudioStreams.clear();
}

SOCKET ServerSocket::getSocket() {
	return listenSocket;
}

//socket bind and listen 
void ServerSocket::startListen(SOCKET listensocket)
{
	status = ::bind(listenSocket, AddrInfo->ai_addr, static_cast<int>(AddrInfo->ai_addrlen));
	status = listen(listenSocket, SOMAXCONN);
	listening = true;
	cout << "Server Is Ready To Accept Connections" << endl;
	acceptConnection();
}

//Accept newly connected clients
void ServerSocket::acceptConnection() {
	int clientId = -1;
	while (listening == true)
	{

		while ((clientId = accept(listenSocket,  NULL, NULL)) != -1)
		{
			if (clientList.size() < numOfClients)
			{
				clientList.push_back(clientId);
				FD_SET(clientId, &read);
				clientAudioStreams.push_back(std::thread([=] {ClientAudioStreamThread(clientId); }));
			}
			else
			{
				closeClientSocket(clientId);
				while (true)
				{
					clientAudioStream.lock();
					if (clientList.size() == 0)
					{
						clientAudioStream.unlock();
						break;
					}
					clientAudioStream.unlock();
				}
				listening = false;
				break;
			}
		}
	}
}

PacketContents ServerSocket::Receive(int clientId)
{
	std::vector<char> recvData(1280);
	int bytesReceived = 0;
	while ((bytesReceived = recv(clientId, recvData.data(), recvData.size(), 0)) != 1280) {
		if (bytesReceived == -1)
		{
			if (WSAGetLastError() == WSAEMSGSIZE) // server has more data to send than the buffer can get in one call
			{
				continue; // iterate again to get more data
			}
		}
		else
		{
			break;
		}
	}
	PacketContents data;
	data.content = std::move(recvData);
	data.byteSize = bytesReceived;
	recvData.clear();
	recvData = vector<char>();
	return data;
}

int ServerSocket::Send(const char * writeBuffer, int writeLength, SOCKET clientSocket)
{
	if (clientSocket != SOCKET_ERROR)
	{
		cout << writeLength << endl;
		int bytesSent = 0;
		while (bytesSent = send(clientSocket, writeBuffer, writeLength, 0) != writeLength)
		{
		}
		return bytesSent;
	}
	return 0;
}

void ServerSocket::createWsaData() 
{
	status = WSAStartup(MAKEWORD(WINSOCK_MAJOR_VERSION, WINSOCK_MINOR_VERSION), &wsaData);
	if (status != 0)
	{
		cout << "Unable to start sockets layer" << endl;
	}
}

void ServerSocket::closeClientSocket(int clientSocket)
{
	closesocket(clientSocket);
}

/* Retrieve a list of wave Files from specified PATH directory in 
   serverSettings.ini
*/
void ServerSocket::getFilesFromDir(const string directory)
{

	HANDLE dir;
	unique_ptr<WIN32_FIND_DATA> file_data(new WIN32_FIND_DATA);

	if ((dir = FindFirstFile((directory + "*.wav").c_str(), file_data.get())) == INVALID_HANDLE_VALUE)
		return; /* No files found */

	do {
		const string file_name = file_data->cFileName;
		const string full_file_name = directory + "\\" + file_name;
		const bool is_directory = (file_data->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

		if (file_name[0] == '.')
			continue;

		if (is_directory)
			continue;
		unique_ptr<WaveFile> waveFile(new WaveFile(full_file_name));
		waveFiles.push_back(std::move(waveFile));
	} while (FindNextFile(dir, file_data.get()));

	FindClose(dir);
} 



