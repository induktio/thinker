
#include "launch.h"

FILE* debug_log = NULL;


bool FileExists(const char* path) {
    return GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES;
}

void ExitFail() {
    MessageBox(0, "Error while attempting to patch game binary. Game is unable to start.",
        MOD_VERSION, MB_OK|MB_ICONSTOP);
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
    char cmdline[1024] = GameExeFile;

    for (int i=1; i<argc; i++) {
        if (strlen(cmdline) + strlen(argv[i]) >= sizeof(cmdline)-4) {
            break;
        }
        strncat(cmdline, " ", 2);
        strncat(cmdline, argv[i], sizeof(cmdline)-4);
    }
    debug("cmdline: %s\n", cmdline);

    BOOL value = CreateProcess(
        GameExeFile, cmdline, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi);
    if (!value) {
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
    if (!WriteProcessMemory(pi.hProcess, arg, ModDllFile, HookBufSize, NULL)) {
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
    */
    if (!ResumeThread(pi.hThread)) {
        ExitFail();
    }
//    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

	return 0;
}


