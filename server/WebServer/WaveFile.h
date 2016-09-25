#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <iostream>
#include <memory>
#include <mmreg.h>
#include <vector>
#define fourccRIFF MAKEFOURCC('R', 'I', 'F', 'F')
#define fourccDATA MAKEFOURCC('d', 'a', 't', 'a')
#define fourccFMT  MAKEFOURCC('f', 'm', 't', ' ')
#define fourccWAVE MAKEFOURCC('W', 'A', 'V', 'E')
#define fourccXWMA MAKEFOURCC('X', 'W', 'M', 'A')
using namespace std;
class WaveFile
{
public:
	WaveFile(string audioFileName);
	~WaveFile();
	HRESULT FindChunk(HANDLE fileHandle, FOURCC fourcc, DWORD & chunkSize, DWORD & chunkDataPosition);
	HRESULT ReadChunkData(HANDLE fileHandle, void * buffer, DWORD buffersize, DWORD bufferoffset);
	HANDLE Open();
	bool readFileNameWithoutDirAndExt();
	bool readFileType();
    bool readFileFmt();
	bool readData();
	bool readDataFromOffset();

	std::string				   getFileName(void) { return (fileNameWithoutDirAndExt); }
	WAVEFORMATEX			   getWaveFMT(void) { return (waveFormatEx); }
	std::vector<unsigned char> getReadData(void) { return (dataBuffer); }
	DWORD					   getFileType(void) { return (filetype); }
	DWORD					   getDataBufferSize(void) { return (dataBufferSize); }
	int						   getDuration(void);
	
private:
	WAVEFORMATEXTENSIBLE  waveFormatExtensible = { 0 };
	WAVEFORMATEX		  waveFormatEx;
	unique_ptr<wchar_t>		  audioFileName;
	std::string			  fileNameWithoutDirAndExt;
	DWORD				  chunkSize;
	DWORD				  chunkPosition;
	DWORD				  filetype;
	HANDLE			      fileHandle;
	vector<unsigned char> dataBuffer;
	DWORD			      dataBufferSize;
};