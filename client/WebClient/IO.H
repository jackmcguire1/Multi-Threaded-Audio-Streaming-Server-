#pragma once
#include <iostream>
#include <string>
#include <vector>

using namespace std;

bool printSongNameList(vector<string> &songNameList)
{
	try
	{
		cout << "Avaliable Songs" << endl;
		cout << "--------------------------------" << endl;
		for (int i = 0; i < songNameList.size(); i++)
		{
			cout << "Song " << (i + 1) << ".   " << songNameList.data()[i] << endl;
			cout << "--------------------------------" << endl;
		}
		return true;
	}
	catch (exception e)
	{
		return false;
	}
}

bool IntTryParse(string &input, int& output)
{
	output = -1;
	for (char c : input)
		if (!isdigit(c))
			return false;
	sscanf(input.c_str(), "%d", &output);
	return true;
}

int takeInput(vector<string> &songNameList) {
	string input;
	int num1 = 0;
	bool done = false;
	while (!done)
	{
		printSongNameList(songNameList);
		cout << "Please choose one of the following songs to stream or type Exit." << endl;
		cout << "--------------------------------" << endl;
		cin >> input;
		cout << "--------------------------------" << endl;
		if (IntTryParse(input, num1))
		{
			if (num1 <= songNameList.size() && !(num1 <= 0))
			{
				return num1 - 1;
			}
		}
		if (input == "Exit" || input == "exit")
		{
			return 1000;
		}
		cout << "INCORRECT INPUT!" << endl;
		cout << "--------------------------------" << endl;
	}
}


int startOrStop() {
	string input;
	int num1 = 0;
	bool done = false;
	while (!done)
	{
		cout << "Please choose one of the following songs to stream or type Exit.\n";
		cin >> input;
		if (IntTryParse(input, num1))
		{
			return num1;
		}
		if (input == "start" || input == "stop")
		{
			return 1000;
		}
		cout << "Incorrect Input!\n";
	}
}