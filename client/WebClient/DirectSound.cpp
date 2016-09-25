#include "DirectSound.h"

DirectSound::DirectSound(AudioInitPacket * AudioInitPacket, ClientSocket * clientSocketProvided, int songChoice)
{
	//set waveFormat and third buffer from provided values
	waveFormat.reset(&AudioInitPacket->audioFMT);
	dataBufferSize = AudioInitPacket->audioByteSze;
	dataBuffer.resize(dataBufferSize);

	//start data streaming thread, with buffer no larger then 12800
	clientSocket.reset(clientSocketProvided);
	threads.push_back(std::thread([=] {threadWaveSecond1(); }));
	//initialise primary and secondary buffer
	if (initialiseDeviceAndPrimaryBuffer() == 0)
	{
		cout << "failure to initialise Primary Buffer" << endl;
	}
	if (initialiseSecondaryBuffer() == 0)
	{
		cout << "failure to initialise Secondary Buffer" << endl;
	}
	//Play Song
	playWaveFile();
}

DirectSound::~DirectSound()
{
	clientSocket.release();
	for (int i = 0; i < threads.size(); i++)
	{
		threads[i].join();
	}
	waveFormat.release();
}

bool DirectSound::initialiseDeviceAndPrimaryBuffer()
{
	HRESULT result;
	
	// Initialize the direct sound interface COMMPTR for the default sound device.
	result = DirectSoundCreate8(NULL, &directSound, NULL);
	if (FAILED(result))
	{
		return false;
	}

	// Set the cooperative level to priority and provide console HWND
	result = directSound->SetCooperativeLevel(GetDesktopWindow(), DSSCL_PRIORITY);
	if (FAILED(result))
	{
		return false;
	}

	bufferDesc.dwSize = sizeof(DSBUFFERDESC);
	bufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME;
	bufferDesc.dwBufferBytes = 0;
	bufferDesc.dwReserved = 0;
	bufferDesc.lpwfxFormat = NULL;
	bufferDesc.guid3DAlgorithm = GUID_NULL;

	// Get control of the primary sound buffer on the default sound device.
	result = directSound->CreateSoundBuffer(&bufferDesc, &primaryBuffer, NULL);
	if (FAILED(result))
	{
		return false;
	}

	result = primaryBuffer->SetFormat(waveFormat.get());
	if (FAILED(result))
	{
		return false;
	}
	return true;
}

bool DirectSound::initialiseSecondaryBuffer()
{
	DSBUFFERDESC bufferDesc;
	HRESULT result;
	CComPtr<IDirectSoundBuffer> tempBuffer;

	//Secondary Buffer Description with the size of four seconds
	bufferDesc.dwSize = sizeof(DSBUFFERDESC);
	bufferDesc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2;
	bufferDesc.dwBufferBytes = 4 * waveFormat->nAvgBytesPerSec;
	bufferDesc.dwReserved = 0;
	bufferDesc.lpwfxFormat = waveFormat.get();
	bufferDesc.guid3DAlgorithm = GUID_NULL;

	result = directSound->CreateSoundBuffer(&bufferDesc, &tempBuffer, NULL);
	if (FAILED(result))
	{
		return false;
	}

	//create the secondary buffer.
	result = tempBuffer->QueryInterface(IID_IDirectSoundBuffer8, (void**)&secondaryBuffer);
	if (FAILED(result))
	{
		return false;
	}

	return true;
}

