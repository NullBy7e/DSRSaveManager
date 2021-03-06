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
#include <thread>
#include <sys/types.h>
#include <sys/stat.h>

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
	wsprintf(pszNewWindowTitle, L"DSR Save Manager v1.0.2 - NullBy7e");

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

bool RegisterBackupHotkey()
{
	return RegisterHotKey(
		NULL,
		1,
		MOD_CONTROL | MOD_NOREPEAT,
		0x42);
}

bool RegisterOpenFolderHotkey()
{
	return RegisterHotKey(
		NULL,
		2,
		MOD_CONTROL | MOD_NOREPEAT,
		0x4F);
}

bool RegisterRestoreBackupHotkey()
{
	return RegisterHotKey(
		NULL,
		3,
		MOD_CONTROL | MOD_NOREPEAT,
		0x52);
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

wstring GetTime()
{
	time_t rawtime;
	struct tm timeinfo;
	char buffer[80];

	time(&rawtime);
	localtime_s(&timeinfo, &rawtime);

	strftime(buffer, 80, "%T", &timeinfo);

	return s2ws(buffer);
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
thread autoSaveThread;

void BackupSaveFolder(bool autosave = false) 
{
	wstring prefix = L"M";
	if (autosave)
	{
		prefix = L"A";
	}

	wstring epochTime = to_wstring(GetEpochTime());
	wstring currentTime = GetTime();
	wstring path = wstring(backupPath + L"\\" + epochTime + L"-" + prefix);

	string converted = ws2s(path);
	
	if(DirectoryExists(converted.c_str()))
	{
		return;
	}

	_tmkdir(path.c_str());
	fs::copy(savePath, path, fs::copy_options::recursive);

	wcout << "[" << currentTime << "] [" << prefix << "] Backup saved to: " << path << endl;

	epochTime.clear();
	currentTime.clear();
	path.clear();
}

void OpenBackupFolder()
{
	ShellExecute(NULL, L"open", backupPath.c_str(), NULL, NULL, SW_SHOWDEFAULT);
}

bool endsWith(const std::wstring &mainStr, const std::wstring &toMatch)
{
	if (mainStr.size() >= toMatch.size() &&
		mainStr.compare(mainStr.size() - toMatch.size(), toMatch.size(), toMatch) == 0)
		return true;
	else
		return false;
}

std::vector<std::wstring> GetBackupFiles()
{
	std::vector<std::wstring> files;

	fs::recursive_directory_iterator iter(backupPath);	
	fs::recursive_directory_iterator end;

	while (iter != end)
	{		
		error_code ec;
		iter.increment(ec);

		if (ec) {
			std::cerr << "Error While Accessing : " << iter->path().string() << " :: " << ec.message() << '\n';
		}

		fs::recursive_directory_iterator iter2(iter->path().wstring());
		fs::recursive_directory_iterator end2;

		auto add = false;

		while (iter2 != end2)
		{
			error_code ec2;
			iter2.increment(ec2);

			if (ec2) {
				std::cerr << "Error While Accessing : " << iter2->path().string() << " :: " << ec2.message() << '\n';
			}

			if (endsWith(iter2->path().wstring(), L"DRAKS0005.sl2"))
			{
				add = true;
			}

			if (!endsWith(iter->path().wstring(), L"-M") && !endsWith(iter->path().wstring(), L"-A"))
			{
				add = false;
			}
		}

		if (add)
		{
			files.push_back(iter->path().wstring());
		}
	}

	return files;
}

time_t GetFileLastModified(string path)
{
	struct stat result;
	if (stat(path.c_str(), &result) == 0)
	{
		return result.st_mtime;
	}

	return NULL;
}

wstring GetMostRecentBackupPath()
{
	wstring lastPath;
	time_t lastTime = NULL;

	auto files = GetBackupFiles();

	for (auto file : files)
	{
		auto time = GetFileLastModified(ws2s(file));

		if (lastTime == NULL || (lastTime != NULL && time > lastTime))
		{
			lastTime = time;
			lastPath = file;
		}
	}

	return lastPath;
}

void RestoreLastBackup()
{
	auto path = GetMostRecentBackupPath();
	if (path == L"")
		return;

	wstring currentTime = GetTime();
	fs::copy(path, savePath, fs::copy_options::overwrite_existing | fs::copy_options::recursive);

	wcout << "[" << currentTime << "] Restored: " << path << endl;
}

void *AutoSaveThreadFunction() 
{
	while (1)
	{
		std::this_thread::sleep_for(600s);
		BackupSaveFolder(true);
	}

	return NULL;	
}

void CreateAutoSaveThread()
{
	cout << "Creating autosave thread..." << endl;
	cout << "Autosave interval is set to 10 minutes." << endl << endl;
	autoSaveThread = std::thread(AutoSaveThreadFunction);
}

void DetachAutoSaveThread()
{
	autoSaveThread.detach();
}

void Cleanup() 
{
	UnregisterHotKey(hwnd, 0xAAAA);
	DetachAutoSaveThread();

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
	
	if (!RegisterBackupHotkey()) 
	{
		cout << "Failed to register hotkey CTRL+B." << endl;
		system("pause");

		return 0;
	}

	if (!RegisterOpenFolderHotkey())
	{
		cout << "Failed to register hotkey CTRL+O." << endl;
		system("pause");

		return 0;
	}

	if (!RegisterRestoreBackupHotkey())
	{
		cout << "Failed to register hotkey CTRL+R." << endl;
		system("pause");

		return 0;
	}

	cout << "DSR Save Manager" << endl << endl;

	string converted = ws2s(backupPath);
	if (!DirectoryExists(converted.c_str()))
	{
		_tmkdir(backupPath.c_str());
	}

	CreateAutoSaveThread();

	wcout << "Save File Location: " << savePath << endl;
	wcout << "Backup Destination: " << backupPath << endl << endl;
	cout << "Use the hotkey CTRL+O to open the backup folder." << endl;
	cout << "Use the hotkey CTRL+B to backup your current save data." << endl << endl;
	cout << "Use the hotkey CTRL+R to restore the latest backup (autosave or manual)." << endl << endl;

	signal(SIGBREAK, onSigBreakSignal);

	MSG msg = { 0 };
	while (GetMessage(&msg, NULL, 0, 0) != 0)
	{
		if (msg.message == WM_HOTKEY)
		{
			if (msg.wParam == 1) 
			{
				BackupSaveFolder();
			}

			if (msg.wParam == 2)
			{
				OpenBackupFolder();
			}

			if (msg.wParam == 3)
			{
				RestoreLastBackup();
			}

			DispatchMessage(&msg);
		}
	}

	return 0;
}
