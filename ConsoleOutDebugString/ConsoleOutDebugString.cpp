// ConsoleOutDebugString.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include <locale.h>

HANDLE hProcess = NULL;

void Usages() {
    _ftprintf(stdout, _T("PrintDebugString process_id\n\n"));
    _ftprintf(stdout, _T("process_id\t\tspecify id of process to debug."));
}

void PrintError(LPCTSTR str, DWORD error) {
    LPVOID lpMessageBuffer;
  
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMessageBuffer,
        0,
        NULL);

    _ftprintf(stderr, _T("%s : %s"), str, lpMessageBuffer);

    LocalFree(lpMessageBuffer);
}

void PrintDebugString(OUTPUT_DEBUG_STRING_INFO& info) {
    BYTE strBuffer[2048];
    
    SIZE_T readSize = 0;
    SIZE_T readReqSize = min((info.nDebugStringLength) * (info.fUnicode ? sizeof(wchar_t) : sizeof(char)), sizeof(strBuffer) - sizeof(TCHAR));
    
    if (ReadProcessMemory(hProcess, info.lpDebugStringData, strBuffer, readReqSize, &readSize)) {
        if (info.fUnicode) {
            strBuffer[readSize + 1] = 0;
            strBuffer[readSize + 2] = 0;
            fwprintf(stdout, L"%s", (LPWSTR)strBuffer);
        } else {
            strBuffer[readSize + 1] = 0;
            fprintf(stdout, "%s", (LPSTR)strBuffer);
        }
    }
}

void Shutdown() {
    if (hProcess) {
        CloseHandle(hProcess); hProcess = NULL;
    }
}

#define ERR_OK                  (0)
#define ERR_INVALID_ARGS        (1)
#define ERR_OPEN_PROCESS        (2)
#define ERR_ATTACH_PROCESS      (3)
#define ERR_DEBUG_EVENT         (4)
#define ERR_DETACH_PROCESS      (5)

int _tmain(int argc, _TCHAR* argv[])
{
    setlocale(LC_ALL, setlocale(LC_CTYPE, ""));

    if (argc < 2) {
        Usages();
        Shutdown();
        return ERR_INVALID_ARGS;
    }

    DWORD pid = _tstoi(argv[1]);
    
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (hProcess == NULL) {
        PrintError(_T("failed to OpenProcess"), GetLastError());
        return ERR_OPEN_PROCESS;
    }

    if (!DebugActiveProcess(pid)) {
        PrintError(_T("failed to DebugActiveProcess"), GetLastError());
        Shutdown();
        return ERR_ATTACH_PROCESS;
    }
    
    bool breakLoop = false;

    DEBUG_EVENT debugEvent;
    while(TRUE){
        if(!WaitForDebugEvent(&debugEvent, INFINITE)){
            PrintError(_T("failed to WaitForDebugEvent"), GetLastError());
            Shutdown();
            return ERR_DEBUG_EVENT;
        }

        switch(debugEvent.dwDebugEventCode){
        case OUTPUT_DEBUG_STRING_EVENT:
            PrintDebugString(debugEvent.u.DebugString);
            break;
        case CREATE_PROCESS_DEBUG_EVENT:
            break;
        case CREATE_THREAD_DEBUG_EVENT:
            break;
        case EXIT_THREAD_DEBUG_EVENT:
            break;
        case LOAD_DLL_DEBUG_EVENT:
            break;
        case UNLOAD_DLL_DEBUG_EVENT:
            break;
        case EXCEPTION_DEBUG_EVENT:
            break;
        case RIP_EVENT:
            break;
        case EXIT_PROCESS_DEBUG_EVENT:
            breakLoop = true;
            break;
        default:
            break;
        }

        if (breakLoop) {
            break;
        }

        ContinueDebugEvent(pid, debugEvent.dwThreadId, DBG_CONTINUE);
    }

    if (!DebugActiveProcessStop(pid)) {
        PrintError(_T("failed to DebugActiveProcessStop"), GetLastError());
        Shutdown();
        return ERR_DETACH_PROCESS;
    }

    CloseHandle(hProcess);

    return ERR_OK;
}

