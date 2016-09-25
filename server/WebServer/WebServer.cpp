#pragma once
#include "Server.h"
#include "WebServer.h"
#include <fstream>
#include <iostream>
using namespace std;

 Settings readConfigurationFile()
{
	Settings configuration;
	string data = "";
	ifstream iFile;
	
	iFile.open("../ServerSettings.ini");
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

		//Track List DIR
		getline(iFile, data, '=');
		getline(iFile, data, '\n');
		configuration.audioFileDir = data;
		
		iFile.close();
	}
	
	return configuration;
};

int main(void)
{
	Settings serverConfiguration = readConfigurationFile();
	unique_ptr<ServerSocket> serverSocket(new ServerSocket(serverConfiguration));
	cout << "Server ShutDown" << endl;
	system("pause");
	return 0;
};

