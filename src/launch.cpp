/*
 * Thinker - AI improvement mod for Sid Meier's Alpha Centauri.
 * https://github.com/induktio/thinker/
 *
 * Thinker is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) version 3 of the GPL.
 *
 * Thinker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Thinker.  If not, see <https://www.gnu.org/licenses/>.
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
            TEXT("Game is unable to start due to an error.\n"\
                "Starting the game in this folder might need additional privileges.\n"\
                "Error code %d: %s") :
            TEXT("Game is unable to start due to an error.\n"\
                "Error code %d: %s")),
        code, msgBuf);

    MessageBox(0, (LPCTSTR)displayBuf, MOD_VERSION, MB_OK|MB_ICONSTOP);
    exit(EXIT_FAILURE);
}

void ExitFail(PROCESS_INFORMATION& pi) {
    TerminateProcess(pi.hProcess, ~0u);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    ExitFail();
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

    for (int i = 1; i < argc; i++) {
        if (strlen(cmdline) + strlen(argv[i]) >= sizeof(cmdline)-4) {
            break;
        }
        strncat(cmdline, " ", 2);
        strncat(cmdline, argv[i], sizeof(cmdline)-4);
    }
    debug("CommandLine: %s\n", cmdline);

    BOOL createValue = CreateProcess(
        GameExeFile, cmdline, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi);
    if (!createValue) {
        ExitFail();
    }
    LPVOID loadAddr = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    if (loadAddr == NULL) {
        ExitFail(pi);
    }
    LPVOID arg = VirtualAllocEx(
        pi.hProcess, NULL, HookBufSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (arg == NULL) {
        ExitFail(pi);
    }
    BOOL writeValue = WriteProcessMemory(pi.hProcess, arg, ModDllFile, HookBufSize, NULL);
    if (!writeValue) {
        ExitFail(pi);
    }
    HANDLE hThread = CreateRemoteThread(
        pi.hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)loadAddr, arg, NULL, NULL);
    if (hThread == NULL) {
        ExitFail(pi);
    }
    /*
    When CreateRemoteThread is called, it begins DLL initialization immediately
    while the game process is still in suspended state. We need to still avoid
    any race conditions by making sure DLL initialization function returns first.
    Normally this should not return WAIT_TIMEOUT but terminating the game process
    in that case should not be required.
    */
    DWORD waitValue = WaitForSingleObject(hThread, 5000);
    debug("WaitForSingleObject: %lu\n", waitValue);

    if (waitValue != WAIT_OBJECT_0 && waitValue != WAIT_TIMEOUT) {
        ExitFail(pi);
    }
    if (!ResumeThread(pi.hThread)) {
        ExitFail(pi);
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return 0;
}


