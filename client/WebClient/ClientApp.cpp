#pragma once
#include "Client.h"
#include "DirectSound.h"
#include "IO.H"
#include <sstream>
#include <fstream>
#include <iostream>

using namespace std;
struct WaveFileNameListAsString {
	std::vector<string> waveFileNames;
};

void RandomDelay()
{
	this_thread::sleep_for(chrono::milliseconds(rand() % 150 + 50));
}

//read configuration settings
struct Settings readConfigurationFile()
{
	Settings configuration;
	string data = "";
	ifstream iFile;

	iFile.open("../ClientSettings.ini");
	if (iFile.is_open())
	{
		//Ipv4 Address
		getline(iFile, data, '=');
		getline(iFile, data, '\n');
		configuration.Hostname = data;

		//Port
		getline(iFile, data, '=');
		getline(iFile, data, '\n');
		configuration.Port = data;

		iFile.close();
	}

	return configuration;
};

int main(void)
{
	//setup socket with settings provided from configuration
	Settings clientSettings = readConfigurationFile();
	unique_ptr<ClientSocket> clientSocket(new ClientSocket(clientSettings));
	if (!clientSocket->connectToServer())
	{
		cout << "Server Is Not Operational." << endl;
		system("pause");
		return 0;
	}
	
	//request a list of audio tracks avaliable for streaming
	PacketContents fileName = clientSocket->receive(1000);
	if (fileName.byteSize == 0)
	{
		cout << "Server Reached Max Connections." << endl;
		system("pause");
		return 0;
	}
	WaveFileNameList wavFileNames;
	WaveFileNameListAsString wavFileNamesAsString;
	vector<char> fileNameChar;
	while(fileName.byteSize == sizeof(WaveFileNameList)) {
		memcpy(&wavFileNames, fileName.content.data(), fileName.byteSize);
		string newString(wavFileNames.waveFileName, strlen(wavFileNames.waveFileName));
		wavFileNamesAsString.waveFileNames.push_back(newString);
		RandomDelay();
		fileName = clientSocket->receive(1000);
	}


	unique_ptr<UserChoice> userChoice(new UserChoice);
	while (userChoice->userValue != 1000)
	{
		//ask user which song they would like to stream from track list
		userChoice->userValue = takeInput(wavFileNamesAsString.waveFileNames);
		if (userChoice->userValue != 1000)
		{
			//convert user choice into suitable format to send over socket.
			vector<char> sendUserChoice(sizeof(UserChoice));
			memcpy(sendUserChoice.data(), userChoice.get(), sizeof(userChoice));
			int test = clientSocket->sendData(sendUserChoice.data(),sendUserChoice.size());

			//recieve the total size and waveformat of the song
			RandomDelay();
			PacketContents audioFormatDataPacket = clientSocket->receive(28);
			unique_ptr<AudioInitPacket> waveFormat(new AudioInitPacket);
			memcpy(waveFormat.get(), audioFormatDataPacket.content.data(), audioFormatDataPacket.byteSize);
			//create direct sound object with provided waveFormat and other required objects
			unique_ptr<DirectSound> directSound(new DirectSound(waveFormat.get(), clientSocket.get(), userChoice->userValue));
		}

		else 
		{
			//Tell the Server we are exiting and to shut down the socket connection
			vector<char> sendUserChoice(sizeof(UserChoice));
			memcpy(sendUserChoice.data(), userChoice.get(), sizeof(userChoice));
			int test = clientSocket->sendData(sendUserChoice.data(),sendUserChoice.size());
		}
	}
	
	return 0;
};