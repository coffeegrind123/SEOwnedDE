#pragma once

typedef void* FileHandle_t;

enum FileSystemSeek_t
{
	FILESYSTEM_SEEK_HEAD = 0,
	FILESYSTEM_SEEK_CURRENT,
	FILESYSTEM_SEEK_TAIL
};

class IBaseFileSystem
{
public:
	virtual int Read(void* pOutput, int size, FileHandle_t file) = 0;
	virtual int Write(void const* pInput, int size, FileHandle_t file) = 0;
	virtual FileHandle_t Open(const char* pFileName, const char* pOptions, const char* pathID = 0) = 0;
	virtual void Close(FileHandle_t file) = 0;
	virtual void Seek(FileHandle_t file, int pos, FileSystemSeek_t seekType) = 0;
	virtual unsigned int Tell(FileHandle_t file) = 0;
	virtual unsigned int Size(FileHandle_t file) = 0;
	virtual unsigned int Size(const char* pFileName, const char* pPathID = 0) = 0;
	virtual void Flush(FileHandle_t file) = 0;
	virtual bool Precache(const char* pFileName, const char* pPathID = 0) = 0;
	virtual bool FileExists(const char* pFileName, const char* pPathID = 0) = 0;
	virtual bool IsFileWritable(char const* pFileName, const char* pPathID = 0) = 0;
	virtual bool SetFileWritable(char const* pFileName, bool writable, const char* pPathID = 0) = 0;
};

MAKE_INTERFACE_VERSION(IBaseFileSystem, BaseFileSystem, "filesystem_stdio.dll", "VBaseFileSystem011");
