// FileSystemMonitor.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <Windows.h>
#include <ctime>
#include <iomanip>
#include <sstream>

const DWORD BUFFER_SIZE = 32768;



std::wstring GetCurrentTimestamp() {
	auto now = std::time(nullptr);
    struct tm timeinfo;
	localtime_s(&timeinfo, &now);
	std::wstringstream wss;
	wss << std::put_time(&timeinfo, L"[%Y-%m-%d %H:%M:%S] ");
	return wss.str();
}

int main()
{
        HANDLE directoryHandle = CreateFileW(
            L"C:\\",
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS,
            NULL
        );

        if (directoryHandle == INVALID_HANDLE_VALUE) {
            std::wcout << GetCurrentTimestamp() << "Failed to open directory. Error: " << GetLastError() << std::endl;
            return 1;
        }

        std::wcout << GetCurrentTimestamp() << "Directory opened successfully" << std::endl;

        // Buffer to store file change data
        char fileLogBuffer[BUFFER_SIZE];
        DWORD bytesReturned;

        while (true) {
            BOOL success = ReadDirectoryChangesW(
                directoryHandle,
                &fileLogBuffer,
                sizeof(fileLogBuffer),
                true,
                FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_SIZE,
                &bytesReturned,
                NULL,
                NULL
            );

            if (!success) {
                DWORD error = GetLastError();
                std::wcout << GetCurrentTimestamp() << "ReadDirectoryChangesW failed. Error code: " << error << std::endl;

                // Check if we just need to reset our buffer
                if (error == ERROR_NOTIFY_ENUM_DIR) {
                    std::wcout << GetCurrentTimestamp() << "Buffer overflow - continuing to monitor..." << std::endl;
					Sleep(1000);  // Sleep for a bit to avoid a busy loop
                    continue;  // Start the loop again
                }
                else {
                    // For any other error, we should probably exit
                    std::wcout << GetCurrentTimestamp() << "Fatal error - stopping monitor" << std::endl;
                    break;
                }
            }

            // Only process if we got some bytes back
            if (bytesReturned > 0) {
                FILE_NOTIFY_INFORMATION* fileNotifyInfo = (FILE_NOTIFY_INFORMATION*)fileLogBuffer;
                while (true) {
                    std::wstring fileName(fileNotifyInfo->FileName, fileNotifyInfo->FileNameLength / sizeof(wchar_t));
                    std::wstring timestamp = GetCurrentTimestamp();

                    switch (fileNotifyInfo->Action) {
                    case FILE_ACTION_ADDED:
                        std::wcout << timestamp << L"File added: " << fileName << std::endl;
                        break;
                    case FILE_ACTION_REMOVED:
                        std::wcout << timestamp << L"File removed: " << fileName << std::endl;
                        break;
                    case FILE_ACTION_MODIFIED:
                        std::wcout << timestamp << L"File modified: " << fileName << std::endl;
                        break;
                    case FILE_ACTION_RENAMED_OLD_NAME:
                        std::wcout << timestamp << L"File renamed old name: " << fileName << std::endl;
                        break;
                    case FILE_ACTION_RENAMED_NEW_NAME:
                        std::wcout << timestamp << L"File renamed new name: " << fileName << std::endl;
                        break;
                    default:
                        std::wcout << timestamp << L"Unknown action: " << fileName << std::endl;
                        break;
                    }

                    if (fileNotifyInfo->NextEntryOffset == 0) {
                        break;
                    }

                    fileNotifyInfo = (FILE_NOTIFY_INFORMATION*)((char*)fileNotifyInfo + fileNotifyInfo->NextEntryOffset);
                }
            }

            // Clear the buffer for the next batch
            ZeroMemory(&fileLogBuffer, sizeof(fileLogBuffer));
        }
        CloseHandle(directoryHandle);
        return 0;
    }

