#include <windows.h>
#include <tchar.h>
#include <iostream>
using namespace std;

int main()
{
    //to create a separate process and launch the Wox application
    STARTUPINFO info = { sizeof(info) };
    PROCESS_INFORMATION processInfo;
    wstring cmdLine = L"..\\Output\\Release\\Wox.exe";

    if (!CreateProcess(NULL, (LPSTR)cmdLine.c_str(), NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo))
    {
        printf("CreateProcess failed (%d).\n", GetLastError());
    }

    WaitForSingleObject(processInfo.hProcess, INFINITE);
    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);

}