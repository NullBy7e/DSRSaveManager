// DSRSaveManager.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <string>
#include <ShlObj.h>
#include <comdef.h>
#include <stdio.h>
#include <tchar.h>
#include <signal.h>
#include <ctime>
#include <cmath>
#include <time.h>
#include <locale>
#include <codecvt>
#include <conio.h>
#include <process.h>
#include <iostream>
#include <fstream>
#include <filesystem>

namespace fs = std::experimental::filesystem;

using namespace std;

#define BUFSIZE MAX_PATH

//https://support.microsoft.com/en-us/help/124103/how-to-obtain-a-console-window-handle-hwnd
HWND GetConsoleHwnd(void)
{
	#define MY_BUFSIZE 1024
	HWND hwndFound; 
	TCHAR pszNewWindowTitle[MY_BUFSIZE];
	TCHAR pszOldWindowTitle[MY_BUFSIZE];

	GetConsoleTitle(pszOldWindowTitle, MY_BUFSIZE);
	wsprintf(pszNewWindowTitle, L"DSR Save Manager");

	SetConsoleTitle(pszNewWindowTitle);
	Sleep(40);
	hwndFound = FindWindow(NULL, pszNewWindowTitle);

	return(hwndFound);
}

//https://stackoverflow.com/questions/143174/how-do-i-get-the-directory-that-a-program-is-running-from
wstring GetExePath()
{
	TCHAR Buffer[BUFSIZE];
	DWORD dwRet;

	dwRet = GetCurrentDirectory(BUFSIZE, Buffer);

	if (dwRet == 0)
	{
		printf("GetCurrentDirectory failed (%d)\n", GetLastError());
		return NULL;
	}
	if (dwRet > BUFSIZE)
	{
		printf("Buffer too small; need %d characters\n", dwRet);
		return NULL;
	}

	return Buffer;
}

//https://stackoverflow.com/questions/25625327/hresult-to-string-getting-documents-path-c
wstring GetDocumentPath()
{
	wchar_t* pOut = nullptr;
	// Retrieve document path (CheckError throws a _com_error exception on failure)
	_com_util::CheckError(::SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_DEFAULT,
		nullptr, &pOut));
	// Attach returned buffer to a smart pointer with custom deleter. This
	// is necessary, because the std::wstring c'tor throws on failure.
	// Without this smart pointer, any exception would leak memory.
	auto deleter = [](void* p) { ::CoTaskMemFree(p); };
	std::unique_ptr<wchar_t, decltype(deleter)> buffer{ pOut, deleter };
	return std::wstring{ buffer.get() };
	// Invisible: Run deleter for buffer, cleaning up allocated resources.
}

bool RegisterHotkey()
{
	return RegisterHotKey(
		NULL,
		1,
		MOD_CONTROL | MOD_NOREPEAT,
		0x42);
}

long GetEpochTime()
{
	time_t rawtime;
	time(&rawtime);

	struct tm now;
	localtime_s(&now, &rawtime);

	return (long)rawtime;
}

wstring s2ws(const std::string& str)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.from_bytes(str);
}

string ws2s(const std::wstring& wstr)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.to_bytes(wstr);
}

//https://stackoverflow.com/questions/8233842/how-to-check-if-directory-exist-using-c-and-winapi/8233867
BOOL DirectoryExists(const char* dirName) {
	DWORD attribs = ::GetFileAttributesA(dirName);
	if (attribs == INVALID_FILE_ATTRIBUTES) {
		return false;
	}
	return (attribs & FILE_ATTRIBUTE_DIRECTORY);
}

HWND hwnd;
wstring backupPath = wstring(GetExePath() + L"\\Backups");
wstring documentPath = GetDocumentPath();
wstring savePath = wstring(documentPath + L"\\NBGI\\DARK SOULS REMASTERED");

void BackupSaveFolder() 
{
	wstring epochTime = to_wstring(GetEpochTime());
	wstring path = wstring(backupPath + L"\\" + epochTime);

	string converted = ws2s(path);
	
	if(DirectoryExists(converted.c_str()))
	{
		return;
	}

	_tmkdir(path.c_str());
	fs::copy(savePath, path, fs::copy_options::recursive);

	wcout << "Backup saved to: " << path << endl;

	epochTime.clear();
	path.clear();
}

void Cleanup() 
{
	UnregisterHotKey(hwnd, 0xAAAA);

	backupPath.clear();
	documentPath.clear();
	savePath.clear();
}

void onSigBreakSignal(int)
{
	Cleanup();
}

int main()
{
	hwnd = GetConsoleHwnd();
	if (hwnd == NULL) {
		cout << "Failed to retrieve window handle." << endl;
		system("pause");

		return 0;
	}

	if (!RegisterHotkey()) 
	{
		cout << "Failed to register hotkey CTRL+B." << endl;
		system("pause");

		return 0;
	}

	cout << "DSR Save Manager v1.0.0" << endl << "Created by NullBy7e " << endl << endl;

	string converted = ws2s(backupPath);
	if (!DirectoryExists(converted.c_str()))
	{
		_tmkdir(backupPath.c_str());
	}

	wcout << "Save File Location: " << savePath << endl;
	wcout << "Backup Destination: " << backupPath << endl << endl;
	cout << "Use the hotkey CTRL+B to backup your current save data." << endl << endl;

	signal(SIGBREAK, onSigBreakSignal);

	MSG msg = { 0 };
	while (GetMessage(&msg, NULL, 0, 0) != 0)
	{
		if (msg.message == WM_HOTKEY)
		{
			BackupSaveFolder();
		}
	}

	return 0;
}
