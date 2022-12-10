/*
 * Thinker - AI improvement mod for Sid Meier's Alpha Centauri.
 * https://github.com/induktio/thinker/
 */

#include "launch.h"

FILE* debug_log = NULL;


bool FileExists(const char* path) {
    return GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES;
}

void ExitFail() {
    DWORD code = GetLastError();
    WCHAR msgBuf[StrBufSize];
    WCHAR displayBuf[StrBufSize];

    FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&msgBuf,
        StrBufSize,
        NULL);

    StringCchPrintf((LPTSTR)displayBuf,
        StrBufSize,
        (code == ERROR_ELEVATION_REQUIRED ?
            TEXT("Unable to start the game due to an error.\n"\
                "Starting the game in this folder might need administrator privileges.\n"\
                "Error code %d: %s") :
            TEXT("Game is unable to start due to an error.\n"\
                "Error code %d: %s")),
        code, msgBuf);

    MessageBox(0, (LPCTSTR)displayBuf, MOD_VERSION, MB_OK|MB_ICONSTOP);
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[]) {
    if (!FileExists(GameExeFile)) {
        MessageBox(0, "Cannot find " GameExeFile ". Game is unable to start.",
            MOD_VERSION, MB_OK|MB_ICONSTOP);
        exit(EXIT_FAILURE);
    }
    if (!FileExists(ModDllFile)) {
        MessageBox(0, "Cannot find " ModDllFile ". Game is unable to start.",
            MOD_VERSION, MB_OK|MB_ICONSTOP);
        exit(EXIT_FAILURE);
    }
    if (DEBUG) {
        debug_log = fopen("debugstart.txt", "w");
    }
    PROCESS_INFORMATION pi = {};
    STARTUPINFO si = {};
    si.cb = sizeof(si);
    char cmdline[StrBufSize] = GameExeFile;

    for (int i=1; i<argc; i++) {
        if (strlen(cmdline) + strlen(argv[i]) >= sizeof(cmdline)-4) {
            break;
        }
        strncat(cmdline, " ", 2);
        strncat(cmdline, argv[i], sizeof(cmdline)-4);
    }
    debug("cmdline: %s\n", cmdline);

    BOOL createValue = CreateProcess(
        GameExeFile, cmdline, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi);
    if (!createValue) {
        ExitFail();
    }
    LPVOID loadAddr = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    if (loadAddr == NULL) {
        ExitFail();
    }
    LPVOID arg = VirtualAllocEx(
        pi.hProcess, NULL, HookBufSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (arg == NULL) {
        ExitFail();
    }
    BOOL writeValue = WriteProcessMemory(pi.hProcess, arg, ModDllFile, HookBufSize, NULL);
    if (!writeValue) {
        ExitFail();
    }
    HANDLE hThread = CreateRemoteThread(
        pi.hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)loadAddr, arg, NULL, NULL);
    if (hThread == NULL) {
        ExitFail();
    }
    /*
    This should avoid race conditions because CreateRemoteThread begins executing
    immediately while the game process is still in suspended state.
    DLL initialization routines are serialized within a process and the main thread
    does not begin execution until all the DLL initialization is done.
    Sleep calls added nevertheless for compatibility reasons.
    */
    Sleep(100);
    if (!ResumeThread(pi.hThread)) {
        ExitFail();
    }
    WaitForSingleObject(pi.hProcess, 100);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return 0;
}


