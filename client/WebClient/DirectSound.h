#pragma once
#include "Client.h"
#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <dsound.h>
#include <iostream>
#include <vector>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <chrono>
#include <queue>
#include <conio.h>
#include "stdafx.h"
#include <atlsync.h>
#include <atlwin.h>

#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "winmm.lib")

using namespace std;

class DirectSound {
public:
	DirectSound(AudioInitPacket * audioInitPacket, ClientSocket * clientSocket, int songChoice);
	~DirectSound();
	bool initialiseDeviceAndPrimaryBuffer();
	bool initialiseSecondaryBuffer();
	bool playWaveFile();
	bool play();
	bool stop();

private:
	
	void						 threadWaveSecond1();
	void						 threadPlay( bool * arg1, bool * arg2, bool * arg3, int * arg4);
	HRESULT						 writeData(int bufferOffset);

	CComPtr<IDirectSound8>  	 directSound = nullptr;
	CComPtr<IDirectSoundBuffer>	 primaryBuffer = nullptr;
	CComPtr<IDirectSoundBuffer>  secondaryBuffer = nullptr;
	vector<unsigned char>		dataBuffer;
	DWORD						dataBufferSize;
	DSBUFFERDESC				bufferDesc;
	unique_ptr<WAVEFORMATEX>	waveFormat;
	int							secondaryBufferOffset = 0;
	unique_ptr<ClientSocket>	clientSocket;
	vector<thread>				threads;
	mutex						queueLock;
	bool						finished = false;
	bool						firstplay = true;
	bool						playing = false;
	bool						fourSeconds = false;
};
