#include "WaveFile.h"

WaveFile::WaveFile(string wavFileName) {
	audioFileName.reset(new wchar_t[4096]);
	MultiByteToWideChar(CP_ACP, 0, wavFileName.c_str(), -1, audioFileName.get(), 4096);
	readFileNameWithoutDirAndExt();
	Open();
	readFileFmt();
	readFileType();
	readData();
}

WaveFile::~WaveFile()
{
	CloseHandle(fileHandle);
}

//create FileHandle from Wav File for future manipulation 
HANDLE WaveFile::Open() {

	fileHandle = CreateFile(audioFileName.get(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (fileHandle == INVALID_HANDLE_VALUE)
	{
		cout << "Fail" << endl;
		return false;
	}
	if (SetFilePointer(fileHandle, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		return false;
	}
	return fileHandle;
}

HRESULT WaveFile::ReadChunkData(HANDLE fileHandle, void * buffer, DWORD buffersize, DWORD bufferoffset)
{
	HRESULT hr = S_OK;
	DWORD bytesRead;

	if (SetFilePointer(fileHandle, bufferoffset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}
	if (ReadFile(fileHandle, buffer, buffersize, &bytesRead, NULL) == 0)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
	}
	return hr;
}

//Retrieve the FileName as a string without the Directory or File Type Extension
bool WaveFile::readFileNameWithoutDirAndExt()
{
	unique_ptr<wstring> audioFileNameWString(new wstring(&audioFileName.get()[0]));
	fileNameWithoutDirAndExt.assign(audioFileNameWString.get()->begin(), audioFileNameWString.get()->end());
	
	const size_t last_slash_idx = fileNameWithoutDirAndExt.find_last_of("\\/");
	if (std::string::npos != last_slash_idx)
	{
		fileNameWithoutDirAndExt.erase(0, last_slash_idx + 1);
	}

	const size_t period_idx = fileNameWithoutDirAndExt.rfind('.');
	if (std::string::npos != period_idx)
	{
		fileNameWithoutDirAndExt.erase(period_idx);
	}
	audioFileNameWString->clear();
	return true;
}

bool WaveFile::readFileType()
{
	FindChunk(fileHandle, fourccRIFF, chunkSize, chunkPosition);
	ReadChunkData(fileHandle, &filetype, sizeof(DWORD), chunkPosition);
	if (filetype != fourccWAVE)
	{
		return false;
	}
	return true;
}


bool WaveFile::readFileFmt()
{
	FindChunk(fileHandle, fourccFMT, chunkSize, chunkPosition);
	ReadChunkData(fileHandle, &waveFormatExtensible, chunkSize, chunkPosition);

	waveFormatEx.wFormatTag = waveFormatExtensible.Format.wFormatTag;
	waveFormatEx.nSamplesPerSec = waveFormatExtensible.Format.nSamplesPerSec;
	waveFormatEx.wBitsPerSample = waveFormatExtensible.Format.wBitsPerSample;
	waveFormatEx.nChannels = waveFormatExtensible.Format.nChannels;
	waveFormatEx.nBlockAlign = waveFormatExtensible.Format.nBlockAlign;
	waveFormatEx.nAvgBytesPerSec = waveFormatExtensible.Format.nAvgBytesPerSec;
	waveFormatEx.cbSize = waveFormatExtensible.Format.cbSize;

	return true;
}

bool WaveFile::readData()
{
	FindChunk(fileHandle, fourccDATA, chunkSize, chunkPosition);
	dataBufferSize = chunkSize;
	dataBuffer.resize(dataBufferSize);
	ReadChunkData(fileHandle, dataBuffer.data(), dataBufferSize, chunkPosition);
	return true;
}

//return duration of song in seconds
int WaveFile::getDuration(void)
{
	int numSamples = dataBufferSize /
		(waveFormatEx.nChannels * (waveFormatEx.wBitsPerSample / 8));
	int durationSeconds = numSamples / waveFormatEx.nSamplesPerSec;
	return durationSeconds;
}

HRESULT WaveFile::FindChunk(HANDLE fileHandle, FOURCC fourcc, DWORD & chunkSize, DWORD & chunkDataPosition)
{
	HRESULT hr = S_OK;
	DWORD chunkType;
	DWORD chunkDataSize;
	DWORD riffDataSize = 0;
	DWORD fileType;
	DWORD bytesRead = 0;
	DWORD offset = 0;

	if (SetFilePointer(fileHandle, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}
	while (hr == S_OK)
	{
		if (ReadFile(fileHandle, &chunkType, sizeof(DWORD), &bytesRead, NULL) == 0)
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
		if (ReadFile(fileHandle, &chunkDataSize, sizeof(DWORD), &bytesRead, NULL) == 0)
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
		switch (chunkType)
		{
		case fourccRIFF:
			riffDataSize = chunkDataSize;
			chunkDataSize = 4;
			if (ReadFile(fileHandle, &fileType, sizeof(DWORD), &bytesRead, NULL) == 0)
			{
				hr = HRESULT_FROM_WIN32(GetLastError());
			}
			break;

		default:
			if (SetFilePointer(fileHandle, chunkDataSize, NULL, FILE_CURRENT) == INVALID_SET_FILE_POINTER)
			{
				return HRESULT_FROM_WIN32(GetLastError());
			}
		}

		offset += sizeof(DWORD) * 2;
		if (chunkType == fourcc)
		{
			chunkSize = chunkDataSize;
			chunkDataPosition = offset;
			return S_OK;
		}

		offset += chunkDataSize;
		if (bytesRead >= riffDataSize)
		{
			return S_FALSE;
		}
	}
	return S_OK;
}
