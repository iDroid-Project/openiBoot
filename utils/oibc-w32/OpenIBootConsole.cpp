/*
 * OpeniBoot Console for Microsoft Windows
 *
 * Copyright (c) 2010 Ricky Taylor
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#include <windows.h>
#include <stdio.h>

static const size_t giSendMax = 16;
static size_t gpSendAmount[giSendMax];
static char* gpSendData[giSendMax];
static int giSendCount = 0;
static bool gbRunning = true;
static HANDLE gInThread;
static HANDLE gOutThread;
static CRITICAL_SECTION gReadSection;

void parseCommand(HANDLE hOib, char *cmd)
{
	if(strcmp(cmd, "quit") == 0)
		gbRunning = false;
}

void sendFile(HANDLE hOib, char *_fileName, int _location)
{
	OVERLAPPED sfOverlapped = {0};
	HANDLE hFile = CreateFileA(_fileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "Failed to open '%s' to send.\n", _fileName);
		return;
	}

	sfOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if(!sfOverlapped.hEvent)
	{
		fprintf(stderr, "Failed to allocate event for sending file.\n");
		CloseHandle(hFile);
		return;
	}

	DWORD fSize = GetFileSize(hFile, NULL);
	if(fSize == (DWORD)INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "Failed to calculate file size.\n");
		goto cleanup;
	}
	
	char buff[1024];
	int amt;
	DWORD numWritten;

	// Note: The superflous \ns are here to prevent madness in the
	// peripheral driver... Bug Ricky26 to fix it at some point -- Ricky26
	if(_location < 0)
		amt = sprintf_s(buff, "sendfile 0x09000000 %d\n\n\n\n", fSize);
	else
		amt = sprintf_s(buff, "sendfile %d %d\n\n\n\n", _location, fSize);

	if(amt <= 0)
	{
		fprintf(stderr, "Failed to build sendfile command.\n");
		goto cleanup;
	}

	if(!WriteFile(hOib, buff, amt-1, &numWritten, &sfOverlapped))
	{
		if(GetLastError() != ERROR_IO_PENDING
			|| WaitForSingleObject(sfOverlapped.hEvent, INFINITE) != WAIT_OBJECT_0)
		{
			fprintf(stderr, "Failed to send sendfile command.\n");
			goto cleanup;
		}
	}

	// POTENTIAL BUG: We probably can't allocate enough
	// memory for larger files, so this will fail,
	// we should make the driver provide some kind of
	// transaction-layer so we can send the file without
	// worrying about storing it all in memory.
	char *data = new char[fSize];
	DWORD numRead;

	ReadFile(hFile, data, fSize, &numRead, NULL);

	// POTENTIAL BUG: ReadFile not reading the whole file.
	// I should check numRead and error out. -- Ricky26

	if(!WriteFile(hOib, data, fSize, &numWritten, &sfOverlapped))
	{
		if(GetLastError() != ERROR_IO_PENDING
			|| WaitForSingleObject(sfOverlapped.hEvent, INFINITE) != WAIT_OBJECT_0)
		{
			fprintf(stderr, "Failed to write file data command.\n");
			goto cleanup;
		}
	}

	delete[] data;

cleanup:
	CloseHandle(sfOverlapped.hEvent);
	CloseHandle(hFile);
}

void receiveFile(HANDLE hOib, char *_fileName, size_t _size, int _location)
{
	OVERLAPPED rfOverlapped = {0};
	HANDLE hFile = CreateFileA(_fileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "Failed to open '%s' for writing.\n", _fileName);
		return;
	}

	if(_size == 0)
	{
		CloseHandle(hFile);
		return;
	}

	rfOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if(!rfOverlapped.hEvent)
	{
		fprintf(stderr, "Failed to allocate event for receiving file.\n");
		CloseHandle(hFile);
		return;
	}

	EnterCriticalSection(&gReadSection);

	char buff[1024];
	int amt;
	DWORD numWritten;

	if(_location < 0)
		amt = sprintf_s(buff, "recvfile 0x09000000 %ul\n", _size);
	else
		amt = sprintf_s(buff, "recvfile %d %ul\n", _location, _size);

	if(amt <= 0)
	{
		fprintf(stderr, "Failed to build getfile command.\n");
		goto cleanup;
	}

	WriteFile(hOib, buff, amt-1, &numWritten, NULL);

	if(numWritten <= 0)
	{
		fprintf(stderr, "Failed to send getfile command.\n");
		goto cleanup;
	}

	// POTENTIAL BUG: We need to make sure that the buffer
	// in the driver is cleared at this point, or at least
	// that it is not used, or we will get OIB output mixed
	// in with our file! -- Ricky26
	amt = 0;
	while(amt < _size)
	{
		DWORD numRead;
		ReadFile(hOib, buff, 512, &numRead, NULL);
		WriteFile(hFile, buff, numRead, &numWritten, NULL);

		if(numWritten < numRead)
		{
			fprintf(stderr, "Failed to write to file.\n");
			goto cleanup;
		}

		amt += numRead;
	}

cleanup:
	LeaveCriticalSection(&gReadSection);
	CloseHandle(hFile);
	CloseHandle(rfOverlapped.hEvent);
}

DWORD WINAPI inputThread(LPVOID _param)
{
	HANDLE hFile = (HANDLE)_param;
	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	OVERLAPPED writeOverlapped = {0};
	if(hFile == INVALID_HANDLE_VALUE || hIn == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "Failed to get stdin or OpeniBoot handle.\n");
		gbRunning = false;
		return 1;
	}

	writeOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if(!writeOverlapped.hEvent)
	{
		fprintf(stderr, "Failed to allocate event for writing.\n");
		return 3;
	}

	while(gbRunning)
	{
		char conInput[1024];
		DWORD amtRead;
		if(ReadFile(hIn, conInput, sizeof(conInput)-2, &amtRead, NULL))
		{
			if(amtRead > 0)
			{
				if(conInput[amtRead-1] != '\n')
				{
					conInput[amtRead] = '\n';
					conInput[amtRead+1] = '\0';
				}
				else
					conInput[amtRead] = '\0';

				switch(conInput[0])
				{
				case '!': // Send File
					{
						conInput[amtRead-2] = '\0'; // No trailing \r\n.
						char *cmd = conInput+1;

						int iLocation = -1;
						char *szLocation = strchr(cmd, '@');
						if(szLocation)
						{
							*szLocation = 0;
							szLocation++;

							sscanf(szLocation, "%d", &iLocation);
						}

						sendFile(hFile, cmd, iLocation);
					}
					break;

				case '~': // Receive File
					{
						conInput[amtRead-2] = '\0'; // No trailing \r\n.
						char *cmd = conInput+1;

						int iLocation = -1;
						char *szLocation = strchr(cmd, '@');
						if(szLocation)
						{
							*szLocation = 0;
							szLocation++;

							sscanf(szLocation, "%d", &iLocation);
						}

						size_t iSize = 0;
						char *szSize = strchr(cmd, ':');
						if(szSize)
						{
							*szSize = 0;
							szSize++;

							sscanf(szSize, "%ul", &iSize);
						}

						receiveFile(hFile, cmd, iSize, iLocation);
					}
					break;

				case ':': // Local Command
					{
						conInput[amtRead-2] = '\0'; // No trailing \r\n.
						parseCommand(hFile, conInput+1);
					}
					break;

				default: // Remote Command
					{
						DWORD written;
						if(!WriteFile(hFile, conInput, amtRead, &written, &writeOverlapped)
							&& GetLastError() == ERROR_IO_PENDING)
							WaitForSingleObject(writeOverlapped.hEvent, INFINITE);
					}
					break;
				}
			}
		}
		else
		{
			fprintf(stderr, "Failed to read from stdin.\n");
			gbRunning = false;
			CloseHandle(writeOverlapped.hEvent);
			return 2;
		}
	}

	CloseHandle(writeOverlapped.hEvent);
	return 0;
}

DWORD WINAPI outputThread(LPVOID _param)
{
	HANDLE hFile = (HANDLE)_param;
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	OVERLAPPED readOverlapped = {0};
	if(hFile == INVALID_HANDLE_VALUE || hOut == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "Failed to get stdout or OpeniBoot handle.\n");
		gbRunning = false;
		return 1;
	}
	
	readOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if(!readOverlapped.hEvent)
	{
		fprintf(stderr, "Failed to allocate event for reading.\n");
		return 3;
	}

	int ready = 2;
	while(gbRunning)
	{
		char buff[512];
		DWORD numRead;
		int newReady;

		EnterCriticalSection(&gReadSection);

		if(ReadFile(hFile, buff, sizeof(buff), &numRead, &readOverlapped))
			WriteFile(hOut, buff, readOverlapped.InternalHigh, NULL, NULL);
		else if(GetLastError() == ERROR_IO_PENDING)
		{
			if(WaitForSingleObject(readOverlapped.hEvent, INFINITE) == WAIT_OBJECT_0)
				WriteFile(hOut, buff, readOverlapped.InternalHigh, NULL, NULL);
		}

		LeaveCriticalSection(&gReadSection);

		Sleep(50);
	}

	CloseHandle(readOverlapped.hEvent);

	return 0;
}

int main(int _argc, char **_argv)
{
	InitializeCriticalSection(&gReadSection);

	HANDLE hFile = CreateFileA("\\\\.\\OpeniBoot", GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "Failed to open OpeniBoot connection (%d).\n", GetLastError());
		return 1;
	}

	gInThread = CreateThread(NULL, 0, inputThread, (void*)hFile, 0, NULL);
	if(gInThread == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "Failed to create input thread.\n");
		return 2;
	}

	gOutThread = CreateThread(NULL, 0, outputThread, (void*)hFile, 0, NULL);
	if(gOutThread == INVALID_HANDLE_VALUE)
	{
		TerminateThread(gInThread, 2);
		fprintf(stderr, "Failed to create output thread.\n");
		return 3;
	}

	WaitForSingleObject(gInThread, INFINITE);
	WaitForSingleObject(gOutThread, INFINITE);

	return 0;
}