bool DirectSound::playWaveFile()
{
	HRESULT result;
	CComQIPtr<IDirectSoundNotify8, &IID_IDirectSoundNotify8> directSoundNotify;
	DSBPOSITIONNOTIFY positionNotify[2];
	
	//pointers passed through to userInput detection Thread for playback manipulation
	bool * playingPtr = &playing;
	bool * finishedPtr = &finished;
	bool * firstPlayPtr = &firstplay;
	int * secondaryBufferOffsetPtr = &secondaryBufferOffset;

	result = secondaryBuffer->SetCurrentPosition(0);
	if (FAILED(result))
	{
		return false;
	}

	// Set volume of the buffer to 100%.
	result = secondaryBuffer->SetVolume(DSBVOLUME_MAX);
	if (FAILED(result))
	{
		return false;
	}

	result = secondaryBuffer->QueryInterface(IID_IDirectSoundNotify8, (LPVOID*)&directSoundNotify);
	if (FAILED(result))
	{
		return false;
	}

	HANDLE playEventHandles[2];
	playEventHandles[0] = CreateEvent(NULL, FALSE, FALSE, NULL);
	playEventHandles[1] = CreateEvent(NULL, FALSE, FALSE, NULL);

	//set notifications with interrupts at the positions of One and Three Seconds.
	positionNotify[0].dwOffset = 2 * waveFormat->nAvgBytesPerSec / 2 - 1;
	positionNotify[0].hEventNotify = playEventHandles[0];
	positionNotify[1].dwOffset = 6 * waveFormat->nAvgBytesPerSec / 2 - 1;
	positionNotify[1].hEventNotify = playEventHandles[1];
	
	
	directSoundNotify->SetNotificationPositions(2, positionNotify);
	directSoundNotify.Release();


	while (true)
	{
		queueLock.lock();
		if (fourSeconds)
		{
			queueLock.unlock();
			break;
		}
		queueLock.unlock();
	}
	writeData(0);
	writeData(2 * waveFormat->nAvgBytesPerSec);

	//userInput thread initialised with boolean pointers too manipulate values used to manipulate playback
	threads.push_back(std::thread([=] {threadPlay(std::ref(playingPtr), std::ref(finishedPtr), std::ref(firstPlayPtr), std::ref(secondaryBufferOffsetPtr)); }));
	//threads.push_back(std::thread([=] {volumeControl(); }));

	while (!finished) 
	{
		while (playing)
		{
			result = WaitForMultipleObjects(2, playEventHandles, FALSE, INFINITE);

			//Song has finished playing
			if (secondaryBufferOffset >= dataBufferSize)
			{
				stop();
				cout << "finished playing Song!" << endl;
				cout << "--------------------------------" << endl;
				finished = true;
				playing = false;
				break;
			}
			//First Second Notification
			if (WAIT_OBJECT_0 == result)
			{
				if (!firstplay)
				{
					writeData(2 * waveFormat->nAvgBytesPerSec);
				}
			}
			//Third Second Notification
			if (WAIT_OBJECT_0 + 1 == result)
			{
				writeData(0);
			}
			firstplay = false;
		}
	}

	CloseHandle(playEventHandles[1]);
	CloseHandle(playEventHandles[0]);

	return true;
}

bool DirectSound::play()
{
	HRESULT result;
	result = secondaryBuffer->Play(0,0,DSBPLAY_LOOPING);
	if (FAILED(result))
	{
		return false;
	}
	return true;
}

bool DirectSound::stop()
{
	HRESULT result;
	result = secondaryBuffer->Stop();
	if (FAILED(result))
	{
		return false;
	}
	return true;
}

//writes data to specified position within the buffer
HRESULT DirectSound::writeData(int bufferOffset)
{
	HRESULT result;
	unsigned char * bufferPtr1;
	unsigned long   bufferSize1;
	unsigned char * bufferPtr2;
	unsigned long   bufferSize2;

	result = secondaryBuffer->Lock(bufferOffset, 2 * waveFormat->nAvgBytesPerSec, (void**)&bufferPtr1, (DWORD*)&bufferSize1, (void**)&bufferPtr2, (DWORD*)&bufferSize2, 0);
	if (FAILED(result))
	{
		return false;
	}

	if (secondaryBufferOffset >= dataBuffer.size())
	{
		memset(bufferPtr1, 0, bufferSize1);
	}
	else
	{
		if (bufferSize1 > (dataBuffer.size() - secondaryBufferOffset)) //fill the secondaryBuffer with remaining audio data, then fill the rest of the space with silence
		{
			int restOfData = dataBuffer.size() - secondaryBufferOffset;
			memcpy(bufferPtr1, dataBuffer.data() + secondaryBufferOffset, restOfData);
			memset(bufferPtr1 + restOfData, 0, bufferSize1 - restOfData);
			secondaryBufferOffset += restOfData;
		}
		else
		{
			memcpy(bufferPtr1, dataBuffer.data() + secondaryBufferOffset, bufferSize1);
			secondaryBufferOffset += bufferSize1;
		}
	}

	result = secondaryBuffer->Unlock((void*)bufferPtr1, bufferSize1, (void*)bufferPtr2, bufferSize2);
	if (FAILED(result))
	{
		return false;
	}
}

/*
Continously retrieve Audio Data of the chosen song
using a buffer of no more then 12800
*/
void DirectSound::threadWaveSecond1()
{
	bool done = false;
	while (!done)
	{
		PacketContents oneSecondOfAudio = clientSocket->receive(12800);
		int byteOffset = 0;
		
		while (oneSecondOfAudio.byteSize != 0 && byteOffset != dataBufferSize)
		{
			queueLock.lock();
			memcpy(dataBuffer.data() + byteOffset, oneSecondOfAudio.content.data(), oneSecondOfAudio.byteSize);
			byteOffset += oneSecondOfAudio.byteSize;
			if (!fourSeconds)
			{
				if (byteOffset >= 4 * waveFormat->nAvgBytesPerSec || byteOffset >=  dataBufferSize)
				{
					bool * fourSecondPtr = &fourSeconds;
					*fourSecondPtr = true;
				}
			}
			
			if (byteOffset < dataBufferSize)
			{
					oneSecondOfAudio = clientSocket->receive(12800);
			}
			else
			{
				oneSecondOfAudio.byteSize = 0;
				byteOffset = dataBufferSize;
			}
			queueLock.unlock();
		}
		done = true;
	}
}


/*This thread is used to detect user input during playback
to stop, pause/resume or restart the song.
*/
void DirectSound::threadPlay(bool * playingPtr, bool * finishedPtr, bool * firstPlayPtr, int * offset)
{
	bool done = false;
	while (!done)
	{
		HRESULT result;
		if (!play())
		{
			*playingPtr = false;
			break;
		}
		*playingPtr = true;
		cout << "Press Enter/Return to Stop Play Back " << endl << 
				"Space Bar To Pause And Resume Playback" << endl << 
				"OR BackSpace too restart Song from the beginning" << endl;
		cout << "--------------------------------" << endl;
		
		/* this encapsulated while loop, detects for user input while the song has not finished playing, 
		using conditions allows the user to resume plackback, restart from the beginning or stop playback entirely.
		*/
		while (!*finishedPtr)
		{
			if (kbhit())
			{
				char c = getch();
				//User Pauses Playback
				if ( c == 32)
				{
					cout << "You have Paused the song. Press SpaceBar to Resume" << endl;
					cout << "--------------------------------" << endl;
					stop();
					*playingPtr = false;
				}
				//Song reset to beginning
				if (c == 8)
				{
					cout << "You have Restarted the Song, Press SpaceBar to Pause" << endl;
					cout << "--------------------------------" << endl;
					result = secondaryBuffer->SetCurrentPosition(0);
					*offset = 0;
					writeData(0);
					writeData(2 * waveFormat->nAvgBytesPerSec);
					*firstPlayPtr = true;
					if (!play())
					{
						*playingPtr = false;
						break;
					}
					*playingPtr = true;
				}
				//force playback to stop
				if (c == 13)
				{
					cout << "You have Forced To Stop Playback" << endl;
					cout << "--------------------------------" << endl;
					*offset = dataBufferSize;
					*playingPtr = false;
					*finishedPtr = true;
					secondaryBuffer->SetVolume(DSBVOLUME_MIN);
				}
			}

			while (!*playingPtr && !*finishedPtr)
			{
				if (kbhit())
				{
					char c = getch();
					//User unpaused
					if (c == 32)
					{
						cout << "You have Resumed the Song." << endl;
						cout << "--------------------------------" << endl;
						if (!play())
						{
							*playingPtr = false;
						}
						*playingPtr = true;
					}
					//Song Reset
					if (c  == 8)
					{
						cout << "You have Restarted the Song, Press SpaceBar to Pause" << endl;
						cout << "--------------------------------" << endl;
						result = secondaryBuffer->SetCurrentPosition(0);
						*offset = 0;
						writeData(0);
						writeData(2 * waveFormat->nAvgBytesPerSec);
						*firstPlayPtr = true;
						if (!play())
						{
							*playingPtr = false;
						}
						*playingPtr = true;
					}
					//force playback to stop
					if (c == 13)
					{
						cout << "You have Forced To Stop Playback" << endl;
						cout << "--------------------------------" << endl;
						*offset = dataBufferSize;
						*playingPtr = false;
						*finishedPtr = true;
						secondaryBuffer->SetVolume(DSBVOLUME_MIN);
						if (!play())
						{
							*playingPtr = false;
							break;
						}
					}
				}
			}
		}
		done = true;
	}
}



